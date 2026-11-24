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

// WebGL 2: lo que WebGL2RenderingContext agrega a WebGL 1, sobre OpenGL ES
// 3.0 (qgl3, gfx/gl/gl.h). Solo existe si el driver es ES 3.x (gl_es3). Lo que
// hay es lo que usa three.js (r163 en adelante ya no funciona con WebGL 1):
// vertex arrays, texturas 3D y con texStorage, dibujo instanciado, varias
// salidas y multimuestreo en framebuffers, uniform buffers y sync.
// Parte de binding/webgl.c, despues de las partes de WebGL 1.

#include "common.h"
#include "args.h"

// ---- objetos: vertex arrays
CREATE(createVertexArray, OBJ_VERTEX_ARRAY, qgl3.GenVertexArrays(1, &id))

static JSValue js_gl_deleteVertexArray(ARGS) {
  CTX();
  GLObj *g = argc > 0 ? obj_get(argv[0]) : NULL;
  if (!g || g->kind != OBJ_VERTEX_ARRAY || g->deleted) return JS_UNDEFINED;
  qgl3.DeleteVertexArrays(1, &g->name);
  g->deleted = 1;
  return JS_UNDEFINED;
}

SIMPLE(bindVertexArray, qgl3.BindVertexArray(argc > 0 ? obj_name(argv[0], OBJ_VERTEX_ARRAY) : 0))

static JSValue js_gl_isVertexArray(ARGS) {
  CTX();
  GLObj *g = argc > 0 ? obj_get(argv[0]) : NULL;
  return JS_NewBool(ctx, g && g->kind == OBJ_VERTEX_ARRAY && !g->deleted);
}

// ---- texturas
// texImage3D(target, level, internalformat, w, h, d, border, format, type, datos | null)
static JSValue js_gl_texImage3D(ARGS) {
  CTX();
  size_t len = 0;
  uint8_t *p = (argc > 9 && JS_IsObject(argv[9])) ? bytes_of(ctx, argv[9], &len) : NULL;
  qgl3.TexImage3D(u(0), n(1), n(2), n(3), n(4), n(5), n(6), u(7), u(8), p);
  return JS_UNDEFINED;
}

// texSubImage3D(target, level, x, y, z, w, h, d, format, type, datos)
static JSValue js_gl_texSubImage3D(ARGS) {
  CTX();
  size_t len = 0;
  uint8_t *p = (argc > 10 && JS_IsObject(argv[10])) ? bytes_of(ctx, argv[10], &len) : NULL;
  if (p) qgl3.TexSubImage3D(u(0), n(1), n(2), n(3), n(4), n(5), n(6), n(7), u(8), u(9), p);
  return JS_UNDEFINED;
}

SIMPLE(texStorage2D, qgl3.TexStorage2D(u(0), n(1), u(2), n(3), n(4)))
SIMPLE(texStorage3D, qgl3.TexStorage3D(u(0), n(1), u(2), n(3), n(4), n(5)))

// compressedTexImage3D(target, level, internalformat, w, h, d, border, datos)
static JSValue js_gl_compressedTexImage3D(ARGS) {
  CTX();
  size_t len = 0;
  uint8_t *p = (argc > 7 && JS_IsObject(argv[7])) ? bytes_of(ctx, argv[7], &len) : NULL;
  if (p) qgl3.CompressedTexImage3D(u(0), n(1), u(2), n(3), n(4), n(5), n(6), (GLsizei)len, p);
  return JS_UNDEFINED;
}

SIMPLE(framebufferTextureLayer,
       qgl3.FramebufferTextureLayer(u(0), u(1), argc > 2 ? obj_name(argv[2], OBJ_TEXTURE) : 0, n(3), n(4)))

// ---- dibujar
static JSValue js_gl_drawArraysInstanced(ARGS) {
  CTX();
  qgl3.DrawArraysInstanced(u(0), n(1), n(2), n(3));
  mark_drawn(w);
  return JS_UNDEFINED;
}

