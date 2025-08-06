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

#include <stdio.h>
#include <string.h>
#include "core/app_context.h"
#include "canvas.h"

static JSValue js_canvas_getContext(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

JSValue js_create_canvas(JSContext *ctx) {
  JSValue canvas = JS_NewObject(ctx);

  AppContext *app = JS_GetContextOpaque(ctx);

  JS_SetPropertyStr(ctx, canvas, "width",  JS_NewInt32(ctx, app->cfg->game.width));
  JS_SetPropertyStr(ctx, canvas, "height", JS_NewInt32(ctx, app->cfg->game.height));

  // getContext() method
  JS_SetPropertyStr(
      ctx,
      canvas,
      "getContext",
      JS_NewCFunction(ctx, js_canvas_getContext, "getContext", 1)
      );
  return canvas;
}

static JSValue js_canvas_getContext(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    const char *type = argc > 0 ? JS_ToCString(ctx, argv[0]) : NULL;

    printf("[canvas getContext] type %s\n", type);
    if (!type || strcmp(type, "2d") != 0) {
        if (type) JS_FreeCString(ctx, type);
        return JS_NULL;
    }
    JS_FreeCString(ctx, type);
    return js_create_canvas_context(ctx);
}
