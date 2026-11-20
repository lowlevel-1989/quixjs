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

// ArrayBuffer y TypedArray.
// Parte de runtime/js/jsc.c, que une las partes y se compila como un solo
// archivo (ver runtime/js/README.md): no se compila sola. Lo compartido esta
// en jsc.h.

#include "jsc.h"

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
  if (jsc_native_prof) jsc_prof_typed++;
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
