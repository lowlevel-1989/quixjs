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

// fillRect, strokeRect, clearRect, paths

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "binding/canvas.h"
#include "core/app_context.h"

JSValue js_canvas_fillRect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 4) return JS_UNDEFINED;

  int x = 0, y = 0, w = 0, h = 0;
  JS_ToInt32(ctx, &x, argv[0]);
  JS_ToInt32(ctx, &y, argv[1]);
  JS_ToInt32(ctx, &w, argv[2]);
  JS_ToInt32(ctx, &h, argv[3]);

  // unsigned char r, g, b;
  printf("x %d y %d w %d h %d\n", x, y, w, h);

  AppContext *app = JS_GetContextOpaque(ctx);
  app->backend->iface->fill_rect_color(app->backend, x, y, w, h, 0, 0, 0, 255);

  return JS_UNDEFINED;
}

/*
static JSValue js_canvas_context_clearRect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 4) return JS_UNDEFINED;

  int x = 0, y = 0, w = 0, h = 0;
  JS_ToInt32(ctx, &x, argv[0]);
  JS_ToInt32(ctx, &y, argv[1]);
  JS_ToInt32(ctx, &w, argv[2]);
  JS_ToInt32(ctx, &h, argv[3]);

  if (active_backend && active_backend->iface->clear_rect) {
    active_backend->iface->clear_rect(active_backend, x, y, w, h);
  }

  return JS_UNDEFINED;
}
*/
