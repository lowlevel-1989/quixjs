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

// El estado de GL (blend, depth, stencil, viewport, enable...), clear y
// drawArrays / drawElements: casi todos con SIMPLE (args.h).
// Parte de binding/webgl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "args.h"

SIMPLE(blendColor,            qgl.BlendColor(f(0), f(1), f(2), f(3)))
SIMPLE(blendEquation,         qgl.BlendEquation(u(0)))
SIMPLE(blendEquationSeparate, qgl.BlendEquationSeparate(u(0), u(1)))
SIMPLE(blendFunc,             qgl.BlendFunc(u(0), u(1)))
SIMPLE(blendFuncSeparate,     qgl.BlendFuncSeparate(u(0), u(1), u(2), u(3)))
SIMPLE(clearColor,            qgl.ClearColor(f(0), f(1), f(2), f(3)))
SIMPLE(clearDepth,            qgl.ClearDepthf(f(0)))
SIMPLE(clearStencil,          qgl.ClearStencil(n(0)))
SIMPLE(colorMask,             qgl.ColorMask(b(0), b(1), b(2), b(3)))
SIMPLE(cullFace,              qgl.CullFace(u(0)))
SIMPLE(depthFunc,             qgl.DepthFunc(u(0)))
SIMPLE(depthMask,             qgl.DepthMask(b(0)))
SIMPLE(depthRange,            qgl.DepthRangef(f(0), f(1)))
SIMPLE(disable,               qgl.Disable(u(0)))
SIMPLE(enable,                qgl.Enable(u(0)))
SIMPLE(disableVertexAttribArray, qgl.DisableVertexAttribArray(u(0)))
SIMPLE(enableVertexAttribArray,  qgl.EnableVertexAttribArray(u(0)))
SIMPLE(finish,                qgl.Finish())
SIMPLE(flush,                 qgl.Flush())
SIMPLE(frontFace,             qgl.FrontFace(u(0)))
SIMPLE(hint,                  qgl.Hint(u(0), u(1)))
SIMPLE(lineWidth,             qgl.LineWidth(f(0)))
SIMPLE(polygonOffset,         qgl.PolygonOffset(f(0), f(1)))
SIMPLE(sampleCoverage,        qgl.SampleCoverage(f(0), b(1)))
SIMPLE(scissor,               qgl.Scissor(n(0), n(1), n(2), n(3)))
SIMPLE(stencilFunc,           qgl.StencilFunc(u(0), n(1), u(2)))
SIMPLE(stencilFuncSeparate,   qgl.StencilFuncSeparate(u(0), u(1), n(2), u(3)))
SIMPLE(stencilMask,           qgl.StencilMask(u(0)))
SIMPLE(stencilMaskSeparate,   qgl.StencilMaskSeparate(u(0), u(1)))
SIMPLE(stencilOp,             qgl.StencilOp(u(0), u(1), u(2)))
SIMPLE(stencilOpSeparate,     qgl.StencilOpSeparate(u(0), u(1), u(2), u(3)))
SIMPLE(texParameterf,         qgl.TexParameterf(u(0), u(1), f(2)))
SIMPLE(texParameteri,         qgl.TexParameteri(u(0), u(1), n(2)))
SIMPLE(vertexAttrib1f,        qgl.VertexAttrib1f(u(0), f(1)))
SIMPLE(vertexAttrib2f,        qgl.VertexAttrib2f(u(0), f(1), f(2)))
SIMPLE(vertexAttrib3f,        qgl.VertexAttrib3f(u(0), f(1), f(2), f(3)))
SIMPLE(vertexAttrib4f,        qgl.VertexAttrib4f(u(0), f(1), f(2), f(3), f(4)))
SIMPLE(vertexAttribPointer,   qgl.VertexAttribPointer(u(0), n(1), u(2), b(3), n(4), (const void *)(intptr_t)n(5)))
SIMPLE(viewport,              qgl.Viewport(n(0), n(1), n(2), n(3)))
SIMPLE(copyTexImage2D,        qgl.CopyTexImage2D(u(0), n(1), u(2), n(3), n(4), n(5), n(6), n(7)))
SIMPLE(copyTexSubImage2D,     qgl.CopyTexSubImage2D(u(0), n(1), n(2), n(3), n(4), n(5), n(6), n(7)))

static JSValue js_gl_activeTexture(ARGS) {
  CTX();
  GLuint t = u(0);
  if (t >= GL_TEXTURE0 && t < GL_TEXTURE0 + MAX_UNITS) w->unit = (int)(t - GL_TEXTURE0);
  qgl.ActiveTexture(t);
  return JS_UNDEFINED;
}

static JSValue js_gl_clear(ARGS) {
  CTX();
  qgl.Clear(u(0));
  mark_drawn(w);
  return JS_UNDEFINED;
}

static JSValue js_gl_drawArrays(ARGS) {
  CTX();
  qgl.DrawArrays(u(0), n(1), n(2));
  mark_drawn(w);
  return JS_UNDEFINED;
}

static JSValue js_gl_drawElements(ARGS) {
  backend_loading_poke();   // un trabajo largo: el aviso de carga, si toca
  CTX();
  qgl.DrawElements(u(0), n(1), u(2), (const void *)(intptr_t)n(3));
  mark_drawn(w);
  return JS_UNDEFINED;
}

static JSValue js_gl_getError(ARGS) {
  WebGLCtx *w = webgl_get(ctx, this_val);
  if (!w) return JS_EXCEPTION;
  return JS_NewInt32(ctx, (int)qgl.GetError());
}

static JSValue js_gl_isEnabled(ARGS) {
  CTX();
  return JS_NewBool(ctx, qgl.IsEnabled(u(0)));
}

static JSValue js_gl_pixelStorei(ARGS) {
  CTX();
  GLenum p = u(0);
  if (p == UNPACK_FLIP_Y_WEBGL) w->flip_y = n(1) != 0;
  else if (p == UNPACK_PREMULTIPLY_ALPHA_WEBGL) w->premultiply = n(1) != 0;
  else if (p == UNPACK_COLORSPACE_CONVERSION_WEBGL) {}
  else qgl.PixelStorei(p, n(1));
  return JS_UNDEFINED;
}

static JSValue js_gl_checkFramebufferStatus(ARGS) {
  CTX();
  return JS_NewInt32(ctx, (int)qgl.CheckFramebufferStatus(u(0)));
}
