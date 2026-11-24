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

// Modo WebView2. Ver webview2.h.

#include <stdio.h>
#include <string.h>
#include "webview2.h"

// runtime/binding/webview2/*.js, metidos en el binario al compilar
// (tools/embed_js.py, como los de nw.js)
#include "webview2_parts.h"

int js_api_add_webview2(JSContext *ctx) {
  for (size_t i = 0; i < sizeof(WEBVIEW2_PARTS) / sizeof(WEBVIEW2_PARTS[0]); i++) {
    char name[96];
    snprintf(name, sizeof(name), "<quix-webview2/%s>", WEBVIEW2_PARTS[i].name);
    JSValue r = JS_Eval(ctx, WEBVIEW2_PARTS[i].src, strlen(WEBVIEW2_PARTS[i].src), name, JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(r)) {
      js_std_dump_error(ctx);
      return -1;
    }
    JS_FreeValue(ctx, r);
  }
  printf("WebView2 mode\n");
  return 0;
}
