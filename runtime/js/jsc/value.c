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

// Valores: liberar (JS_FreeValue...), crear (JS_NewObject, JS_NewString...)
// y convertir (JS_ToBool, JS_ToInt32, JS_ToString...).
// Parte de runtime/js/jsc.c, que une las partes y se compila como un solo
// archivo (ver runtime/js/README.md): no se compila sola. Lo compartido esta
// en jsc.h.

#include "jsc.h"

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

// ---------- valores nuevos ----------

JSValue JS_NewObject(JSContext *ctx) {
  if (jsc_native_prof) jsc_prof_plain_objects++;
  return wrap(ctx->g, JSObjectMake(ctx->g, NULL, NULL), 1);
}

JSValue JS_NewArray(JSContext *ctx) {
  if (jsc_native_prof) jsc_prof_arrays++;
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

// UTF-8 con un 0 dentro: JSStringCreateWithUTF8CString pararia ahi (una
// partida de RPG Maker MZ es texto con ceros, comprimido con pako). Se pasa
// a UTF-16 aqui, entero; lo que no es UTF-8 valido, U+FFFD (como Node)
static JSValue new_string_utf8_nul(JSContext *ctx, const unsigned char *s, size_t len) {
  uint16_t *out = malloc((len + 1) * sizeof(uint16_t));
  if (!out) return JS_ThrowOutOfMemory(ctx);
  size_t n = 0, i = 0;
  while (i < len) {
    unsigned c = s[i], need = 0, cp = 0, min = 0;
    if (c < 0x80) { out[n++] = (uint16_t)c; i++; continue; }
    if ((c & 0xE0) == 0xC0) { need = 1; cp = c & 0x1F; min = 0x80; }
    else if ((c & 0xF0) == 0xE0) { need = 2; cp = c & 0x0F; min = 0x800; }
    else if ((c & 0xF8) == 0xF0) { need = 3; cp = c & 0x07; min = 0x10000; }
    else { out[n++] = 0xFFFD; i++; continue; }
    size_t k = 1;
    while (k <= need && i + k < len && (s[i + k] & 0xC0) == 0x80) { cp = (cp << 6) | (s[i + k] & 0x3F); k++; }
    if (k <= need || cp < min || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
      out[n++] = 0xFFFD;
      i += k;   // la secuencia rota, hasta donde llego
      continue;
    }
    i += k;
    if (cp >= 0x10000) {
      cp -= 0x10000;
      out[n++] = (uint16_t)(0xD800 + (cp >> 10));
      out[n++] = (uint16_t)(0xDC00 + (cp & 0x3FF));
    } else {
      out[n++] = (uint16_t)cp;
    }
  }
  JSValue v = JS_NewStringUTF16(ctx, out, n);
  free(out);
  return v;
}

JSValue JS_NewStringLen(JSContext *ctx, const char *str, size_t len) {
  if (len && memchr(str, 0, len)) return new_string_utf8_nul(ctx, (const unsigned char *)str, len);
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
