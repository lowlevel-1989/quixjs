/*
 * QuixJS - A lightweight JS runtime using SDL2 and JavaScriptCore
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

// setTimeout, setInterval, requestAnimationFrame y performance.now() para JS,
// sobre el reloj y los timers de core/timer.c.

#include "core/timer.h"
#include "timer.h"

static JSValue js_requestAnimationFrame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1 || !JS_IsFunction(ctx, argv[0]))
    return JS_ThrowTypeError(ctx, "callback must be a function");

  int id = timer_request_animation_frame(ctx, argv[0]);
  if (id < 0) return JS_ThrowOutOfMemory(ctx);
  return JS_NewInt32(ctx, id);
}

static JSValue js_cancelAnimationFrame(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  int id = 0;
  if (argc < 1 || JS_ToInt32(ctx, &id, argv[0])) return JS_UNDEFINED;
  timer_cancel_animation_frame(ctx, id);
  return JS_UNDEFINED;
}

// magic 0: setTimeout, 1: setInterval
static JSValue js_setTimer(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) {
  if (argc < 1 || !JS_IsFunction(ctx, argv[0]))
    return JS_ThrowTypeError(ctx, "callback must be a function");

  double delay = 0;
  if (argc > 1 && JS_ToFloat64(ctx, &delay, argv[1])) return JS_EXCEPTION;

  int extra = argc > 2 ? argc - 2 : 0;
  int id = timer_set(ctx, argv[0], delay, magic, extra, extra ? argv + 2 : NULL);
  if (id < 0) return JS_ThrowOutOfMemory(ctx);
  return JS_NewInt32(ctx, id);
}

static JSValue js_clearTimer(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  int id = 0;
  if (argc < 1 || JS_ToInt32(ctx, &id, argv[0])) return JS_UNDEFINED;
  timer_clear(ctx, id);
  return JS_UNDEFINED;
}

static JSValue js_performance_now(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return JS_NewFloat64(ctx, clock_now_ms());
}

static const JSCFunctionListEntry js_timer_funcs[] = {
  JS_CFUNC_MAGIC_DEF("setTimeout",  2, js_setTimer, 0),
  JS_CFUNC_MAGIC_DEF("setInterval", 2, js_setTimer, 1),
  JS_CFUNC_DEF("clearTimeout",  1, js_clearTimer),
  JS_CFUNC_DEF("clearInterval", 1, js_clearTimer),
  JS_CFUNC_DEF("requestAnimationFrame", 1, js_requestAnimationFrame),
  JS_CFUNC_DEF("cancelAnimationFrame",  1, js_cancelAnimationFrame),
};

static const JSCFunctionListEntry js_performance_funcs[] = {
  JS_CFUNC_DEF("now", 0, js_performance_now),
};

void js_api_add_timer(JSContext *ctx) {
  JSValue global = JS_GetGlobalObject(ctx);

  JS_SetPropertyFunctionList(ctx, global, js_timer_funcs, countof(js_timer_funcs));

  JSValue performance = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, performance, js_performance_funcs, countof(js_performance_funcs));
  JS_SetPropertyStr(ctx, global, "performance", performance);

  JS_FreeValue(ctx, global);
}
