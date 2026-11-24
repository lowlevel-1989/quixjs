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

// Las APIs del navegador escritas en JS (binding/polyfill/*.js, dentro del
// binario por tools/embed_js.py). Ver polyfill.h.

#include <stdio.h>
#include <string.h>
#include "polyfill.h"
// polyfill/*.js: tools/embed_js.py los mete en el binario al compilar;
// polyfills.h (generado) define POLYFILLS[], {nombre, texto} de cada uno.
// Ver docs/embedded-js.md.
#include "polyfills.h"

// Cada archivo es independiente (su propia funcion, que se llama a si misma con
// globalThis): el orden entre ellos no importa y, si uno falla, el error dice
// cual. Necesitan las APIs de C: se ejecutan despues de ellas
// (runtime/core/main/start.c llama a js_api_add_polyfill despues de
// js_api_add_window, _video...): runtime/binding/polyfill/fonts.js y
// runtime/binding/polyfill/media.js usan document al ejecutarse,
// runtime/binding/polyfill/window.js innerWidth
int js_api_add_polyfill(JSContext *ctx) {
  for (size_t i = 0; i < sizeof(POLYFILLS) / sizeof(POLYFILLS[0]); i++) {
    char name[96];
    snprintf(name, sizeof(name), "<quix-polyfill/%s>", POLYFILLS[i].name);
    JSValue r = JS_Eval(ctx, POLYFILLS[i].src, strlen(POLYFILLS[i].src), name, JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(r)) {
      js_std_dump_error(ctx);
      return -1;
    }
    JS_FreeValue(ctx, r);
  }
  return 0;
}