static JSValue js_gl_drawElementsInstanced(ARGS) {
  CTX();
  qgl3.DrawElementsInstanced(u(0), n(1), u(2), (const void *)(intptr_t)n(3), n(4));
  mark_drawn(w);
  return JS_UNDEFINED;
}

static JSValue js_gl_drawRangeElements(ARGS) {
  CTX();
  qgl3.DrawRangeElements(u(0), u(1), u(2), n(3), u(4), (const void *)(intptr_t)n(5));
  mark_drawn(w);
  return JS_UNDEFINED;
}

SIMPLE(vertexAttribDivisor, qgl3.VertexAttribDivisor(u(0), u(1)))
SIMPLE(vertexAttribIPointer, qgl3.VertexAttribIPointer(u(0), n(1), u(2), n(3), (const void *)(intptr_t)n(4)))

// ---- framebuffers
// una lista JS de enums -> GLenum[] (como mucho 16)
static int enums_of(JSContext *ctx, JSValueConst v, GLenum *out) {
  uint32_t len = 0;
  JSValue l = JS_GetPropertyStr(ctx, v, "length");
  JS_ToUint32(ctx, &len, l);
  JS_FreeValue(ctx, l);
  if (len > 16) len = 16;
  for (uint32_t i = 0; i < len; i++) {
    JSValue e = JS_GetPropertyUint32(ctx, v, i);
    uint32_t x = 0;
    JS_ToUint32(ctx, &x, e);
    JS_FreeValue(ctx, e);
    out[i] = x;
  }
  return (int)len;
}

static JSValue js_gl_drawBuffers(ARGS) {
  CTX();
  GLenum list[16];
  int k = argc > 0 ? enums_of(ctx, argv[0], list) : 0;
  qgl3.DrawBuffers(k, list);
  return JS_UNDEFINED;
}

static JSValue js_gl_invalidateFramebuffer(ARGS) {
  CTX();
  GLenum list[16];
  int k = argc > 1 ? enums_of(ctx, argv[1], list) : 0;
  qgl3.InvalidateFramebuffer(u(0), k, list);
  return JS_UNDEFINED;
}

SIMPLE(blitFramebuffer, qgl3.BlitFramebuffer(n(0), n(1), n(2), n(3), n(4), n(5), n(6), n(7), u(8), u(9)))
SIMPLE(renderbufferStorageMultisample, qgl3.RenderbufferStorageMultisample(u(0), n(1), u(2), n(3), n(4)))
SIMPLE(readBuffer, qgl3.ReadBuffer(u(0)))

// clearBuffer{f,i,ui}v(buffer, drawbuffer, valores)
static JSValue clear_buffer(ARGS, int kind) {
  CTX();
  int count = 0, owned = 0;
  void *v = argc > 2 ? array_data(ctx, argv[2], kind == 0, &count, &owned) : NULL;
  if (v && count >= 1) {
    if (kind == 0) qgl3.ClearBufferfv(u(0), n(1), v);
    else if (kind == 1) qgl3.ClearBufferiv(u(0), n(1), v);
    else qgl3.ClearBufferuiv(u(0), n(1), v);
  }
  if (owned) free(v);
  return JS_UNDEFINED;
}
static JSValue js_gl_clearBufferfv(ARGS) { return clear_buffer(ctx, this_val, argc, argv, 0); }
static JSValue js_gl_clearBufferiv(ARGS) { return clear_buffer(ctx, this_val, argc, argv, 1); }
static JSValue js_gl_clearBufferuiv(ARGS) { return clear_buffer(ctx, this_val, argc, argv, 2); }
SIMPLE(clearBufferfi, qgl3.ClearBufferfi(u(0), n(1), f(2), n(3)))

// ---- uniforms de enteros sin signo y uniform buffers
UNI(uniform1ui, qgl3.Uniform1ui(L, u(1)))
UNI(uniform2ui, qgl3.Uniform2ui(L, u(1), u(2)))
UNI(uniform3ui, qgl3.Uniform3ui(L, u(1), u(2), u(3)))
UNI(uniform4ui, qgl3.Uniform4ui(L, u(1), u(2), u(3), u(4)))

