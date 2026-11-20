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

// Runtime y contextos: JS_NewRuntime, JS_NewContext, JS_GetGlobalObject...
// Parte de runtime/js/jsc.c, que une las partes y se compila como un solo
// archivo (ver runtime/js/README.md): no se compila sola. Lo compartido esta
// en jsc.h.

#include "jsc.h"

JSRuntime *quix_js_runtimes[4];

JSRuntime *JS_NewRuntime(void) {
  JSRuntime *rt = calloc(1, sizeof(JSRuntime));
  if (!rt) return NULL;
  rt->group = JSContextGroupCreate();
  jsc_function_init();
  for (int i = 0; i < 4; i++) {
    if (!quix_js_runtimes[i]) {
      quix_js_runtimes[i] = rt;
      break;
    }
  }
  return rt;
}

void JS_FreeRuntime(JSRuntime *rt) {
  if (!rt) return;
  for (int i = 0; i < 4; i++) {
    if (quix_js_runtimes[i] == rt) quix_js_runtimes[i] = NULL;
  }
  JSContextGroupRelease(rt->group);
  free(rt->pending_free);
  free(rt);
}

static JSObjectRef global_get_obj(JSGlobalContextRef g, const char *path) {
  // "Object.defineProperty": por partes desde el global
  char buf[128];
  snprintf(buf, sizeof buf, "%s", path);
  JSValueRef v = JSContextGetGlobalObject(g);
  for (char *tok = strtok(buf, "."); tok; tok = strtok(NULL, ".")) {
    JSStringRef n = JSStringCreateWithUTF8CString(tok);
    v = JSObjectGetProperty(g, (JSObjectRef)v, n, NULL);
    JSStringRelease(n);
    if (!JSValueIsObject(g, v)) return NULL;
  }
  JSValueProtect(g, v);
  return (JSObjectRef)v;
}

JSContext *JS_NewContext(JSRuntime *rt) {
  if (rt->context_count == MAX_CONTEXTS) return NULL;
  JSContext *ctx = calloc(1, sizeof(JSContext));
  if (!ctx) return NULL;
  ctx->rt = rt;
  ctx->g = JSGlobalContextCreateInGroup(rt->group, NULL);
  ctx->function_proto = global_get_obj(ctx->g, "Function.prototype");
  ctx->define_property = global_get_obj(ctx->g, "Object.defineProperty");
  ctx->error_ctor[0] = global_get_obj(ctx->g, "TypeError");
  ctx->error_ctor[1] = global_get_obj(ctx->g, "RangeError");
  ctx->error_ctor[2] = global_get_obj(ctx->g, "SyntaxError");
  rt->contexts[rt->context_count++] = ctx;
  return ctx;
}

void JS_FreeContext(JSContext *ctx) {
  if (!ctx) return;
  JSRuntime *rt = ctx->rt;
  drain(rt);
  for (int i = 0; i < rt->context_count; i++) {
    if (rt->contexts[i] == ctx) {
      rt->contexts[i] = rt->contexts[--rt->context_count];
      break;
    }
  }
  JSGlobalContextRelease(ctx->g);
  free(ctx);
}

JSRuntime *JS_GetRuntime(JSContext *ctx) { return ctx->rt; }
void JS_SetContextOpaque(JSContext *ctx, void *opaque) { ctx->opaque = opaque; }
void *JS_GetContextOpaque(JSContext *ctx) { return ctx->opaque; }

JSValue JS_GetGlobalObject(JSContext *ctx) {
  return wrap(ctx->g, JSContextGetGlobalObject(ctx->g), 1);
}
