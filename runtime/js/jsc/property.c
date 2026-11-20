/*
 * quixjs - A lightweight JS runtime using SDL2 and JavaScriptCore
 * Copyright (C) 2025  Vinicio Valbuena (lowlevel.1989)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// Propiedades: la cache de nombres, JS_GetPropertyStr y compania, los atomos
// y las propiedades ocultas (JS_SetHidden).
// Parte de runtime/js/jsc.c, que une las partes y se compila como un solo
// archivo (ver runtime/js/README.md): no se compila sola. Lo compartido esta
// en jsc.h.

#include "jsc.h"

// ---------- nombres de propiedad (cache de JSStringRef) ----------

#define NAME_CACHE 2048
static struct { char *s; uint32_t h; JSStringRef js; } names[NAME_CACHE];

static JSStringRef name_ref(const char *s) {
  uint32_t h = 2166136261u;
  for (const char *p = s; *p; p++) h = (h ^ (uint8_t)*p) * 16777619u;
  for (uint32_t i = 0; i < NAME_CACHE; i++) {
    uint32_t k = (h + i) & (NAME_CACHE - 1);
    if (!names[k].s) {
      names[k].s = strdup(s);
      names[k].h = h;
      names[k].js = JSStringCreateWithUTF8CString(s);
      return names[k].js;
    }
    if (names[k].h == h && strcmp(names[k].s, s) == 0) return names[k].js;
  }
  return NULL;   // llena: el que llama crea y suelta la suya
}

// el JSStringRef de un nombre (de la cache, o nuevo y *release = 1)
JSStringRef jsc_name_get(const char *s, int *release) {
  JSStringRef r = name_ref(s);
  if (r) {
    *release = 0;
    return r;
  }
  *release = 1;
  return JSStringCreateWithUTF8CString(s);
}

// El objeto de v (las cadenas y numeros se envuelven, como JS)
JSObjectRef jsc_to_object(JSContext *ctx, JSValueConst v) {
  if (JS_IsObject(v)) return (JSObjectRef)v.ref;
  JSValueRef exc = NULL;
  JSObjectRef o = JSValueToObject(ctx->g, ref_of(ctx->g, v), &exc);
  if (exc) {
    JS_Throw(ctx, wrap(ctx->g, exc, 1));
    return NULL;
  }
  return o;
}

// ---------- propiedades ----------

JSValue JS_GetPropertyStr(JSContext *ctx, JSValueConst obj, const char *name) {
  JSObjectRef o = jsc_to_object(ctx, obj);
  if (!o) return JS_EXCEPTION;
  int rel;
  JSStringRef n = jsc_name_get(name, &rel);
  JSValueRef exc = NULL;
  JSValueRef r = JSObjectGetProperty(ctx->g, o, n, &exc);
  if (rel) JSStringRelease(n);
  return result(ctx, r, exc);
}

JSValue JS_GetPropertyUint32(JSContext *ctx, JSValueConst obj, uint32_t idx) {
  JSObjectRef o = jsc_to_object(ctx, obj);
  if (!o) return JS_EXCEPTION;
  JSValueRef exc = NULL;
  JSValueRef r = JSObjectGetPropertyAtIndex(ctx->g, o, idx, &exc);
  return result(ctx, r, exc);
}

int jsc_set_prop(JSContext *ctx, JSValueConst obj, const char *name, JSValue val, unsigned attrs) {
  JSObjectRef o = obj_of(obj);
  if (!o) {
    JS_FreeValue(ctx, val);
    JS_ThrowTypeError(ctx, "cannot set property '%s' of a non-object", name);
    return -1;
  }
  int rel;
  JSStringRef n = jsc_name_get(name, &rel);
  JSValueRef exc = NULL;
  JSObjectSetProperty(ctx->g, o, n, ref_of(ctx->g, val), attrs, &exc);
  if (rel) JSStringRelease(n);
  JS_FreeValue(ctx, val);
  if (exc) {
    JS_Throw(ctx, wrap(ctx->g, exc, 1));
    return -1;
  }
  return 1;
}

int JS_SetPropertyStr(JSContext *ctx, JSValueConst obj, const char *name, JSValue val) {
  return jsc_set_prop(ctx, obj, name, val, 0);
}

int JS_HasPropertyStr(JSContext *ctx, JSValueConst obj, const char *name) {
  JSObjectRef o = obj_of(obj);
  if (!o) return 0;
  int rel;
  JSStringRef n = jsc_name_get(name, &rel);
  int r = JSObjectHasProperty(ctx->g, o, n) ? 1 : 0;
  if (rel) JSStringRelease(n);
  return r;
}

int JS_DeletePropertyStr(JSContext *ctx, JSValueConst obj, const char *name) {
  JSObjectRef o = obj_of(obj);
  if (!o) return 0;
  int rel;
  JSStringRef n = jsc_name_get(name, &rel);
  JSValueRef exc = NULL;
  int r = JSObjectDeleteProperty(ctx->g, o, n, &exc) ? 1 : 0;
  if (rel) JSStringRelease(n);
  return exc ? 0 : r;
}

int JS_DefinePropertyValueStr(JSContext *ctx, JSValueConst obj, const char *name, JSValue val, int flags) {
  return jsc_set_prop(ctx, obj, name, val, attrs_of(flags));
}

int JS_SetPropertyUint32(JSContext *ctx, JSValueConst obj, uint32_t idx, JSValue val) {
  JSObjectRef o = obj_of(obj);
  if (!o) {
    JS_FreeValue(ctx, val);
    JS_ThrowTypeError(ctx, "cannot set an index of a non-object");
    return -1;
  }
  JSValueRef exc = NULL;
  JSObjectSetPropertyAtIndex(ctx->g, o, idx, ref_of(ctx->g, val), &exc);
  JS_FreeValue(ctx, val);
  if (exc) {
    JS_Throw(ctx, wrap(ctx->g, exc, 1));
    return -1;
  }
  return 1;
}

int JS_SetPrototype(JSContext *ctx, JSValueConst obj, JSValueConst proto) {
  JSObjectRef o = obj_of(obj);
  if (!o) return -1;
  JSObjectSetPrototype(ctx->g, o, ref_of(ctx->g, proto));
  return 1;
}

// ---------- atomos: indices de una tabla de nombres ----------

#define MAX_ATOMS 256
static char *atoms[MAX_ATOMS];
static int atom_count;

JSAtom JS_ValueToAtom(JSContext *ctx, JSValueConst v) {
  const char *s = JS_ToCString(ctx, v);
  if (!s) return 0;
  for (int i = 1; i <= atom_count; i++) {
    if (strcmp(atoms[i - 1], s) == 0) {
      JS_FreeCString(ctx, s);
      return (JSAtom)i;
    }
  }
  if (atom_count == MAX_ATOMS) {
    JS_FreeCString(ctx, s);
    return 0;
  }
  atoms[atom_count++] = strdup(s);
  JS_FreeCString(ctx, s);
  return (JSAtom)atom_count;
}

const char *JS_AtomToCString(JSContext *ctx, JSAtom atom) {
  (void)ctx;
  if (atom == 0 || atom > (JSAtom)atom_count) return NULL;
  return strdup(atoms[atom - 1]);
}

void JS_FreeAtom(JSContext *ctx, JSAtom atom) {
  (void)ctx;
  (void)atom;
}

JSValue JS_GetProperty(JSContext *ctx, JSValueConst obj, JSAtom prop) {
  if (prop == 0 || prop > (JSAtom)atom_count) return JS_UNDEFINED;
  return JS_GetPropertyStr(ctx, obj, atoms[prop - 1]);
}

int JS_SetProperty(JSContext *ctx, JSValueConst obj, JSAtom prop, JSValue val) {
  if (prop == 0 || prop > (JSAtom)atom_count) {
    JS_FreeValue(ctx, val);
    return -1;
  }
  return JS_SetPropertyStr(ctx, obj, atoms[prop - 1], val);
}

// ---------- propiedades ocultas (clave Symbol) ----------

#define MAX_HIDDEN 64
static struct { char *name; JSValueRef sym; JSContext *ctx; } hidden[MAX_HIDDEN];
static int hidden_count;

static JSValueRef hidden_key(JSContext *ctx, const char *name) {
  for (int i = 0; i < hidden_count; i++) {
    if (hidden[i].ctx == ctx && strcmp(hidden[i].name, name) == 0) return hidden[i].sym;
  }
  if (hidden_count == MAX_HIDDEN) return NULL;
  JSStringRef d = JSStringCreateWithUTF8CString(name);
  JSValueRef sym = JSValueMakeSymbol(ctx->g, d);
  JSStringRelease(d);
  JSValueProtect(ctx->g, sym);
  hidden[hidden_count].name = strdup(name);
  hidden[hidden_count].sym = sym;
  hidden[hidden_count].ctx = ctx;
  hidden_count++;
  return sym;
}

int JS_SetHidden(JSContext *ctx, JSValueConst obj, const char *name, JSValue val) {
  JSObjectRef o = obj_of(obj);
  JSValueRef key = hidden_key(ctx, name);
  if (!o || !key) {
    JS_FreeValue(ctx, val);
    return -1;
  }
  JSValueRef exc = NULL;
  JSObjectSetPropertyForKey(ctx->g, o, key, ref_of(ctx->g, val), kJSPropertyAttributeDontEnum, &exc);
  JS_FreeValue(ctx, val);
  return exc ? -1 : 1;
}

JSValue JS_GetHidden(JSContext *ctx, JSValueConst obj, const char *name) {
  JSObjectRef o = obj_of(obj);
  JSValueRef key = hidden_key(ctx, name);
  if (!o || !key) return JS_UNDEFINED;
  JSValueRef exc = NULL;
  JSValueRef r = JSObjectGetPropertyForKey(ctx->g, o, key, &exc);
  return result(ctx, r, exc);
}
