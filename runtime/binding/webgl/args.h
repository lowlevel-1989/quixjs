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
#pragma once

// Los argumentos de los metodos de WebGL: U/I/F/B (uint, int, float, bool
// del argumento i) y los macros con los que se escriben los metodos (ARGS,
// CTX, SIMPLE...). Parte de binding/webgl.c (ver README.md).

#include "common.h"

static GLuint U(JSContext *ctx, int argc, JSValueConst *argv, int i) {
  uint32_t v = 0;
  if (i < argc) JS_ToUint32(ctx, &v, argv[i]);
  return v;
}
static GLint I(JSContext *ctx, int argc, JSValueConst *argv, int i) {
  int32_t v = 0;
  if (i < argc) JS_ToInt32(ctx, &v, argv[i]);
  return v;
}
static GLfloat F(JSContext *ctx, int argc, JSValueConst *argv, int i) {
  double v = 0;
  if (i < argc) JS_ToFloat64(ctx, &v, argv[i]);
  return (GLfloat)v;
}
static GLboolean B(JSContext *ctx, int argc, JSValueConst *argv, int i) {
  return (i < argc && JS_ToBool(ctx, argv[i])) ? GL_TRUE : GL_FALSE;
}

#define ARGS JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv
#define CTX() WebGLCtx *w = webgl_get(ctx, this_val); if (!w) return JS_EXCEPTION; if (w->lost) return JS_UNDEFINED
#define u(i) U(ctx, argc, argv, i)
#define n(i) I(ctx, argc, argv, i)
#define f(i) F(ctx, argc, argv, i)
#define b(i) B(ctx, argc, argv, i)

// los metodos que solo reciben numeros
#define SIMPLE(js, call) static JSValue js_gl_##js(ARGS) { CTX(); call; return JS_UNDEFINED; }
