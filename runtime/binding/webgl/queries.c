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

// Consultas: getParameter, getError, get*Parameter...
// Parte de binding/webgl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "args.h"

static JSValue int_array(JSContext *ctx, const GLint *v, int n) {
  JSValue len = JS_NewInt32(ctx, n);
  JSValue a = JS_NewTypedArray(ctx, 1, &len, JS_TYPED_ARRAY_INT32);
  for (int i = 0; i < n; i++) JS_SetPropertyUint32(ctx, a, i, JS_NewInt32(ctx, v[i]));
  return a;
}

static JSValue float_array(JSContext *ctx, const GLfloat *v, int n) {
  JSValue len = JS_NewInt32(ctx, n);
  JSValue a = JS_NewTypedArray(ctx, 1, &len, JS_TYPED_ARRAY_FLOAT32);
  for (int i = 0; i < n; i++) JS_SetPropertyUint32(ctx, a, i, JS_NewFloat64(ctx, v[i]));
  return a;
}

static JSValue js_gl_getParameter(ARGS) {
  CTX();
  GLenum p = u(0);
  GLint iv[4] = { 0 };
  GLfloat fv[4] = { 0 };
  GLboolean bv[4] = { 0 };
  switch (p) {
    case GL_VENDOR:                   return JS_NewString(ctx, "quixjs");
    case GL_RENDERER:                 return JS_NewString(ctx, "quixjs WebGL");
    case GL_VERSION:                  return JS_NewString(ctx, "WebGL 1.0 (OpenGL ES 2.0, quixjs)");
    case GL_SHADING_LANGUAGE_VERSION: return JS_NewString(ctx, "WebGL GLSL ES 1.0 (quixjs)");
    case UNMASKED_VENDOR_WEBGL:       return JS_NewString(ctx, (const char *)qgl.GetString(GL_VENDOR));
    case UNMASKED_RENDERER_WEBGL:     return JS_NewString(ctx, gl_renderer_name());
    case UNPACK_FLIP_Y_WEBGL:         return JS_NewBool(ctx, w->flip_y);
    case UNPACK_PREMULTIPLY_ALPHA_WEBGL: return JS_NewBool(ctx, w->premultiply);
    case UNPACK_COLORSPACE_CONVERSION_WEBGL: return JS_NewInt32(ctx, BROWSER_DEFAULT_WEBGL);
    case GL_CURRENT_PROGRAM:          return JS_DupValue(ctx, w->program);
    case GL_ARRAY_BUFFER_BINDING:     return JS_DupValue(ctx, w->array_buffer);
    case GL_ELEMENT_ARRAY_BUFFER_BINDING: return JS_DupValue(ctx, w->element_buffer);
    case GL_FRAMEBUFFER_BINDING:      return JS_DupValue(ctx, w->framebuffer);
    case GL_RENDERBUFFER_BINDING:     return JS_DupValue(ctx, w->renderbuffer);
    case GL_TEXTURE_BINDING_2D:       return JS_DupValue(ctx, w->textures[w->unit][0]);
    case GL_TEXTURE_BINDING_CUBE_MAP: return JS_DupValue(ctx, w->textures[w->unit][1]);
    case GL_VIEWPORT:
    case GL_SCISSOR_BOX:
      qgl.GetIntegerv(p, iv);
      return int_array(ctx, iv, 4);
    case GL_MAX_VIEWPORT_DIMS:
      qgl.GetIntegerv(p, iv);
      return int_array(ctx, iv, 2);
    case GL_COLOR_CLEAR_VALUE:
    case GL_BLEND_COLOR:
      qgl.GetFloatv(p, fv);
      return float_array(ctx, fv, 4);
    case GL_DEPTH_RANGE:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
      qgl.GetFloatv(p, fv);
      return float_array(ctx, fv, 2);
    case GL_DEPTH_CLEAR_VALUE:
    case GL_LINE_WIDTH:
    case GL_POLYGON_OFFSET_FACTOR:
    case GL_POLYGON_OFFSET_UNITS:
    case GL_SAMPLE_COVERAGE_VALUE:
      qgl.GetFloatv(p, fv);
      return JS_NewFloat64(ctx, fv[0]);
    case GL_COLOR_WRITEMASK: {
      qgl.GetBooleanv(p, bv);
      JSValue a = JS_NewArray(ctx);
      for (int i = 0; i < 4; i++) JS_SetPropertyUint32(ctx, a, i, JS_NewBool(ctx, bv[i]));
      return a;
    }
    case GL_BLEND: case GL_CULL_FACE: case GL_DEPTH_TEST: case GL_DITHER: case GL_POLYGON_OFFSET_FILL:
    case GL_SAMPLE_ALPHA_TO_COVERAGE: case GL_SAMPLE_COVERAGE: case GL_SCISSOR_TEST: case GL_STENCIL_TEST:
    case GL_DEPTH_WRITEMASK: case GL_SAMPLE_COVERAGE_INVERT:
      qgl.GetBooleanv(p, bv);
      return JS_NewBool(ctx, bv[0]);
    case GL_COMPRESSED_TEXTURE_FORMATS:
      return int_array(ctx, iv, 0);
    default:
      while (qgl.GetError() != GL_NO_ERROR) {}
      qgl.GetIntegerv(p, iv);
      if (qgl.GetError() != GL_NO_ERROR) return JS_NULL;
      return JS_NewInt32(ctx, iv[0]);
  }
}

