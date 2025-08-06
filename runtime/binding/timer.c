/*
 * QuixJS - A lightweight JS runtime using SDL2 and QuickJS
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
  int id;
  if (argc < 1 || JS_ToInt32(ctx, &id, argv[0]))
    return JS_EXCEPTION;

  timer_clear(ctx, id);
  return JS_UNDEFINED;
}

static JSValue js_setTimeout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 2 || !JS_IsFunction(ctx, argv[0])) return JS_ThrowTypeError(ctx, "Invalid arguments");

  int64_t delay;
  if (JS_ToInt64(ctx, &delay, argv[1])) return JS_EXCEPTION;

  int id = timer_set(ctx, argv[0], delay);
  if (id < 0) return JS_ThrowOutOfMemory(ctx);

  return JS_NewInt32(ctx, id);
}

static JSValue js_clearTimeout(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  int id;
  if (argc < 1 || JS_ToInt32(ctx, &id, argv[0])) return JS_EXCEPTION;

  timer_clear(ctx, id);
  return JS_UNDEFINED;
}

void js_api_add_timer(JSContext *ctx) {
  JSValue global = JS_GetGlobalObject(ctx);

  JS_SetPropertyStr(ctx, global, "setTimeout",
      JS_NewCFunction(ctx, js_setTimeout, "setTimeout", 2));
  JS_SetPropertyStr(ctx, global, "clearTimeout",
      JS_NewCFunction(ctx, js_clearTimeout, "clearTimeout", 1));
  JS_SetPropertyStr(ctx, global, "requestAnimationFrame",
      JS_NewCFunction(ctx, js_requestAnimationFrame, "requestAnimationFrame", 1));
  JS_SetPropertyStr(ctx, global, "cancelAnimationFrame",
      JS_NewCFunction(ctx, js_cancelAnimationFrame, "cancelAnimationFrame", 1));

  JS_FreeValue(ctx, global);
}

