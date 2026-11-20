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

// Excepciones: JS_Throw, JS_GetException, JS_ThrowTypeError... y
// js_std_dump_error.
// Parte de runtime/js/jsc.c, que une las partes y se compila como un solo
// archivo (ver runtime/js/README.md): no se compila sola. Lo compartido esta
// en jsc.h.

#include "jsc.h"

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