static JSValue js_gl_getBufferParameter(ARGS) {
  CTX();
  GLint v = 0;
  qgl.GetBufferParameteriv(u(0), u(1), &v);
  return JS_NewInt32(ctx, v);
}

static JSValue js_gl_getRenderbufferParameter(ARGS) {
  CTX();
  GLint v = 0;
  qgl.GetRenderbufferParameteriv(u(0), u(1), &v);
  return JS_NewInt32(ctx, v);
}

static JSValue js_gl_getTexParameter(ARGS) {
  CTX();
  GLint v = 0;
  qgl.GetTexParameteriv(u(0), u(1), &v);
  return JS_NewInt32(ctx, v);
}

static JSValue js_gl_getFramebufferAttachmentParameter(ARGS) {
  CTX();
  GLint v = 0;
  qgl.GetFramebufferAttachmentParameteriv(u(0), u(1), u(2), &v);
  return JS_NewInt32(ctx, v);
}

static JSValue js_gl_getVertexAttrib(ARGS) {
  CTX();
  GLint v = 0;
  qgl.GetVertexAttribiv(u(0), u(1), &v);
  return JS_NewInt32(ctx, v);
}

static JSValue js_gl_getVertexAttribOffset(ARGS) {
  CTX();
  void *ptr = NULL;
  qgl.GetVertexAttribPointerv(u(0), u(1), &ptr);
  return JS_NewInt64(ctx, (int64_t)(intptr_t)ptr);
}

static JSValue js_gl_getUniform(ARGS) {
  CTX();
  return JS_NULL;   // TODO: casi ningun juego lo usa
}

static JSValue js_gl_isContextLost(ARGS) {
  WebGLCtx *w = JS_GetOpaque2(ctx, this_val, js_webgl_class_id);
  if (!w) return JS_EXCEPTION;
  return JS_NewBool(ctx, w->lost);
}

static JSValue js_gl_getContextAttributes(ARGS) {
  WebGLCtx *w = JS_GetOpaque2(ctx, this_val, js_webgl_class_id);
  if (!w) return JS_EXCEPTION;
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "alpha", JS_NewBool(ctx, w->alpha));
  JS_SetPropertyStr(ctx, o, "depth", JS_NewBool(ctx, w->want_depth));
  JS_SetPropertyStr(ctx, o, "stencil", JS_NewBool(ctx, w->want_stencil));
  JS_SetPropertyStr(ctx, o, "antialias", JS_FALSE);
  JS_SetPropertyStr(ctx, o, "premultipliedAlpha", JS_NewBool(ctx, w->premultiplied));
  JS_SetPropertyStr(ctx, o, "preserveDrawingBuffer", JS_NewBool(ctx, w->preserve));
  JS_SetPropertyStr(ctx, o, "failIfMajorPerformanceCaveat", JS_FALSE);
  JS_SetPropertyStr(ctx, o, "powerPreference", JS_NewString(ctx, "default"));
  return o;
}