static JSValue uniform_uiv(ARGS, int size) {
  CTX();
  GLint L = argc > 0 ? loc_of(argv[0]) : -1;
  int count = 0, owned = 0;
  void *v = (L >= 0 && argc > 1) ? array_data(ctx, argv[1], 0, &count, &owned) : NULL;
  if (v && count >= size) {
    if (size == 1) qgl3.Uniform1uiv(L, count, v);
    else if (size == 2) qgl3.Uniform2uiv(L, count / 2, v);
    else if (size == 3) qgl3.Uniform3uiv(L, count / 3, v);
    else qgl3.Uniform4uiv(L, count / 4, v);
  }
  if (owned) free(v);
  return JS_UNDEFINED;
}
static JSValue js_gl_uniform1uiv(ARGS) { return uniform_uiv(ctx, this_val, argc, argv, 1); }
static JSValue js_gl_uniform2uiv(ARGS) { return uniform_uiv(ctx, this_val, argc, argv, 2); }
static JSValue js_gl_uniform3uiv(ARGS) { return uniform_uiv(ctx, this_val, argc, argv, 3); }
static JSValue js_gl_uniform4uiv(ARGS) { return uniform_uiv(ctx, this_val, argc, argv, 4); }

static JSValue js_gl_getUniformBlockIndex(ARGS) {
  CTX();
  const char *name = argc > 1 ? JS_ToCString(ctx, argv[1]) : NULL;
  GLuint i = name ? qgl3.GetUniformBlockIndex(obj_name(argv[0], OBJ_PROGRAM), name) : GL_INVALID_INDEX;
  if (name) JS_FreeCString(ctx, name);
  return JS_NewUint32(ctx, i);
}

SIMPLE(uniformBlockBinding, qgl3.UniformBlockBinding(argc > 0 ? obj_name(argv[0], OBJ_PROGRAM) : 0, u(1), u(2)))
SIMPLE(bindBufferBase, qgl3.BindBufferBase(u(0), u(1), argc > 2 ? obj_name(argv[2], OBJ_BUFFER) : 0))
SIMPLE(bindBufferRange, qgl3.BindBufferRange(u(0), u(1), argc > 2 ? obj_name(argv[2], OBJ_BUFFER) : 0, n(3), n(4)))

static JSValue js_gl_getActiveUniformBlockParameter(ARGS) {
  CTX();
  GLint v = 0;
  qgl3.GetActiveUniformBlockiv(argc > 0 ? obj_name(argv[0], OBJ_PROGRAM) : 0, u(1), u(2), &v);
  return JS_NewInt32(ctx, v);
}

// getBufferSubData(target, desde, vista): se mapea el buffer y se copia
static JSValue js_gl_getBufferSubData(ARGS) {
  CTX();
  size_t len = 0;
  uint8_t *dst = argc > 2 ? bytes_of(ctx, argv[2], &len) : NULL;
  if (!dst || !len) return JS_UNDEFINED;
  void *src = qgl3.MapBufferRange(u(0), n(1), (GLsizeiptr)len, GL_MAP_READ_BIT);
  if (src) {
    memcpy(dst, src, len);
    qgl3.UnmapBuffer(u(0));
  }
  return JS_UNDEFINED;
}

// ---- sync
static JSValue js_gl_fenceSync(ARGS) {
  CTX();
  JSValue o = new_obj(ctx, OBJ_SYNC, 0);
  GLObj *g = JS_IsException(o) ? NULL : obj_get(o);
  if (g) g->sync = qgl3.FenceSync(u(0), u(1));
  return o;
}

static GLsync sync_of(JSValueConst v) {
  GLObj *g = obj_get(v);
  return (g && g->kind == OBJ_SYNC && !g->deleted) ? g->sync : NULL;
}

