/*
 * QuixJS - A lightweight JS runtime using SDL2 and JavaScriptCore
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

// La capa de motor (js.h) sobre la API en C de JavaScriptCore.
//
// - Un JSValue con un objeto o una cadena guarda su JSValueRef. Si es "tuyo"
//   (lo devolvio una funcion de la capa) esta protegido del recolector
//   (JSValueProtect) hasta su JS_FreeValue (JSValueUnprotect).
// - Las funciones de C y los objetos de las clases de los bindings son
//   objetos de JSC creados con una JSClassRef (callbacks): su dato privado
//   (FnPriv u ObjPriv) dice que funcion de C llamar o que puntero de C
//   llevan.
// - Una excepcion queda pendiente en el JSContext hasta JS_GetException.
// - Los finalizadores (en mitad de una recoleccion) no pueden tocar el motor:
//   lo que sueltan (JS_FreeValueRT) se suelta despues (drain).

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <JavaScriptCore/JavaScript.h>
#include "js.h"
// La API privada en C de JavaScriptCore (la compilamos nosotros): memoria,
// recoleccion completa y su profiler de muestreo
JSObjectRef JSGetMemoryUsageStatistics(JSContextRef ctx);
void JSSynchronousGarbageCollectForDebugging(JSContextRef ctx);
bool JSContextGroupEnableSamplingProfiler(JSContextGroupRef group);
void JSContextGroupDisableSamplingProfiler(JSContextGroupRef group);
JSStringRef JSContextGroupTakeSamplesFromSamplingProfiler(JSContextGroupRef group);
void JSContextGroupPerformIdleWork(JSContextGroupRef group, double milliseconds);   // parches de quixjs
double JSContextGroupGCPressure(JSContextGroupRef group);
void JSContextGroupCollectNow(JSContextGroupRef group);

#define MAX_CLASSES   64
#define MAX_CONTEXTS  4
#define FN_MAGIC      0x46554e43u   // "FUNC"
#define OBJ_MAGIC     0x4f424a43u   // "OBJC"

typedef struct {
  JSClassDef def;
  JSClassRef cls;
  int used;
  uint32_t prof_made;            // objetos creados mientras mide el profiler
} ClassEntry;

// objetos sin clase que crea quixjs (JS_NewObject, JS_NewArray, typed arrays)
static uint32_t prof_plain_objects, prof_arrays, prof_typed;

struct JSRuntime {
  JSContextGroupRef group;
  ClassEntry classes[MAX_CLASSES];
  JSContext *contexts[MAX_CONTEXTS];
  int context_count;
  // lo que soltaron los finalizadores, para soltarlo fuera de la recoleccion
  JSValueRef *pending_free;
  int pending_count, pending_cap;
};

struct JSContext {
  JSRuntime *rt;
  JSGlobalContextRef g;
  void *opaque;
  JSValueRef exception;          // pendiente (protegida) o NULL
  JSValueRef class_proto[MAX_CLASSES];
  JSObjectRef function_proto;    // Function.prototype
  JSObjectRef define_property;   // Object.defineProperty
  JSObjectRef error_ctor[3];     // TypeError, RangeError, SyntaxError
};

typedef struct {
  uint32_t magic;
  JSCFunctionEnum kind;
  JSCFunctionType f;
  JSCFunctionData *fdata;
  int length, magic_arg;
  int data_len;
  JSValue *data;                 // JS_NewCFunctionData (protegidos)
  JSRuntime *rt;
  // para el profiler: su nombre, y lo que tarda y cuantas veces se llama
  // (solo mientras mide); todas en una lista
  char name[48];
  uint64_t prof_ns;
  uint32_t prof_calls;
  struct FnPrivLink { void *prev, *next; } link;
} FnPriv;

static FnPriv *all_fns;          // la lista de funciones de C vivas
static int native_prof;          // el profiler mide las funciones de C

typedef struct {
  uint32_t magic;
  JSClassID id;
  void *opaque;
} ObjPriv;

static JSClassRef fn_class, ctor_class;
static JSClassID next_class_id = 1;

// ---------- contextos ----------

static JSContext *ctx_of(JSContextRef jctx) {
  JSGlobalContextRef g = JSContextGetGlobalContext(jctx);
  // el runtime de quixjs tiene uno o dos contextos: busqueda lineal
  extern JSRuntime *quix_js_runtimes[];
  for (int r = 0; r < 4; r++) {
    JSRuntime *rt = quix_js_runtimes[r];
    if (!rt) continue;
    for (int i = 0; i < rt->context_count; i++) {
      if (rt->contexts[i]->g == g) return rt->contexts[i];
    }
  }
  return NULL;
}

JSRuntime *quix_js_runtimes[4];

static void drain(JSRuntime *rt) {
  if (!rt->pending_count || !rt->context_count) return;
  JSGlobalContextRef g = rt->contexts[0]->g;
  for (int i = 0; i < rt->pending_count; i++) JSValueUnprotect(g, rt->pending_free[i]);
  rt->pending_count = 0;
}

// ---------- valores ----------

// Un JSValueRef como JSValue; own: protegerlo (sera de quien lo recibe)
static JSValue wrap(JSContextRef c, JSValueRef r, int own) {
  if (!r) return JS_UNDEFINED;
  switch (JSValueGetType(c, r)) {
    case kJSTypeUndefined: return JS_UNDEFINED;
    case kJSTypeNull: return JS_NULL;
    case kJSTypeBoolean: return JS_MKVAL(JS_TAG_BOOL, JSValueToBoolean(c, r) ? 1 : 0);
    case kJSTypeNumber: return JS_MKVAL(JS_TAG_NUMBER, JSValueToNumber(c, r, NULL));
    case kJSTypeString: {
      if (own) JSValueProtect(c, r);
      return (JSValue){ r, JS_REF_STRING, JS_TAG_REF };
    }
    case kJSTypeObject: {
      if (own) JSValueProtect(c, r);
      return (JSValue){ r, JS_REF_OBJECT, JS_TAG_REF };
    }
    case kJSTypeSymbol: {
      if (own) JSValueProtect(c, r);
      return (JSValue){ r, JS_REF_SYMBOL, JS_TAG_REF };
    }
    default: {
      if (own) JSValueProtect(c, r);
      return (JSValue){ r, JS_REF_OTHER, JS_TAG_REF };
    }
  }
}

static JSValueRef ref_of(JSContextRef c, JSValueConst v) {
  switch (v.tag) {
    case JS_TAG_REF: return (JSValueRef)v.ref;
    case JS_TAG_NULL: return JSValueMakeNull(c);
    case JS_TAG_BOOL: return JSValueMakeBoolean(c, v.num != 0);
    case JS_TAG_NUMBER: return JSValueMakeNumber(c, v.num);
    default: return JSValueMakeUndefined(c);
  }
}

static JSObjectRef obj_of(JSValueConst v) {
  return JS_IsObject(v) ? (JSObjectRef)v.ref : NULL;
}

// El objeto de v (las cadenas y numeros se envuelven, como JS)
static JSObjectRef to_object(JSContext *ctx, JSValueConst v) {
  if (JS_IsObject(v)) return (JSObjectRef)v.ref;
  JSValueRef exc = NULL;
  JSObjectRef o = JSValueToObject(ctx->g, ref_of(ctx->g, v), &exc);
  if (exc) {
    JS_Throw(ctx, wrap(ctx->g, exc, 1));
    return NULL;
  }
  return o;
}

// si hubo excepcion la deja pendiente y devuelve JS_EXCEPTION
static JSValue result(JSContext *ctx, JSValueRef r, JSValueRef exc) {
  if (exc) return JS_Throw(ctx, wrap(ctx->g, exc, 1));
  return wrap(ctx->g, r, 1);
}

JSValue JS_DupValue(JSContext *ctx, JSValueConst v) {
  if (v.tag == JS_TAG_REF) JSValueProtect(ctx->g, (JSValueRef)v.ref);
  return v;
}

void JS_FreeValue(JSContext *ctx, JSValue v) {
  if (v.tag == JS_TAG_REF && ctx) JSValueUnprotect(ctx->g, (JSValueRef)v.ref);
}

void JS_FreeValueRT(JSRuntime *rt, JSValue v) {
  if (v.tag != JS_TAG_REF) return;
  if (rt->pending_count == rt->pending_cap) {
    int cap = rt->pending_cap ? rt->pending_cap * 2 : 256;
    JSValueRef *p = realloc(rt->pending_free, cap * sizeof(JSValueRef));
    if (!p) return;
    rt->pending_free = p;
    rt->pending_cap = cap;
  }
  rt->pending_free[rt->pending_count++] = (JSValueRef)v.ref;
}

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
static JSStringRef name_get(const char *s, int *release) {
  JSStringRef r = name_ref(s);
  if (r) {
    *release = 0;
    return r;
  }
  *release = 1;
  return JSStringCreateWithUTF8CString(s);
}

// ---------- runtime y contexto ----------

static void fn_finalize(JSObjectRef obj);
static void fn_release(FnPriv *p);
static JSValueRef fn_call(JSContextRef c, JSObjectRef f, JSObjectRef self, size_t argc, const JSValueRef argv[],
                          JSValueRef *exc);
static JSObjectRef fn_construct(JSContextRef c, JSObjectRef f, size_t argc, const JSValueRef argv[], JSValueRef *exc);

// instanceof con un constructor nuestro: como el de JS (OrdinaryHasInstance),
// buscar ctor.prototype en la cadena de prototipos del objeto. (Un objeto con
// JSClass solo responde a instanceof con este callback.)
static bool fn_has_instance(JSContextRef c, JSObjectRef ctor, JSValueRef v, JSValueRef *exc) {
  if (!JSValueIsObject(c, v)) return false;
  int rel;
  JSStringRef n = name_get("prototype", &rel);
  JSValueRef proto = JSObjectGetProperty(c, ctor, n, exc);
  if (rel) JSStringRelease(n);
  if (*exc || !JSValueIsObject(c, proto)) return false;
  JSValueRef p = JSObjectGetPrototype(c, (JSObjectRef)v);
  for (int depth = 0; depth < 1000 && JSValueIsObject(c, p); depth++) {
    if (JSValueIsStrictEqual(c, p, proto)) return true;
    p = JSObjectGetPrototype(c, (JSObjectRef)p);
  }
  return false;
}

JSRuntime *JS_NewRuntime(void) {
  JSRuntime *rt = calloc(1, sizeof(JSRuntime));
  if (!rt) return NULL;
  rt->group = JSContextGroupCreate();
  if (!fn_class) {
    JSClassDefinition d = kJSClassDefinitionEmpty;
    d.className = "Function";
    d.callAsFunction = fn_call;
    d.finalize = fn_finalize;
    d.hasInstance = fn_has_instance;
    fn_class = JSClassCreate(&d);
    d.callAsConstructor = fn_construct;
    ctor_class = JSClassCreate(&d);
  }
  for (int i = 0; i < 4; i++) {
    if (!quix_js_runtimes[i]) {
      quix_js_runtimes[i] = rt;
      break;
    }
  }
  return rt;
}

void JS_FreeRuntime(JSRuntime *rt) {
  if (!rt) return;
  for (int i = 0; i < 4; i++) {
    if (quix_js_runtimes[i] == rt) quix_js_runtimes[i] = NULL;
  }
  JSContextGroupRelease(rt->group);
  free(rt->pending_free);
  free(rt);
}

static JSObjectRef global_get_obj(JSGlobalContextRef g, const char *path) {
  // "Object.defineProperty": por partes desde el global
  char buf[128];
  snprintf(buf, sizeof buf, "%s", path);
  JSValueRef v = JSContextGetGlobalObject(g);
  for (char *tok = strtok(buf, "."); tok; tok = strtok(NULL, ".")) {
    JSStringRef n = JSStringCreateWithUTF8CString(tok);
    v = JSObjectGetProperty(g, (JSObjectRef)v, n, NULL);
    JSStringRelease(n);
    if (!JSValueIsObject(g, v)) return NULL;
  }
  JSValueProtect(g, v);
  return (JSObjectRef)v;
}

JSContext *JS_NewContext(JSRuntime *rt) {
  if (rt->context_count == MAX_CONTEXTS) return NULL;
  JSContext *ctx = calloc(1, sizeof(JSContext));
  if (!ctx) return NULL;
  ctx->rt = rt;
  ctx->g = JSGlobalContextCreateInGroup(rt->group, NULL);
  ctx->function_proto = global_get_obj(ctx->g, "Function.prototype");
  ctx->define_property = global_get_obj(ctx->g, "Object.defineProperty");
  ctx->error_ctor[0] = global_get_obj(ctx->g, "TypeError");
  ctx->error_ctor[1] = global_get_obj(ctx->g, "RangeError");
  ctx->error_ctor[2] = global_get_obj(ctx->g, "SyntaxError");
  rt->contexts[rt->context_count++] = ctx;
  return ctx;
}

void JS_FreeContext(JSContext *ctx) {
  if (!ctx) return;
  JSRuntime *rt = ctx->rt;
  drain(rt);
  for (int i = 0; i < rt->context_count; i++) {
    if (rt->contexts[i] == ctx) {
      rt->contexts[i] = rt->contexts[--rt->context_count];
      break;
    }
  }
  JSGlobalContextRelease(ctx->g);
  free(ctx);
}

JSRuntime *JS_GetRuntime(JSContext *ctx) { return ctx->rt; }
void JS_SetContextOpaque(JSContext *ctx, void *opaque) { ctx->opaque = opaque; }
void *JS_GetContextOpaque(JSContext *ctx) { return ctx->opaque; }

JSValue JS_GetGlobalObject(JSContext *ctx) {
  return wrap(ctx->g, JSContextGetGlobalObject(ctx->g), 1);
}

// ---------- valores nuevos ----------

JSValue JS_NewObject(JSContext *ctx) {
  if (native_prof) prof_plain_objects++;
  return wrap(ctx->g, JSObjectMake(ctx->g, NULL, NULL), 1);
}

JSValue JS_NewArray(JSContext *ctx) {
  if (native_prof) prof_arrays++;
  JSValueRef exc = NULL;
  JSObjectRef a = JSObjectMakeArray(ctx->g, 0, NULL, &exc);
  return result(ctx, a, exc);
}

JSValue JS_NewString(JSContext *ctx, const char *str) {
  JSStringRef s = JSStringCreateWithUTF8CString(str ? str : "");
  JSValue v = wrap(ctx->g, JSValueMakeString(ctx->g, s), 1);
  JSStringRelease(s);
  return v;
}

JSValue JS_NewStringLen(JSContext *ctx, const char *str, size_t len) {
  char *tmp = malloc(len + 1);
  if (!tmp) return JS_ThrowOutOfMemory(ctx);
  memcpy(tmp, str, len);
  tmp[len] = 0;
  JSValue v = JS_NewString(ctx, tmp);
  free(tmp);
  return v;
}

JSValue JS_NewStringUTF16(JSContext *ctx, const uint16_t *buf, size_t len) {
  JSStringRef s = JSStringCreateWithCharacters((const JSChar *)buf, len);
  JSValue v = wrap(ctx->g, JSValueMakeString(ctx->g, s), 1);
  JSStringRelease(s);
  return v;
}

JS_BOOL JS_IsFunction(JSContext *ctx, JSValueConst v) {
  return JS_IsObject(v) && JSObjectIsFunction(ctx->g, (JSObjectRef)v.ref);
}

JS_BOOL JS_IsArray(JSContext *ctx, JSValueConst v) {
  return JS_IsObject(v) && JSValueIsArray(ctx->g, (JSValueRef)v.ref);
}

JS_BOOL JS_StrictEq(JSContext *ctx, JSValueConst a, JSValueConst b) {
  return JSValueIsStrictEqual(ctx->g, ref_of(ctx->g, a), ref_of(ctx->g, b));
}

// ---------- conversiones ----------

int JS_ToBool(JSContext *ctx, JSValueConst v) {
  switch (v.tag) {
    case JS_TAG_BOOL: case JS_TAG_NUMBER: return v.num != 0 && !isnan(v.num);
    case JS_TAG_REF: return JSValueToBoolean(ctx->g, (JSValueRef)v.ref);
    default: return 0;
  }
}

int JS_ToFloat64(JSContext *ctx, double *res, JSValueConst v) {
  if (v.tag == JS_TAG_NUMBER || v.tag == JS_TAG_BOOL) {
    *res = v.num;
    return 0;
  }
  if (v.tag == JS_TAG_UNDEFINED) {
    *res = NAN;
    return 0;
  }
  if (v.tag == JS_TAG_NULL) {
    *res = 0;
    return 0;
  }
  JSValueRef exc = NULL;
  double d = JSValueToNumber(ctx->g, ref_of(ctx->g, v), &exc);
  if (exc) {
    JS_Throw(ctx, wrap(ctx->g, exc, 1));
    *res = NAN;
    return -1;
  }
  *res = d;
  return 0;
}

// ToInt32 de ECMAScript: modulo 2^32
static int32_t to_int32(double d) {
  if (!isfinite(d)) return 0;
  d = trunc(d);
  if (d >= -2147483648.0 && d <= 2147483647.0) return (int32_t)d;
  double m = fmod(d, 4294967296.0);
  if (m < 0) m += 4294967296.0;
  return (int32_t)(uint32_t)m;
}

int JS_ToInt32(JSContext *ctx, int32_t *res, JSValueConst v) {
  double d;
  if (JS_ToFloat64(ctx, &d, v)) {
    *res = 0;
    return -1;
  }
  *res = to_int32(d);
  return 0;
}

int JS_ToUint32(JSContext *ctx, uint32_t *res, JSValueConst v) {
  int32_t i;
  int r = JS_ToInt32(ctx, &i, v);
  *res = (uint32_t)i;
  return r;
}

int JS_ToInt64(JSContext *ctx, int64_t *res, JSValueConst v) {
  double d;
  if (JS_ToFloat64(ctx, &d, v)) {
    *res = 0;
    return -1;
  }
  if (!isfinite(d)) *res = 0;
  else if (d >= 9.2e18) *res = INT64_MAX;
  else if (d <= -9.2e18) *res = INT64_MIN;
  else *res = (int64_t)d;
  return 0;
}

JSValue JS_ToString(JSContext *ctx, JSValueConst v) {
  if (JS_IsString(v)) return JS_DupValue(ctx, v);
  JSValueRef exc = NULL;
  JSStringRef s = JSValueToStringCopy(ctx->g, ref_of(ctx->g, v), &exc);
  if (exc) return JS_Throw(ctx, wrap(ctx->g, exc, 1));
  JSValue r = wrap(ctx->g, JSValueMakeString(ctx->g, s), 1);
  JSStringRelease(s);
  return r;
}

const char *JS_ToCStringLen(JSContext *ctx, size_t *plen, JSValueConst v) {
  JSValueRef exc = NULL;
  JSStringRef s = JSValueToStringCopy(ctx->g, ref_of(ctx->g, v), &exc);
  if (exc) {
    JS_Throw(ctx, wrap(ctx->g, exc, 1));
    if (plen) *plen = 0;
    return NULL;
  }
  size_t max = JSStringGetMaximumUTF8CStringSize(s);
  char *buf = malloc(max);
  if (!buf) {
    JSStringRelease(s);
    return NULL;
  }
  size_t n = JSStringGetUTF8CString(s, buf, max);   // con el 0 final
  JSStringRelease(s);
  if (plen) *plen = n ? n - 1 : 0;
  return buf;
}

void JS_FreeCString(JSContext *ctx, const char *s) {
  (void)ctx;
  free((void *)s);
}

// ---------- propiedades ----------

static unsigned attrs_of(int flags) {
  unsigned a = 0;
  if (!(flags & JS_PROP_WRITABLE)) a |= kJSPropertyAttributeReadOnly;
  if (!(flags & JS_PROP_ENUMERABLE)) a |= kJSPropertyAttributeDontEnum;
  if (!(flags & JS_PROP_CONFIGURABLE)) a |= kJSPropertyAttributeDontDelete;
  return a;
}

JSValue JS_GetPropertyStr(JSContext *ctx, JSValueConst obj, const char *name) {
  JSObjectRef o = to_object(ctx, obj);
  if (!o) return JS_EXCEPTION;
  int rel;
  JSStringRef n = name_get(name, &rel);
  JSValueRef exc = NULL;
  JSValueRef r = JSObjectGetProperty(ctx->g, o, n, &exc);
  if (rel) JSStringRelease(n);
  return result(ctx, r, exc);
}

JSValue JS_GetPropertyUint32(JSContext *ctx, JSValueConst obj, uint32_t idx) {
  JSObjectRef o = to_object(ctx, obj);
  if (!o) return JS_EXCEPTION;
  JSValueRef exc = NULL;
  JSValueRef r = JSObjectGetPropertyAtIndex(ctx->g, o, idx, &exc);
  return result(ctx, r, exc);
}

static int set_prop(JSContext *ctx, JSValueConst obj, const char *name, JSValue val, unsigned attrs) {
  JSObjectRef o = obj_of(obj);
  if (!o) {
    JS_FreeValue(ctx, val);
    JS_ThrowTypeError(ctx, "cannot set property '%s' of a non-object", name);
    return -1;
  }
  int rel;
  JSStringRef n = name_get(name, &rel);
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
  return set_prop(ctx, obj, name, val, 0);
}

int JS_HasPropertyStr(JSContext *ctx, JSValueConst obj, const char *name) {
  JSObjectRef o = obj_of(obj);
  if (!o) return 0;
  int rel;
  JSStringRef n = name_get(name, &rel);
  int r = JSObjectHasProperty(ctx->g, o, n) ? 1 : 0;
  if (rel) JSStringRelease(n);
  return r;
}

int JS_DeletePropertyStr(JSContext *ctx, JSValueConst obj, const char *name) {
  JSObjectRef o = obj_of(obj);
  if (!o) return 0;
  int rel;
  JSStringRef n = name_get(name, &rel);
  JSValueRef exc = NULL;
  int r = JSObjectDeleteProperty(ctx->g, o, n, &exc) ? 1 : 0;
  if (rel) JSStringRelease(n);
  return exc ? 0 : r;
}

int JS_DefinePropertyValueStr(JSContext *ctx, JSValueConst obj, const char *name, JSValue val, int flags) {
  return set_prop(ctx, obj, name, val, attrs_of(flags));
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

// ---------- errores ----------

JSValue JS_Throw(JSContext *ctx, JSValue obj) {
  if (ctx->exception) JSValueUnprotect(ctx->g, ctx->exception);
  JSValueRef r = ref_of(ctx->g, obj);
  JSValueProtect(ctx->g, r);
  JS_FreeValue(ctx, obj);
  ctx->exception = r;
  return JS_EXCEPTION;
}

JSValue JS_GetException(JSContext *ctx) {
  if (!ctx->exception) return JS_NULL;
  JSValueRef r = ctx->exception;
  ctx->exception = NULL;
  // ya estaba protegida: pasa a ser de quien la recoge
  return wrap(ctx->g, r, 0);
}

static JSValue throw_error(JSContext *ctx, int kind, const char *fmt, va_list ap) {
  char msg[512];
  vsnprintf(msg, sizeof msg, fmt, ap);
  JSStringRef s = JSStringCreateWithUTF8CString(msg);
  JSValueRef arg = JSValueMakeString(ctx->g, s);
  JSStringRelease(s);
  JSValueRef exc = NULL;
  JSObjectRef err = ctx->error_ctor[kind] ? JSObjectCallAsConstructor(ctx->g, ctx->error_ctor[kind], 1, &arg, &exc)
                                          : JSObjectMakeError(ctx->g, 1, &arg, &exc);
  return JS_Throw(ctx, wrap(ctx->g, exc ? exc : err, 1));
}

JSValue JS_ThrowTypeError(JSContext *ctx, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  JSValue r = throw_error(ctx, 0, fmt, ap);
  va_end(ap);
  return r;
}

JSValue JS_ThrowRangeError(JSContext *ctx, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  JSValue r = throw_error(ctx, 1, fmt, ap);
  va_end(ap);
  return r;
}

JSValue JS_ThrowSyntaxError(JSContext *ctx, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  JSValue r = throw_error(ctx, 2, fmt, ap);
  va_end(ap);
  return r;
}

JSValue JS_ThrowOutOfMemory(JSContext *ctx) {
  return JS_ThrowRangeError(ctx, "out of memory");
}

void js_std_dump_error(JSContext *ctx) {
  JSValue e = JS_GetException(ctx);
  const char *msg = JS_ToCString(ctx, e);
  fprintf(stderr, "%s\n", msg ? msg : "(exception)");
  JS_FreeCString(ctx, msg);
  if (JS_IsObject(e)) {
    JSValue st = JS_GetPropertyStr(ctx, e, "stack");
    if (JS_IsString(st)) {
      const char *s = JS_ToCString(ctx, st);
      // la pila de JSC: "funcion@archivo:linea:col" por linea
      if (s && s[0]) fprintf(stderr, "%s\n", s);
      JS_FreeCString(ctx, s);
    }
    JS_FreeValue(ctx, st);
  }
  JS_FreeValue(ctx, e);
  JS_GetException(ctx);   // por si leer la pila dejo otra
}

// ---------- funciones de C ----------

static void fn_finalize(JSObjectRef obj) {
  fn_release(JSObjectGetPrivate(obj));
}

static void fn_release(FnPriv *p) {
  if (!p || p->magic != FN_MAGIC) return;
  FnPriv *prev = p->link.prev, *next = p->link.next;
  if (prev) prev->link.next = next; else if (all_fns == p) all_fns = next;
  if (next) next->link.prev = prev;
  for (int i = 0; i < p->data_len; i++) JS_FreeValueRT(p->rt, p->data[i]);
  free(p->data);
  p->magic = 0;
  free(p);
}

// convierte el JSValue que devolvio la funcion de C para JSC
static JSValueRef finish(JSContext *ctx, JSContextRef c, JSValue r, JSValueRef *exc) {
  if (JS_IsException(r)) {
    if (ctx->exception) {
      *exc = ctx->exception;
      JSValueUnprotect(ctx->g, ctx->exception);   // JSC ya la tiene (exc)
      ctx->exception = NULL;
    } else {
      *exc = JSValueMakeUndefined(c);
    }
    return JSValueMakeUndefined(c);
  }
  JSValueRef ref = ref_of(c, r);
  // la devuelta era de la funcion (protegida): pasa a JSC
  JS_FreeValue(ctx, r);
  return ref;
}

#define ARGS_STACK 16

// llama a la funcion de C de p segun su tipo (args ya con al menos p->length)
static JSValue invoke(JSContext *ctx, FnPriv *p, JSValue self, int argc, JSValue *args, int n, int as_ctor) {
  switch (p->kind) {
    case JS_CFUNC_generic:
      return p->f.generic(ctx, self, argc, args);
    case JS_CFUNC_generic_magic:
      return p->f.generic_magic(ctx, self, argc, args, p->magic_arg);
    case JS_CFUNC_constructor:
    case JS_CFUNC_constructor_or_func:
      if (!as_ctor && p->kind == JS_CFUNC_constructor) return JS_ThrowTypeError(ctx, "must be called with new");
      return p->f.constructor(ctx, self, argc, args);
    case JS_CFUNC_constructor_magic:
    case JS_CFUNC_constructor_or_func_magic:
      if (!as_ctor && p->kind == JS_CFUNC_constructor_magic) return JS_ThrowTypeError(ctx, "must be called with new");
      return p->f.constructor_magic(ctx, self, argc, args, p->magic_arg);
    case JS_CFUNC_getter:
      return p->f.getter(ctx, self);
    case JS_CFUNC_setter:
      return p->f.setter(ctx, self, n > 0 ? args[0] : JS_UNDEFINED);
    case JS_CFUNC_getter_magic:
      return p->f.getter_magic(ctx, self, p->magic_arg);
    case JS_CFUNC_setter_magic:
      return p->f.setter_magic(ctx, self, n > 0 ? args[0] : JS_UNDEFINED, p->magic_arg);
    default:
      return p->fdata(ctx, self, argc, args, p->magic_arg, p->data);
  }
}

static JSValueRef call_cfunc(JSContext *ctx, JSContextRef c, FnPriv *p, JSValue self, size_t argc,
                             const JSValueRef argv[], JSValueRef *exc, int as_ctor) {
  int n = (int)argc > p->length ? (int)argc : p->length;
  JSValue stack[ARGS_STACK];
  JSValue *args = n <= ARGS_STACK ? stack : malloc(sizeof(JSValue) * n);
  if (!args) {
    *exc = JSValueMakeUndefined(c);
    return JSValueMakeUndefined(c);
  }
  for (int i = 0; i < n; i++) args[i] = i < (int)argc ? wrap(c, argv[i], 0) : JS_UNDEFINED;
  JSValue r = invoke(ctx, p, self, (int)argc, args, n, as_ctor);
  if (args != stack) free(args);
  return finish(ctx, c, r, exc);
}

static JSValueRef fn_call(JSContextRef c, JSObjectRef f, JSObjectRef self, size_t argc, const JSValueRef argv[],
                          JSValueRef *exc) {
  FnPriv *p = JSObjectGetPrivate(f);
  if (native_prof) {
    struct timespec a, b;
    clock_gettime(CLOCK_MONOTONIC, &a);
    JSContext *ctx = ctx_of(c);
    if (!p || p->magic != FN_MAGIC || !ctx) return JSValueMakeUndefined(c);
    drain(ctx->rt);
    JSValue this_v = self ? wrap(c, self, 0) : JS_UNDEFINED;
    JSValueRef r = call_cfunc(ctx, c, p, this_v, argc, argv, exc, 0);
    clock_gettime(CLOCK_MONOTONIC, &b);
    p->prof_ns += (uint64_t)((b.tv_sec - a.tv_sec) * 1000000000LL + (b.tv_nsec - a.tv_nsec));
    p->prof_calls++;
    return r;
  }
  JSContext *ctx = ctx_of(c);
  if (!p || p->magic != FN_MAGIC || !ctx) return JSValueMakeUndefined(c);
  drain(ctx->rt);
  JSValue this_v = self ? wrap(c, self, 0) : JS_UNDEFINED;
  return call_cfunc(ctx, c, p, this_v, argc, argv, exc, 0);
}

static JSObjectRef fn_construct(JSContextRef c, JSObjectRef f, size_t argc, const JSValueRef argv[], JSValueRef *exc) {
  FnPriv *p = JSObjectGetPrivate(f);
  JSContext *ctx = ctx_of(c);
  if (!p || p->magic != FN_MAGIC || !ctx) return NULL;
  drain(ctx->rt);
  JSValue new_target = wrap(c, f, 0);
  JSValueRef r = call_cfunc(ctx, c, p, new_target, argc, argv, exc, 1);
  if (*exc) return NULL;
  if (!JSValueIsObject(c, r)) {
    JS_ThrowTypeError(ctx, "constructor did not return an object");
    *exc = ctx->exception;
    JSValueUnprotect(ctx->g, ctx->exception);
    ctx->exception = NULL;
    return NULL;
  }
  return (JSObjectRef)r;
}

static void define_fn_props(JSContext *ctx, JSObjectRef fo, const char *name, int length) {
  JSObjectSetPrototype(ctx->g, fo, ctx->function_proto);
  unsigned ro = kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontEnum;
  int rel;
  JSStringRef n = name_get("name", &rel);
  JSStringRef v = JSStringCreateWithUTF8CString(name ? name : "");
  JSObjectSetProperty(ctx->g, fo, n, JSValueMakeString(ctx->g, v), ro, NULL);
  JSStringRelease(v);
  if (rel) JSStringRelease(n);
  n = name_get("length", &rel);
  JSObjectSetProperty(ctx->g, fo, n, JSValueMakeNumber(ctx->g, length), ro, NULL);
  if (rel) JSStringRelease(n);
}

// ---------- el camino rapido: funciones nativas dentro de la VM ----------
// (parche de quixjs en JavaScriptCore: API/QuixNativeFunction). Los
// argumentos llegan ya en el formato de quixjs, sin pasar por la API publica.
// JavaScriptCore es parte de quixjs (runtime/thirdparty): siempre esta. No
// puede ser una referencia debil: con una libreria estatica el enlazador no
// mete el objeto si nadie lo pide de verdad, y quedaria a NULL.
typedef JSValue QuixValue;
typedef QuixValue (*QuixNativeCallback)(void *priv, JSContextRef ctx, QuixValue thisValue, int argc,
                                        const QuixValue *argv, JSValueRef *exception);
typedef void (*QuixNativeFinalize)(void *priv);
extern JSObjectRef JSQuixMakeNativeFunction(JSContextRef ctx, const char *name, unsigned length, QuixNativeCallback cb,
                                            QuixNativeFinalize fin, void *priv);

static JSContext *last_ctx;
static JSContextRef last_ref;

static QuixValue fast_call(void *priv, JSContextRef c, QuixValue self, int argc, const QuixValue *argv,
                           JSValueRef *exc) {
  FnPriv *p = priv;
  JSContext *ctx = c == last_ref ? last_ctx : ctx_of(c);
  if (!ctx) return JS_UNDEFINED;
  last_ctx = ctx;
  last_ref = c;
  struct timespec t0;
  if (native_prof) clock_gettime(CLOCK_MONOTONIC, &t0);
  drain(ctx->rt);
  int n = argc > p->length ? argc : p->length;
  JSValue stack[ARGS_STACK];
  JSValue *args = n <= ARGS_STACK ? stack : malloc(sizeof(JSValue) * n);
  if (!args) return JS_UNDEFINED;
  for (int i = 0; i < n; i++) args[i] = i < argc ? argv[i] : JS_UNDEFINED;
  JSValue r = invoke(ctx, p, self, argc, args, n, 0);
  if (args != stack) free(args);
  if (JS_IsException(r)) {
    if (ctx->exception) {
      *exc = ctx->exception;
      JSValueUnprotect(ctx->g, ctx->exception);   // JSC ya la tiene
      ctx->exception = NULL;
    } else {
      *exc = JSValueMakeUndefined(c);
    }
    r = JS_UNDEFINED;
  } else {
    // el resultado era de la funcion (protegido): ahora es de JSC, que lo
    // convierte enseguida (sin crear nada en medio: no puede recolectar)
    JS_FreeValue(ctx, r);
  }
  if (native_prof) {
    struct timespec t1;
    clock_gettime(CLOCK_MONOTONIC, &t1);
    p->prof_ns += (uint64_t)((t1.tv_sec - t0.tv_sec) * 1000000000LL + (t1.tv_nsec - t0.tv_nsec));
    p->prof_calls++;
  }
  return r;
}

static void fn_release(FnPriv *p);

static void fast_finalize(void *priv) {
  fn_release(priv);
}

static JSValue new_fn(JSContext *ctx, FnPriv *p, const char *name) {
  p->magic = FN_MAGIC;
  p->rt = ctx->rt;
  if (!p->name[0]) snprintf(p->name, sizeof p->name, "%s", name && *name ? name : "(anonymous)");
  p->link.prev = NULL;
  p->link.next = all_fns;
  if (all_fns) all_fns->link.prev = p;
  all_fns = p;
  int ctor = p->kind == JS_CFUNC_constructor || p->kind == JS_CFUNC_constructor_magic ||
             p->kind == JS_CFUNC_constructor_or_func || p->kind == JS_CFUNC_constructor_or_func_magic;
  if (!ctor) {
    JSObjectRef fast = JSQuixMakeNativeFunction(ctx->g, name, (unsigned)(p->length > 0 ? p->length : 0), fast_call,
                                                fast_finalize, p);
    if (fast) return wrap(ctx->g, fast, 1);
  }
  JSObjectRef fo = JSObjectMake(ctx->g, ctor ? ctor_class : fn_class, p);
  define_fn_props(ctx, fo, name, p->length);
  return wrap(ctx->g, fo, 1);
}

JSValue JS_NewCFunction2(JSContext *ctx, JSCFunction *func, const char *name, int length, JSCFunctionEnum cproto,
                         int magic) {
  FnPriv *p = calloc(1, sizeof(FnPriv));
  if (!p) return JS_ThrowOutOfMemory(ctx);
  p->kind = cproto;
  p->f.generic = func;
  p->length = length;
  p->magic_arg = magic;
  return new_fn(ctx, p, name);
}

JSValue JS_NewCFunctionData(JSContext *ctx, JSCFunctionData *func, int length, int magic, int data_len,
                            JSValueConst *data) {
  FnPriv *p = calloc(1, sizeof(FnPriv));
  if (!p) return JS_ThrowOutOfMemory(ctx);
  p->kind = (JSCFunctionEnum)-1;   // con datos
  p->fdata = func;
  p->length = length;
  p->magic_arg = magic;
  p->data_len = data_len;
  if (data_len > 0) {
    p->data = malloc(sizeof(JSValue) * data_len);
    for (int i = 0; i < data_len; i++) p->data[i] = JS_DupValue(ctx, data[i]);
  }
  return new_fn(ctx, p, "");
}

void JS_SetConstructor(JSContext *ctx, JSValueConst func, JSValueConst proto) {
  JSObjectRef f = obj_of(func), pr = obj_of(proto);
  if (!f || !pr) return;
  int rel;
  JSStringRef n = name_get("prototype", &rel);
  JSObjectSetProperty(ctx->g, f, n, pr, kJSPropertyAttributeDontEnum | kJSPropertyAttributeDontDelete, NULL);
  if (rel) JSStringRelease(n);
  n = name_get("constructor", &rel);
  JSObjectSetProperty(ctx->g, pr, n, f, kJSPropertyAttributeDontEnum, NULL);
  if (rel) JSStringRelease(n);
}

JSValue JS_Call(JSContext *ctx, JSValueConst func, JSValueConst this_val, int argc, JSValueConst *argv) {
  drain(ctx->rt);
  JSObjectRef f = obj_of(func);
  if (!f || !JSObjectIsFunction(ctx->g, f)) return JS_ThrowTypeError(ctx, "not a function");
  JSValueRef stack[ARGS_STACK];
  JSValueRef *args = argc <= ARGS_STACK ? stack : malloc(sizeof(JSValueRef) * argc);
  for (int i = 0; i < argc; i++) args[i] = ref_of(ctx->g, argv[i]);
  JSValueRef exc = NULL;
  JSValueRef r = JSObjectCallAsFunction(ctx->g, f, obj_of(this_val), argc, args, &exc);
  if (args != stack) free(args);
  return result(ctx, r, exc);
}

JSValue JS_CallConstructor(JSContext *ctx, JSValueConst func, int argc, JSValueConst *argv) {
  JSObjectRef f = obj_of(func);
  if (!f || !JSObjectIsConstructor(ctx->g, f)) return JS_ThrowTypeError(ctx, "not a constructor");
  JSValueRef stack[ARGS_STACK];
  JSValueRef *args = argc <= ARGS_STACK ? stack : malloc(sizeof(JSValueRef) * argc);
  for (int i = 0; i < argc; i++) args[i] = ref_of(ctx->g, argv[i]);
  JSValueRef exc = NULL;
  JSObjectRef r = JSObjectCallAsConstructor(ctx->g, f, argc, args, &exc);
  if (args != stack) free(args);
  return result(ctx, r, exc);
}

// getter/setter con Object.defineProperty(obj, name, {get, set, configurable})
static void define_accessor(JSContext *ctx, JSObjectRef o, const JSCFunctionListEntry *e) {
  int magic = e->def_type == JS_DEF_CGETSET_MAGIC;
  JSObjectRef desc = JSObjectMake(ctx->g, NULL, NULL);
  int rel;
  if (e->u.getset.get.getter) {
    FnPriv *p = calloc(1, sizeof(FnPriv));
    p->kind = magic ? JS_CFUNC_getter_magic : JS_CFUNC_getter;
    p->f = e->u.getset.get;
    snprintf(p->name, sizeof p->name, "get %s", e->name);
    p->magic_arg = e->magic;
    JSValue g = new_fn(ctx, p, e->name);
    JSStringRef n = name_get("get", &rel);
    JSObjectSetProperty(ctx->g, desc, n, (JSValueRef)g.ref, 0, NULL);
    if (rel) JSStringRelease(n);
    JS_FreeValue(ctx, g);
  }
  if (e->u.getset.set.setter) {
    FnPriv *p = calloc(1, sizeof(FnPriv));
    p->kind = magic ? JS_CFUNC_setter_magic : JS_CFUNC_setter;
    p->f = e->u.getset.set;
    snprintf(p->name, sizeof p->name, "set %s", e->name);
    p->magic_arg = e->magic;
    p->length = 1;
    JSValue s = new_fn(ctx, p, e->name);
    JSStringRef n = name_get("set", &rel);
    JSObjectSetProperty(ctx->g, desc, n, (JSValueRef)s.ref, 0, NULL);
    if (rel) JSStringRelease(n);
    JS_FreeValue(ctx, s);
  }
  JSStringRef n = name_get("configurable", &rel);
  JSObjectSetProperty(ctx->g, desc, n, JSValueMakeBoolean(ctx->g, (e->prop_flags & JS_PROP_CONFIGURABLE) != 0), 0,
                      NULL);
  if (rel) JSStringRelease(n);
  n = name_get("enumerable", &rel);
  JSObjectSetProperty(ctx->g, desc, n, JSValueMakeBoolean(ctx->g, (e->prop_flags & JS_PROP_ENUMERABLE) != 0), 0,
                      NULL);
  if (rel) JSStringRelease(n);
  JSStringRef key = JSStringCreateWithUTF8CString(e->name);
  JSValueRef args[3] = { o, JSValueMakeString(ctx->g, key), desc };
  JSStringRelease(key);
  JSObjectCallAsFunction(ctx->g, ctx->define_property, NULL, 3, args, NULL);
}

void JS_SetPropertyFunctionList(JSContext *ctx, JSValueConst obj, const JSCFunctionListEntry *tab, int len) {
  JSObjectRef o = obj_of(obj);
  if (!o) return;
  for (int i = 0; i < len; i++) {
    const JSCFunctionListEntry *e = &tab[i];
    switch (e->def_type) {
      case JS_DEF_CFUNC: {
        FnPriv *p = calloc(1, sizeof(FnPriv));
        p->kind = (JSCFunctionEnum)e->u.func.cproto;
        p->f = e->u.func.cfunc;
        p->length = e->u.func.length;
        p->magic_arg = e->magic;
        set_prop(ctx, obj, e->name, new_fn(ctx, p, e->name), attrs_of(e->prop_flags));
        break;
      }
      case JS_DEF_CGETSET:
      case JS_DEF_CGETSET_MAGIC:
        define_accessor(ctx, o, e);
        break;
      case JS_DEF_PROP_STRING:
        set_prop(ctx, obj, e->name, JS_NewString(ctx, e->u.str), attrs_of(e->prop_flags));
        break;
      case JS_DEF_PROP_INT32:
        set_prop(ctx, obj, e->name, JS_NewInt32(ctx, e->u.i32), attrs_of(e->prop_flags));
        break;
      case JS_DEF_PROP_DOUBLE:
        set_prop(ctx, obj, e->name, JS_NewFloat64(ctx, e->u.f64), attrs_of(e->prop_flags));
        break;
    }
  }
}

// ---------- clases con datos de C ----------

static void obj_finalize(JSObjectRef obj) {
  ObjPriv *p = JSObjectGetPrivate(obj);
  if (!p || p->magic != OBJ_MAGIC) return;
  // el finalizador de la clase lee el dato con JS_GetOpaque (aun esta)
  for (int r = 0; r < 4; r++) {
    JSRuntime *rt = quix_js_runtimes[r];
    if (!rt) continue;
    ClassEntry *ce = &rt->classes[p->id];
    if (ce->used && ce->def.finalizer) {
      ce->def.finalizer(rt, (JSValue){ obj, JS_REF_OBJECT, JS_TAG_REF });
      break;
    }
  }
  p->magic = 0;
  free(p);
}

JSClassID JS_NewClassID(JSClassID *pclass_id) {
  if (*pclass_id == 0) *pclass_id = next_class_id++;
  return *pclass_id;
}

int JS_NewClass(JSRuntime *rt, JSClassID class_id, const JSClassDef *class_def) {
  if (class_id == 0 || class_id >= MAX_CLASSES) return -1;
  ClassEntry *ce = &rt->classes[class_id];
  ce->def = *class_def;
  JSClassDefinition d = kJSClassDefinitionEmpty;
  d.className = class_def->class_name;
  d.finalize = obj_finalize;
  ce->cls = JSClassCreate(&d);
  ce->used = 1;
  return 0;
}

void JS_SetClassProto(JSContext *ctx, JSClassID class_id, JSValue proto) {
  if (class_id >= MAX_CLASSES) return;
  if (ctx->class_proto[class_id]) JSValueUnprotect(ctx->g, ctx->class_proto[class_id]);
  // se queda con proto (ya protegido)
  ctx->class_proto[class_id] = JS_IsObject(proto) ? (JSValueRef)proto.ref : NULL;
}

JSValue JS_GetClassProto(JSContext *ctx, JSClassID class_id) {
  if (class_id >= MAX_CLASSES || !ctx->class_proto[class_id]) return JS_NULL;
  return wrap(ctx->g, ctx->class_proto[class_id], 1);
}

JSValue JS_NewObjectProtoClass(JSContext *ctx, JSValueConst proto, JSClassID class_id) {
  if (class_id >= MAX_CLASSES || !ctx->rt->classes[class_id].used) return JS_ThrowTypeError(ctx, "unknown class");
  ObjPriv *p = calloc(1, sizeof(ObjPriv));
  if (!p) return JS_ThrowOutOfMemory(ctx);
  p->magic = OBJ_MAGIC;
  p->id = class_id;
  if (native_prof) ctx->rt->classes[class_id].prof_made++;
  JSObjectRef o = JSObjectMake(ctx->g, ctx->rt->classes[class_id].cls, p);
  if (JS_IsObject(proto)) JSObjectSetPrototype(ctx->g, o, (JSValueRef)proto.ref);
  return wrap(ctx->g, o, 1);
}

JSValue JS_NewObjectClass(JSContext *ctx, int class_id) {
  JSValueRef proto = (class_id > 0 && class_id < MAX_CLASSES) ? ctx->class_proto[class_id] : NULL;
  JSValue pv = proto ? (JSValue){ proto, JS_REF_OBJECT, JS_TAG_REF } : JS_NULL;
  return JS_NewObjectProtoClass(ctx, pv, (JSClassID)class_id);
}

JSValue JS_NewObjectProto(JSContext *ctx, JSValueConst proto) {
  JSObjectRef o = JSObjectMake(ctx->g, NULL, NULL);
  if (JS_IsObject(proto) || JS_IsNull(proto)) JSObjectSetPrototype(ctx->g, o, ref_of(ctx->g, proto));
  return wrap(ctx->g, o, 1);
}

static ObjPriv *priv_of(JSValueConst obj) {
  if (!JS_IsObject(obj)) return NULL;
  ObjPriv *p = JSObjectGetPrivate((JSObjectRef)obj.ref);
  return (p && p->magic == OBJ_MAGIC) ? p : NULL;
}

void JS_SetOpaque(JSValue obj, void *opaque) {
  ObjPriv *p = priv_of(obj);
  if (p) p->opaque = opaque;
}

void *JS_GetOpaque(JSValueConst obj, JSClassID class_id) {
  ObjPriv *p = priv_of(obj);
  return (p && p->id == class_id) ? p->opaque : NULL;
}

void *JS_GetOpaque2(JSContext *ctx, JSValueConst obj, JSClassID class_id) {
  ObjPriv *p = priv_of(obj);
  if (!p || p->id != class_id) {
    const char *name = class_id < MAX_CLASSES ? ctx->rt->classes[class_id].def.class_name : NULL;
    JS_ThrowTypeError(ctx, "%s object expected", name ? name : "native");
    return NULL;
  }
  return p->opaque;
}

// ---------- ejecutar codigo ----------

static JSStringRef source_of(const char *input, size_t len) {
  if (input[len] == 0) return JSStringCreateWithUTF8CString(input);
  char *tmp = malloc(len + 1);
  if (!tmp) return NULL;
  memcpy(tmp, input, len);
  tmp[len] = 0;
  JSStringRef s = JSStringCreateWithUTF8CString(tmp);
  free(tmp);
  return s;
}

JSValue JS_Eval(JSContext *ctx, const char *input, size_t input_len, const char *filename, int eval_flags) {
  (void)eval_flags;
  drain(ctx->rt);
  JSStringRef src = source_of(input, input_len);
  JSStringRef url = JSStringCreateWithUTF8CString(filename ? filename : "<eval>");
  JSValueRef exc = NULL;
  JSValueRef r = JSEvaluateScript(ctx->g, src, NULL, url, 1, &exc);
  JSStringRelease(src);
  JSStringRelease(url);
  return result(ctx, r, exc);
}

int JS_CheckSyntax(JSContext *ctx, const char *input, size_t input_len, const char *filename) {
  JSStringRef src = source_of(input, input_len);
  JSStringRef url = JSStringCreateWithUTF8CString(filename ? filename : "<eval>");
  JSValueRef exc = NULL;
  bool ok = JSCheckScriptSyntax(ctx->g, src, url, 1, &exc);
  JSStringRelease(src);
  JSStringRelease(url);
  if (!ok) {
    JS_Throw(ctx, exc ? wrap(ctx->g, exc, 1) : JS_UNDEFINED);
    return -1;
  }
  return 0;
}

JSValue JS_ParseJSON(JSContext *ctx, const char *buf, size_t len, const char *filename) {
  (void)filename;
  JSStringRef s = source_of(buf, len);
  JSValueRef r = JSValueMakeFromJSONString(ctx->g, s);
  JSStringRelease(s);
  if (!r) return JS_ThrowSyntaxError(ctx, "JSON.parse: bad JSON");
  return wrap(ctx->g, r, 1);
}

JSValue JS_JSONStringify(JSContext *ctx, JSValueConst obj, JSValueConst replacer, JSValueConst space) {
  (void)replacer;
  double ind = 0;
  if (JS_IsNumber(space)) ind = space.num;
  JSValueRef exc = NULL;
  JSStringRef s = JSValueCreateJSONString(ctx->g, ref_of(ctx->g, obj), ind > 0 ? (unsigned)ind : 0, &exc);
  if (exc) return JS_Throw(ctx, wrap(ctx->g, exc, 1));
  if (!s) return JS_UNDEFINED;
  JSValue r = wrap(ctx->g, JSValueMakeString(ctx->g, s), 1);
  JSStringRelease(s);
  return r;
}

JSValue JS_NewPromiseCapability(JSContext *ctx, JSValue *resolving_funcs) {
  JSObjectRef resolve = NULL, reject = NULL;
  JSValueRef exc = NULL;
  JSObjectRef p = JSObjectMakeDeferredPromise(ctx->g, &resolve, &reject, &exc);
  if (exc || !p) return JS_Throw(ctx, wrap(ctx->g, exc, 1));
  resolving_funcs[0] = wrap(ctx->g, resolve, 1);
  resolving_funcs[1] = wrap(ctx->g, reject, 1);
  return wrap(ctx->g, p, 1);
}

// ---------- ArrayBuffer y TypedArray ----------

static void free_bytes(void *bytes, void *ctx) {
  (void)ctx;
  free(bytes);
}

JSValue JS_NewArrayBufferCopy(JSContext *ctx, const uint8_t *buf, size_t len) {
  void *copy = malloc(len ? len : 1);
  if (!copy) return JS_ThrowOutOfMemory(ctx);
  if (len) memcpy(copy, buf, len);
  JSValueRef exc = NULL;
  JSObjectRef ab = JSObjectMakeArrayBufferWithBytesNoCopy(ctx->g, copy, len, free_bytes, NULL, &exc);
  return result(ctx, ab, exc);
}

int JS_BufferKind(JSContext *ctx, JSValueConst obj) {
  JSObjectRef o = obj_of(obj);
  if (!o) return JS_BUFFER_NONE;
  JSTypedArrayType t = JSValueGetTypedArrayType(ctx->g, o, NULL);
  if (t == kJSTypedArrayTypeArrayBuffer) return JS_BUFFER_ARRAY_BUFFER;
  return t == kJSTypedArrayTypeNone ? JS_BUFFER_NONE : JS_BUFFER_TYPED_ARRAY;
}

uint8_t *JS_GetBufferBytes(JSContext *ctx, JSValueConst obj, size_t *len) {
  *len = 0;
  int kind = JS_BufferKind(ctx, obj);
  if (kind == JS_BUFFER_ARRAY_BUFFER) return JS_GetArrayBuffer(ctx, len, obj);
  if (kind != JS_BUFFER_TYPED_ARRAY) return NULL;
  size_t off = 0, blen = 0, bpe = 0, size = 0;
  JSValue buf = JS_GetTypedArrayBuffer(ctx, obj, &off, &blen, &bpe);
  if (JS_IsException(buf)) return NULL;
  uint8_t *p = JS_GetArrayBuffer(ctx, &size, buf);
  JS_FreeValue(ctx, buf);   // la vista lo sigue sujetando
  if (!p) return NULL;
  *len = blen;
  return p + off;
}

uint8_t *JS_GetArrayBuffer(JSContext *ctx, size_t *psize, JSValueConst obj) {
  JSObjectRef o = obj_of(obj);
  if (!o || JSValueGetTypedArrayType(ctx->g, o, NULL) != kJSTypedArrayTypeArrayBuffer) {
    JS_ThrowTypeError(ctx, "ArrayBuffer expected");
    if (psize) *psize = 0;
    return NULL;
  }
  JSValueRef exc = NULL;
  void *p = JSObjectGetArrayBufferBytesPtr(ctx->g, o, &exc);
  if (psize) *psize = JSObjectGetArrayBufferByteLength(ctx->g, o, NULL);
  return (uint8_t *)p;
}

static JSTypedArrayType jsc_type(JSTypedArrayEnum t) {
  switch (t) {
    case JS_TYPED_ARRAY_UINT8C: return kJSTypedArrayTypeUint8ClampedArray;
    case JS_TYPED_ARRAY_INT8: return kJSTypedArrayTypeInt8Array;
    case JS_TYPED_ARRAY_UINT8: return kJSTypedArrayTypeUint8Array;
    case JS_TYPED_ARRAY_INT16: return kJSTypedArrayTypeInt16Array;
    case JS_TYPED_ARRAY_UINT16: return kJSTypedArrayTypeUint16Array;
    case JS_TYPED_ARRAY_INT32: return kJSTypedArrayTypeInt32Array;
    case JS_TYPED_ARRAY_UINT32: return kJSTypedArrayTypeUint32Array;
    case JS_TYPED_ARRAY_FLOAT32: return kJSTypedArrayTypeFloat32Array;
    case JS_TYPED_ARRAY_FLOAT64: return kJSTypedArrayTypeFloat64Array;
  }
  return kJSTypedArrayTypeUint8Array;
}

static size_t bytes_per(JSTypedArrayType t) {
  switch (t) {
    case kJSTypedArrayTypeInt16Array: case kJSTypedArrayTypeUint16Array: return 2;
    case kJSTypedArrayTypeInt32Array: case kJSTypedArrayTypeUint32Array: case kJSTypedArrayTypeFloat32Array: return 4;
    case kJSTypedArrayTypeFloat64Array: return 8;
    default: return 1;
  }
}

JSValue JS_NewTypedArray(JSContext *ctx, int argc, JSValueConst *argv, JSTypedArrayEnum type) {
  if (native_prof) prof_typed++;
  JSTypedArrayType t = jsc_type(type);
  JSValueRef exc = NULL;
  JSObjectRef r;
  JSObjectRef buf = argc > 0 ? obj_of(argv[0]) : NULL;
  if (buf && JSValueGetTypedArrayType(ctx->g, buf, NULL) == kJSTypedArrayTypeArrayBuffer) {
    double off = 0, len = -1;
    if (argc > 1 && JS_IsNumber(argv[1])) off = argv[1].num;
    if (argc > 2 && JS_IsNumber(argv[2])) len = argv[2].num;
    if (len < 0) {
      size_t total = JSObjectGetArrayBufferByteLength(ctx->g, buf, NULL);
      len = (double)((total - (size_t)off) / bytes_per(t));
    }
    r = JSObjectMakeTypedArrayWithArrayBufferAndOffset(ctx->g, t, buf, (size_t)off, (size_t)len, &exc);
  } else {
    double n = argc > 0 && JS_IsNumber(argv[0]) ? argv[0].num : 0;
    r = JSObjectMakeTypedArray(ctx->g, t, (size_t)n, &exc);
  }
  return result(ctx, r, exc);
}

JSValue JS_GetTypedArrayBuffer(JSContext *ctx, JSValueConst obj, size_t *pbyte_offset, size_t *pbyte_length,
                               size_t *pbytes_per_element) {
  JSObjectRef o = obj_of(obj);
  JSTypedArrayType t = o ? JSValueGetTypedArrayType(ctx->g, o, NULL) : kJSTypedArrayTypeNone;
  if (t == kJSTypedArrayTypeNone || t == kJSTypedArrayTypeArrayBuffer) return JS_ThrowTypeError(ctx, "TypedArray expected");
  JSValueRef exc = NULL;
  JSObjectRef b = JSObjectGetTypedArrayBuffer(ctx->g, o, &exc);
  if (pbyte_offset) *pbyte_offset = JSObjectGetTypedArrayByteOffset(ctx->g, o, NULL);
  if (pbyte_length) *pbyte_length = JSObjectGetTypedArrayByteLength(ctx->g, o, NULL);
  if (pbytes_per_element) *pbytes_per_element = bytes_per(t);
  return result(ctx, b, exc);
}

// ---------- memoria y profiler (API privada en C de JSC) ----------

static int64_t stat_of(JSContext *ctx, JSObjectRef st, const char *name) {
  int rel;
  JSStringRef n = name_get(name, &rel);
  JSValueRef v = JSObjectGetProperty(ctx->g, st, n, NULL);
  if (rel) JSStringRelease(n);
  return (int64_t)JSValueToNumber(ctx->g, v, NULL);
}

void JS_ComputeMemoryUsage(JSRuntime *rt, JSMemoryUsage *s) {
  memset(s, 0, sizeof *s);
  if (!rt->context_count) return;
  JSContext *ctx = rt->contexts[0];
  JSObjectRef st = JSGetMemoryUsageStatistics(ctx->g);
  if (!st) return;
  s->heap_size = stat_of(ctx, st, "heapSize");
  s->heap_capacity = stat_of(ctx, st, "heapCapacity");
  s->extra_memory = stat_of(ctx, st, "extraMemorySize");
  s->object_count = stat_of(ctx, st, "objectCount");
}

int JS_ObjectTypeCounts(JSRuntime *rt, char names[][48], double *counts, int max) {
  if (!rt->context_count) return 0;
  JSContext *ctx = rt->contexts[0];
  JSObjectRef st = JSGetMemoryUsageStatistics(ctx->g);
  if (!st) return 0;
  JSStringRef key = JSStringCreateWithUTF8CString("objectTypeCounts");
  JSValueRef v = JSObjectGetProperty(ctx->g, st, key, NULL);
  JSStringRelease(key);
  if (!v || !JSValueIsObject(ctx->g, v)) return 0;
  JSObjectRef o = (JSObjectRef)v;
  JSPropertyNameArrayRef props = JSObjectCopyPropertyNames(ctx->g, o);
  int n = 0;
  size_t total = JSPropertyNameArrayGetCount(props);
  for (size_t i = 0; i < total && n < max; i++) {
    JSStringRef name = JSPropertyNameArrayGetNameAtIndex(props, i);
    JSStringGetUTF8CString(name, names[n], 48);
    counts[n] = JSValueToNumber(ctx->g, JSObjectGetProperty(ctx->g, o, name, NULL), NULL);
    n++;
  }
  JSPropertyNameArrayRelease(props);
  return n;
}

void JS_IdleWork(JSRuntime *rt, double ms) {
  if (!rt->context_count || ms <= 0) return;
  JSContextGroupPerformIdleWork(rt->group, ms);
}

double JS_GCPressure(JSRuntime *rt) {
  return rt->context_count ? JSContextGroupGCPressure(rt->group) : 0;
}

void JS_CollectDue(JSRuntime *rt) {
  if (!rt->context_count) return;
  drain(rt);
  JSContextGroupCollectNow(rt->group);
}

void JS_RunGC(JSRuntime *rt) {
  if (!rt->context_count) return;
  drain(rt);
  JSSynchronousGarbageCollectForDebugging(rt->contexts[0]->g);
}

int JS_ProfilerEnable(JSRuntime *rt) {
  return JSContextGroupEnableSamplingProfiler(rt->group) ? 0 : -1;
}

void JS_ProfilerDisable(JSRuntime *rt) {
  JSContextGroupDisableSamplingProfiler(rt->group);
}

JSValue JS_ProfilerTakeSamples(JSContext *ctx) {
  JSStringRef s = JSContextGroupTakeSamplesFromSamplingProfiler(ctx->rt->group);
  if (!s) return JS_UNDEFINED;
  JSValueRef r = JSValueMakeFromJSONString(ctx->g, s);
  JSStringRelease(s);
  return r ? wrap(ctx->g, r, 1) : JS_UNDEFINED;
}

// ---------- el profiler de las funciones de C ----------

static JSRuntime *prof_runtime_of_last;

void JS_NativeProfEnable(int on) {
  if (on && !native_prof) {
    prof_plain_objects = prof_arrays = prof_typed = 0;
    for (int r = 0; r < 4; r++) {
      if (!quix_js_runtimes[r]) continue;
      for (int c = 0; c < MAX_CLASSES; c++) quix_js_runtimes[r]->classes[c].prof_made = 0;
    }
    for (FnPriv *p = all_fns; p; p = p->link.next) {
      p->prof_ns = 0;
      p->prof_calls = 0;
    }
  }
  native_prof = on;
}

// las n funciones de C que mas tardan: nombre, ns, llamadas
int JS_NativeProfTop(JSNativeProfEntry *out, int n) {
  int count = 0;
  for (FnPriv *p = all_fns; p && n > 0; p = p->link.next) {
    if (!p->prof_calls) continue;
    JSNativeProfEntry e = { p->name, p->prof_ns, p->prof_calls };
    if (count < n) {
      out[count++] = e;
      continue;
    }
    int min = 0;
    for (int i = 1; i < count; i++) if (out[i].ns < out[min].ns) min = i;
    if (e.ns > out[min].ns) out[min] = e;
  }
  for (int i = 1; i < count; i++) {   // de mas a menos tiempo
    JSNativeProfEntry e = out[i];
    int j = i;
    while (j > 0 && out[j - 1].ns < e.ns) {
      out[j] = out[j - 1];
      j--;
    }
    out[j] = e;
  }
  return count;
}

// los objetos que ha creado quixjs mientras media: por clase y los simples
int JS_NativeProfObjects(JSNativeProfEntry *out, int n) {
  int count = 0;
  JSRuntime *rt = quix_js_runtimes[0];
  if (rt) {
    for (int c = 1; c < MAX_CLASSES && count < n; c++) {
      if (!rt->classes[c].used || !rt->classes[c].prof_made) continue;
      const char *name = rt->classes[c].def.class_name;
      out[count++] = (JSNativeProfEntry){ name ? name : "(class)", 0, rt->classes[c].prof_made };
    }
  }
  if (count < n && prof_plain_objects) out[count++] = (JSNativeProfEntry){ "Object (JS_NewObject)", 0, prof_plain_objects };
  if (count < n && prof_arrays) out[count++] = (JSNativeProfEntry){ "Array (JS_NewArray)", 0, prof_arrays };
  if (count < n && prof_typed) out[count++] = (JSNativeProfEntry){ "typed array", 0, prof_typed };
  for (int i = 1; i < count; i++) {   // de mas a menos
    JSNativeProfEntry e = out[i];
    int j = i;
    while (j > 0 && out[j - 1].calls < e.calls) {
      out[j] = out[j - 1];
      j--;
    }
    out[j] = e;
  }
  return count;
}

// ---------- el muestreo de asignaciones (parche de quixjs en JavaScriptCore) ----------

// Simbolos debiles: con un JavaScriptCore sin el cambio de quixjs (uno compilado antes)
// quixjs enlaza igual y el profiler no da la memoria
extern void JSQuixAllocProfileEnable(bool on);
extern char *JSQuixAllocProfileTake(void);

void JS_AllocProfileEnable(int on) {
  JSQuixAllocProfileEnable(on != 0);
}
char *JS_AllocProfileTake(void) { return JSQuixAllocProfileTake(); }
