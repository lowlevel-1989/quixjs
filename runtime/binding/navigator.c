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

#include "navigator.h"

JSValue js_navigator_getGamepads(JSContext *ctx, JSValueConst this_val,
    int argc, JSValueConst *argv) {
  printf("[getGamepads] called\n");

  return JS_UNDEFINED;
}


JSValue js_create_navigator(JSContext *ctx) {
  JSValue navigator = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, navigator, "userAgent", JS_NewString(ctx, "quixjs Dummy"));

  JS_SetPropertyStr(ctx, navigator, "getGamepads",
      JS_NewCFunction(ctx, js_navigator_getGamepads, "getGamepads", 0));
  JS_SetPropertyStr(ctx, navigator, "webkitGetGamepads",
      JS_NewCFunction(ctx, js_navigator_getGamepads, "webkitGetGamepads", 0));
  return navigator;
}