static JSValue js_gl_clientWaitSync(ARGS) {
  CTX();
  GLsync s = argc > 0 ? sync_of(argv[0]) : NULL;
  double timeout = 0;
  if (argc > 2) JS_ToFloat64(ctx, &timeout, argv[2]);
  if (!s) return JS_NewUint32(ctx, GL_WAIT_FAILED);
  return JS_NewUint32(ctx, qgl3.ClientWaitSync(s, u(1), (GLuint64)timeout));
}

static JSValue js_gl_getSyncParameter(ARGS) {
  CTX();
  GLsync s = argc > 0 ? sync_of(argv[0]) : NULL;
  GLint v = 0;
  if (s) qgl3.GetSynciv(s, u(1), 1, NULL, &v);
  return JS_NewInt32(ctx, v);
}

static JSValue js_gl_deleteSync(ARGS) {
  CTX();
  GLObj *g = argc > 0 ? obj_get(argv[0]) : NULL;
  if (!g || g->kind != OBJ_SYNC || g->deleted) return JS_UNDEFINED;
  if (g->sync) qgl3.DeleteSync(g->sync);
  g->sync = NULL;
  g->deleted = 1;
  return JS_UNDEFINED;
}

// getInternalformatParameter(target, internalformat, pname): con
// gl.SAMPLES, las muestras de multimuestreo que admite ese formato (un
// Int32Array, de mas a menos). PIXI 5+ lo pide al crear su renderer WebGL 2
// (RPG Maker MZ): sin esto no arrancaba (Look Outside, pantalla negra)
static JSValue js_gl_getInternalformatParameter(ARGS) {
  CTX();
  GLenum target = u(0), format = u(1), pname = u(2);
  if (pname != GL_SAMPLES) return JS_NULL;
  GLint n = 0, v[16] = { 0 };
  qgl3.GetInternalformativ(target, format, GL_NUM_SAMPLE_COUNTS, 1, &n);
  if (n < 0) n = 0;
  if (n > 16) n = 16;
  if (n) qgl3.GetInternalformativ(target, format, GL_SAMPLES, n, v);
  return int_array(ctx, v, n);
}

#define FN2(name, len) JS_CFUNC_DEF(#name, len, js_gl_##name)
static const JSCFunctionListEntry js_webgl2_proto[] = {
  FN2(createVertexArray, 0), FN2(deleteVertexArray, 1), FN2(bindVertexArray, 1), FN2(isVertexArray, 1),
  FN2(texImage3D, 10), FN2(texSubImage3D, 11), FN2(texStorage2D, 5), FN2(texStorage3D, 6),
  FN2(compressedTexImage3D, 8), FN2(framebufferTextureLayer, 5),
  FN2(drawArraysInstanced, 4), FN2(drawElementsInstanced, 5), FN2(drawRangeElements, 6),
  FN2(vertexAttribDivisor, 2), FN2(vertexAttribIPointer, 5),
  FN2(drawBuffers, 1), FN2(invalidateFramebuffer, 2), FN2(blitFramebuffer, 10),
  FN2(renderbufferStorageMultisample, 5), FN2(readBuffer, 1),
  FN2(clearBufferfv, 3), FN2(clearBufferiv, 3), FN2(clearBufferuiv, 3), FN2(clearBufferfi, 4),
  FN2(uniform1ui, 2), FN2(uniform2ui, 3), FN2(uniform3ui, 4), FN2(uniform4ui, 5),
  FN2(uniform1uiv, 2), FN2(uniform2uiv, 2), FN2(uniform3uiv, 2), FN2(uniform4uiv, 2),
  FN2(getUniformBlockIndex, 2), FN2(uniformBlockBinding, 3), FN2(bindBufferBase, 3), FN2(bindBufferRange, 5),
  FN2(getActiveUniformBlockParameter, 3), FN2(getBufferSubData, 3),
  FN2(fenceSync, 2), FN2(clientWaitSync, 3), FN2(getSyncParameter, 2), FN2(deleteSync, 1),
  FN2(getInternalformatParameter, 3),
};
