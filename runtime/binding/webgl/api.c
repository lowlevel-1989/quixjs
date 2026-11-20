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

// La lista de metodos y propiedades de WebGLRenderingContext,
// js_api_add_webgl y js_create_webgl_context.
// Parte de binding/webgl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "args.h"

#define FN(name, len) JS_CFUNC_DEF(#name, len, js_gl_##name)
static const JSCFunctionListEntry js_webgl_proto[] = {
  JS_CGETSET_DEF("canvas", js_gl_get_canvas, NULL),
  JS_CGETSET_MAGIC_DEF("drawingBufferWidth", js_gl_get_dbw, NULL, 0),
  JS_CGETSET_MAGIC_DEF("drawingBufferHeight", js_gl_get_dbw, NULL, 1),
  FN(activeTexture, 1), FN(attachShader, 2), FN(bindAttribLocation, 3), FN(bindBuffer, 2),
  FN(bindFramebuffer, 2), FN(bindRenderbuffer, 2), FN(bindTexture, 2), FN(blendColor, 4),
  FN(blendEquation, 1), FN(blendEquationSeparate, 2), FN(blendFunc, 2), FN(blendFuncSeparate, 4),
  FN(bufferData, 3), FN(bufferSubData, 3), FN(checkFramebufferStatus, 1), FN(clear, 1),
  FN(clearColor, 4), FN(clearDepth, 1), FN(clearStencil, 1), FN(colorMask, 4), FN(compileShader, 1),
  FN(compressedTexImage2D, 7), FN(copyTexImage2D, 8), FN(copyTexSubImage2D, 8),
  FN(createBuffer, 0), FN(createFramebuffer, 0), FN(createProgram, 0), FN(createRenderbuffer, 0),
  FN(createShader, 1), FN(createTexture, 0), FN(cullFace, 1),
  FN(deleteBuffer, 1), FN(deleteFramebuffer, 1), FN(deleteProgram, 1), FN(deleteRenderbuffer, 1),
  FN(deleteShader, 1), FN(deleteTexture, 1), FN(depthFunc, 1), FN(depthMask, 1), FN(depthRange, 2),
  FN(detachShader, 2), FN(disable, 1), FN(disableVertexAttribArray, 1), FN(drawArrays, 3),
  FN(drawElements, 4), FN(enable, 1), FN(enableVertexAttribArray, 1), FN(finish, 0), FN(flush, 0),
  FN(framebufferRenderbuffer, 4), FN(framebufferTexture2D, 5), FN(frontFace, 1), FN(generateMipmap, 1),
  FN(getActiveAttrib, 2), FN(getActiveUniform, 2), FN(getAttachedShaders, 1), FN(getAttribLocation, 2),
  FN(getBufferParameter, 2), FN(getContextAttributes, 0), FN(getError, 0), FN(getExtension, 1),
  FN(getFramebufferAttachmentParameter, 3), FN(getParameter, 1), FN(getProgramInfoLog, 1),
  FN(getProgramParameter, 2), FN(getRenderbufferParameter, 2), FN(getShaderInfoLog, 1),
  FN(getShaderParameter, 2), FN(getShaderPrecisionFormat, 2), FN(getShaderSource, 1),
  FN(getSupportedExtensions, 0), FN(getTexParameter, 2), FN(getUniform, 2), FN(getUniformLocation, 2),
  FN(getVertexAttrib, 2), FN(getVertexAttribOffset, 2), FN(hint, 2), FN(isBuffer, 1),
  FN(isContextLost, 0), FN(isEnabled, 1), FN(isFramebuffer, 1), FN(isProgram, 1), FN(isRenderbuffer, 1),
  FN(isShader, 1), FN(isTexture, 1), FN(lineWidth, 1), FN(linkProgram, 1), FN(pixelStorei, 2),
  FN(polygonOffset, 2), FN(readPixels, 7), FN(renderbufferStorage, 4), FN(sampleCoverage, 2),
  FN(scissor, 4), FN(shaderSource, 2), FN(stencilFunc, 3), FN(stencilFuncSeparate, 4),
  FN(stencilMask, 1), FN(stencilMaskSeparate, 2), FN(stencilOp, 3), FN(stencilOpSeparate, 4),
  FN(texImage2D, 9), FN(texParameterf, 3), FN(texParameteri, 3), FN(texSubImage2D, 9),
  FN(uniform1f, 2), FN(uniform1fv, 2), FN(uniform1i, 2), FN(uniform1iv, 2),
  FN(uniform2f, 3), FN(uniform2fv, 2), FN(uniform2i, 3), FN(uniform2iv, 2),
  FN(uniform3f, 4), FN(uniform3fv, 2), FN(uniform3i, 4), FN(uniform3iv, 2),
  FN(uniform4f, 5), FN(uniform4fv, 2), FN(uniform4i, 5), FN(uniform4iv, 2),
  FN(uniformMatrix2fv, 3), FN(uniformMatrix3fv, 3), FN(uniformMatrix4fv, 3), FN(useProgram, 1),
  FN(validateProgram, 1), FN(vertexAttrib1f, 2), FN(vertexAttrib1fv, 2), FN(vertexAttrib2f, 3),
  FN(vertexAttrib2fv, 2), FN(vertexAttrib3f, 4), FN(vertexAttrib3fv, 2), FN(vertexAttrib4f, 5),
  FN(vertexAttrib4fv, 2), FN(vertexAttribPointer, 6), FN(viewport, 4),
};

