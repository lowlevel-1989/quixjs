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

// runtime/binding/document/screen.c: que canvas van a la pantalla: los agregados al
// documento y visibles, en orden.
// Parte de runtime/binding/document.c (archivo indice).

#include "common.h"

static uint32_t array_length(JSContext *ctx, JSValueConst arr) {
  uint32_t len = 0;
  JSValue v = JS_GetPropertyStr(ctx, arr, "length");
  JS_ToUint32(ctx, &len, v);
  JS_FreeValue(ctx, v);
  return len;
}

static int array_index_of(JSContext *ctx, JSValueConst arr, JSValueConst item) {
  uint32_t len = array_length(ctx, arr);
  for (uint32_t i = 0; i < len; i++) {
    JSValue v = JS_GetPropertyUint32(ctx, arr, i);
    int same = JS_StrictEq(ctx, v, item);
    JS_FreeValue(ctx, v);
    if (same) return (int)i;
  }
  return -1;
}

static void array_remove_at(JSContext *ctx, JSValueConst arr, int idx) {
  JSValue splice = JS_GetPropertyStr(ctx, arr, "splice");
  JSValue args[2] = { JS_NewInt32(ctx, idx), JS_NewInt32(ctx, 1) };
  JSValue r = JS_Call(ctx, splice, arr, 2, args);
  JS_FreeValue(ctx, r);
  JS_FreeValue(ctx, splice);
}

static void screen_attach(JSContext *ctx, JSValueConst canvas) {
  if (!document_ready || !js_canvas_get(ctx, canvas)) return;
  if (array_index_of(ctx, screen_list, canvas) >= 0) return;
  JS_SetPropertyUint32(ctx, screen_list, array_length(ctx, screen_list), JS_DupValue(ctx, canvas));
}

static void screen_detach(JSContext *ctx, JSValueConst canvas) {
  if (!document_ready) return;
  int idx = array_index_of(ctx, screen_list, canvas);
  if (idx >= 0) array_remove_at(ctx, screen_list, idx);
}

static int style_hidden(JSContext *ctx, JSValueConst el) {
  JSValue style = JS_GetPropertyStr(ctx, el, "style");
  int hidden = 0;
  if (JS_IsObject(style)) {
    JSValue display = JS_GetPropertyStr(ctx, style, "display");
    JSValue visibility = JS_GetPropertyStr(ctx, style, "visibility");
    JSValue opacity = JS_GetPropertyStr(ctx, style, "opacity");
    const char *d = JS_IsString(display) ? JS_ToCString(ctx, display) : NULL;
    const char *v = JS_IsString(visibility) ? JS_ToCString(ctx, visibility) : NULL;
    if (d && strcmp(d, "none") == 0) hidden = 1;
    if (v && strcmp(v, "hidden") == 0) hidden = 1;
    // opacity 0: no se ve (RPG Maker MV deja asi su UpperCanvas, encima de
    // todo, salvo mientras carga). Sin esto se dibujaba en cada imagen.
    if (JS_IsNumber(opacity) || JS_IsString(opacity)) {
      // "" (sin fijar) no cuenta
      const char *o = JS_ToCString(ctx, opacity);
      if (o) {
        char *end = NULL;
        double val = strtod(o, &end);
        if (end != o && val <= 0) hidden = 1;
        JS_FreeCString(ctx, o);
      }
    }
    if (d) JS_FreeCString(ctx, d);
    if (v) JS_FreeCString(ctx, v);
    JS_FreeValue(ctx, display);
    JS_FreeValue(ctx, visibility);
    JS_FreeValue(ctx, opacity);
  }
  JS_FreeValue(ctx, style);
  return hidden;
}

int document_screen_canvases(JSContext *ctx, CanvasData **out, int max) {
  if (!document_ready) return 0;
  int n = 0;
  uint32_t len = array_length(ctx, screen_list);
  for (uint32_t i = 0; i < len && n < max; i++) {
    JSValue c = JS_GetPropertyUint32(ctx, screen_list, i);
    CanvasData *cd = js_canvas_get(ctx, c);
    if (cd && !style_hidden(ctx, c)) out[n++] = cd;
    JS_FreeValue(ctx, c);
  }
  return n;
}
