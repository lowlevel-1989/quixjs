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

// uniform* y vertexAttrib*.
// Parte de binding/webgl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "args.h"

static GLint loc_of(JSValueConst v) {
  GLLoc *l = JS_GetOpaque(v, js_glloc_class_id);
  return l ? l->loc : -1;
}

#define UNI(js, call) static JSValue js_gl_##js(ARGS) { CTX(); \
  GLint L = argc > 0 ? loc_of(argv[0]) : -1; if (L >= 0) call; return JS_UNDEFINED; }
UNI(uniform1f, qgl.Uniform1f(L, f(1)))
UNI(uniform2f, qgl.Uniform2f(L, f(1), f(2)))
UNI(uniform3f, qgl.Uniform3f(L, f(1), f(2), f(3)))
UNI(uniform4f, qgl.Uniform4f(L, f(1), f(2), f(3), f(4)))
UNI(uniform1i, qgl.Uniform1i(L, n(1)))
UNI(uniform2i, qgl.Uniform2i(L, n(1), n(2)))
UNI(uniform3i, qgl.Uniform3i(L, n(1), n(2), n(3)))
UNI(uniform4i, qgl.Uniform4i(L, n(1), n(2), n(3), n(4)))

// Float32Array / Int32Array / array JS -> datos contiguos
static void *array_data(JSContext *ctx, JSValueConst v, int is_float, int *count, int *owned) {
  *owned = 0;
  *count = 0;
  size_t off = 0, blen = 0, bpe = 0, size = 0;
  if (JS_BufferKind(ctx, v) == JS_BUFFER_TYPED_ARRAY) {
    JSValue buf = JS_GetTypedArrayBuffer(ctx, v, &off, &blen, &bpe);
    uint8_t *p = JS_IsException(buf) ? NULL : JS_GetArrayBuffer(ctx, &size, buf);
    JS_FreeValue(ctx, buf);
    if (p && bpe == 4) {
      *count = (int)(blen / 4);
      // un Int32Array en un uniform de float (o al reves) se convierte
      return p + off;
    }
    return NULL;
  }
  if (!JS_IsArray(ctx, v)) return NULL;
  uint32_t len = 0;
  JSValue l = JS_GetPropertyStr(ctx, v, "length");
  JS_ToUint32(ctx, &len, l);
  JS_FreeValue(ctx, l);
  void *out = malloc(len * 4 + 4);
  if (!out) return NULL;
  for (uint32_t i = 0; i < len; i++) {
    JSValue x = JS_GetPropertyUint32(ctx, v, i);
    if (is_float) {
      double d = 0;
      JS_ToFloat64(ctx, &d, x);
      ((float *)out)[i] = (float)d;
    } else {
      int32_t d = 0;
      JS_ToInt32(ctx, &d, x);
      ((int32_t *)out)[i] = d;
    }
    JS_FreeValue(ctx, x);
  }
  *owned = 1;
  *count = (int)len;
  return out;
}

#define UNIV(js, per, is_float, call) static JSValue js_gl_##js(ARGS) { CTX(); \
  GLint L = argc > 0 ? loc_of(argv[0]) : -1; if (L < 0 || argc < 2) return JS_UNDEFINED; \
  int cnt, owned; void *d = array_data(ctx, argv[argc - 1], is_float, &cnt, &owned); \
  if (d && cnt >= per) { call; } if (owned) { free(d); } return JS_UNDEFINED; }
UNIV(uniform1fv, 1, 1, qgl.Uniform1fv(L, cnt / 1, d))
UNIV(uniform2fv, 2, 1, qgl.Uniform2fv(L, cnt / 2, d))
UNIV(uniform3fv, 3, 1, qgl.Uniform3fv(L, cnt / 3, d))
UNIV(uniform4fv, 4, 1, qgl.Uniform4fv(L, cnt / 4, d))
UNIV(uniform1iv, 1, 0, qgl.Uniform1iv(L, cnt / 1, d))
UNIV(uniform2iv, 2, 0, qgl.Uniform2iv(L, cnt / 2, d))
UNIV(uniform3iv, 3, 0, qgl.Uniform3iv(L, cnt / 3, d))
UNIV(uniform4iv, 4, 0, qgl.Uniform4iv(L, cnt / 4, d))
UNIV(uniformMatrix2fv, 4, 1, qgl.UniformMatrix2fv(L, cnt / 4, GL_FALSE, d))
UNIV(uniformMatrix3fv, 9, 1, qgl.UniformMatrix3fv(L, cnt / 9, GL_FALSE, d))
UNIV(uniformMatrix4fv, 16, 1, qgl.UniformMatrix4fv(L, cnt / 16, GL_FALSE, d))

#define ATTRV(js, per, call) static JSValue js_gl_##js(ARGS) { CTX(); \
  int cnt, owned; void *d = argc > 1 ? array_data(ctx, argv[1], 1, &cnt, &owned) : NULL; \
  if (d && cnt >= per) { call; } if (d && owned) { free(d); } return JS_UNDEFINED; }
ATTRV(vertexAttrib1fv, 1, qgl.VertexAttrib1fv(u(0), d))
ATTRV(vertexAttrib2fv, 2, qgl.VertexAttrib2fv(u(0), d))
ATTRV(vertexAttrib3fv, 3, qgl.VertexAttrib3fv(u(0), d))
ATTRV(vertexAttrib4fv, 4, qgl.VertexAttrib4fv(u(0), d))