static JSValue js_illegal_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
  return JS_ThrowTypeError(ctx, "Illegal constructor");
}

static void classes_init(JSContext *ctx) {
  if (classes_ready) return;
  JSRuntime *rt = JS_GetRuntime(ctx);
  JS_NewClassID(&js_webgl_class_id);
  JS_NewClass(rt, js_webgl_class_id, &js_webgl_class);
  JS_NewClassID(&js_globj_class_id);
  JS_NewClass(rt, js_globj_class_id, &js_globj_class);
  JS_NewClassID(&js_glloc_class_id);
  JS_NewClass(rt, js_glloc_class_id, &js_glloc_class);

  JSValue global = JS_GetGlobalObject(ctx);

  JSValue proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, proto, js_webgl_proto, countof(js_webgl_proto));
  JS_SetPropertyFunctionList(ctx, proto, js_webgl_constants, countof(js_webgl_constants));
  JSValue ctor = JS_NewCFunction2(ctx, js_illegal_ctor, "WebGLRenderingContext", 0, JS_CFUNC_constructor, 0);
  JS_SetPropertyFunctionList(ctx, ctor, js_webgl_constants, countof(js_webgl_constants));
  JS_SetConstructor(ctx, ctor, proto);
  JS_SetClassProto(ctx, js_webgl_class_id, proto);
  JS_SetPropertyStr(ctx, global, "WebGLRenderingContext", ctor);

  static const char *names[OBJ_COUNT] = { "WebGLBuffer", "WebGLFramebuffer", "WebGLProgram",
                                          "WebGLRenderbuffer", "WebGLShader", "WebGLTexture" };
  for (int i = 0; i < OBJ_COUNT; i++) {
    obj_protos[i] = JS_NewObject(ctx);
    JSValue c = JS_NewCFunction2(ctx, js_illegal_ctor, names[i], 0, JS_CFUNC_constructor, 0);
    JS_SetConstructor(ctx, c, obj_protos[i]);
    JS_SetPropertyStr(ctx, global, names[i], c);
    // el prototipo vive mientras el constructor este en global
    JS_FreeValue(ctx, obj_protos[i]);
  }
  loc_proto = JS_NewObject(ctx);
  JSValue lc = JS_NewCFunction2(ctx, js_illegal_ctor, "WebGLUniformLocation", 0, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, lc, loc_proto);
  JS_SetPropertyStr(ctx, global, "WebGLUniformLocation", lc);
  JS_FreeValue(ctx, loc_proto);

  JS_FreeValue(ctx, global);
  classes_ready = 1;
}

void js_api_add_webgl(JSContext *ctx) {
  // las clases (WebGLRenderingContext...) existen desde el principio: los
  // juegos las miran antes de pedir el contexto. El contexto GL se crea con
  // el primer getContext("webgl").
  if (enabled) classes_init(ctx);
}

static int attr_bool(JSContext *ctx, JSValueConst attrs, const char *name, int def) {
  if (!JS_IsObject(attrs)) return def;
  JSValue v = JS_GetPropertyStr(ctx, attrs, name);
  int r = JS_IsUndefined(v) ? def : JS_ToBool(ctx, v);
  JS_FreeValue(ctx, v);
  return r;
}

JSValue js_create_webgl_context(JSContext *ctx, JSValueConst canvas, JSValueConst attrs) {
  if (!enabled) return JS_NULL;
  if (gl_init(headless) != 0) {
    enabled = 0;   // sin GL: los juegos usan su renderer de canvas
    return JS_NULL;
  }
  classes_init(ctx);
  gl_make_current();

  CanvasData *cd = js_canvas_get(ctx, canvas);
  if (!cd) return JS_NULL;

  JSValue obj = JS_NewObjectClass(ctx, js_webgl_class_id);
  if (JS_IsException(obj)) return obj;
  WebGLCtx *w = js_mallocz(ctx, sizeof(WebGLCtx));
  if (!w) {
    JS_FreeValue(ctx, obj);
    return JS_EXCEPTION;
  }
  w->canvas = canvas;
  JS_SetHidden(ctx, obj, "canvas", JS_DupValue(ctx, canvas));
  w->program = w->array_buffer = w->element_buffer = w->framebuffer = w->renderbuffer = JS_NULL;
  for (int i = 0; i < MAX_UNITS; i++) w->textures[i][0] = w->textures[i][1] = JS_NULL;
  w->alpha = attr_bool(ctx, attrs, "alpha", 1);
  w->want_depth = attr_bool(ctx, attrs, "depth", 1);
  w->want_stencil = attr_bool(ctx, attrs, "stencil", 0);
  w->premultiplied = attr_bool(ctx, attrs, "premultipliedAlpha", 1);
  w->preserve = attr_bool(ctx, attrs, "preserveDrawingBuffer", 0);
  w->antialias = 0;
  JS_SetOpaque(obj, w);

  active = w;
  if (fb_create(w, cd->surface.w, cd->surface.h) != 0) {
    // sin framebuffer valido no se puede dibujar
    JS_FreeValue(ctx, obj);
    return JS_NULL;
  }
  qgl.Viewport(0, 0, w->width, w->height);
  cd->webgl = w;
  return obj;
}
