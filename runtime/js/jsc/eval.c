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

// Ejecutar codigo: JS_Eval...
// Parte de runtime/js/jsc.c, que une las partes y se compila como un solo
// archivo (ver runtime/js/README.md): no se compila sola. Lo compartido esta
// en jsc.h.

#include "jsc.h"

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
