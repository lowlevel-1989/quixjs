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

// fillStyle, strokeStyle, lineWidth, shadows

#include <string.h>
#include "style.h"
#include "binding/canvas.h"


static char fillstyle[32] = "rgb(159,81,196)";

JSValue js_canvas_get_fillStyle(JSContext *ctx, JSValueConst this_val) {
  return JS_NewString(ctx, fillstyle);
}

JSValue js_canvas_set_fillStyle(JSContext *ctx, JSValueConst this_val, JSValue val) {
  const char *str = JS_ToCString(ctx, val);
  if (!str) return JS_EXCEPTION;

  strncpy(fillstyle, str, sizeof(fillstyle) - 1);
  fillstyle[sizeof(fillstyle) - 1] = '\0';

  int r, g, b;
  if (sscanf(fillstyle, "rgb(%d,%d,%d)", &r, &g, &b) == 3) {
    printf("r: (%d) g: (%d) b: (%d)\n", r, g, b);
  }

  printf("fillStyle set to '%s'\n", fillstyle);

  JS_FreeCString(ctx, str);
  return JS_UNDEFINED;
}

