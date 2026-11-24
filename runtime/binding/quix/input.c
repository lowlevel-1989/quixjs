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

// runtime/binding/quix/input.c: quix.input: meter teclas (pressKey) y
// botones de un mando de prueba (pressPad) en la cola de entrada, para los
// tests y las pruebas.

#include <string.h>
#include "core/app_context.h"
#include "core/keymap.h"
#include "common.h"

static JSValue js_quix_input_pressKey(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  AppContext *app = JS_GetContextOpaque(ctx);
  if (argc < 1) return JS_ThrowTypeError(ctx, "pressKey: code required");

  const char *code = JS_ToCString(ctx, argv[0]);
  if (!code) return JS_EXCEPTION;
  const char *type = argc > 1 ? JS_ToCString(ctx, argv[1]) : NULL;

  int down = !type || strcmp(type, "down") == 0 || strcmp(type, "press") == 0;
  int up = !type || strcmp(type, "up") == 0 || strcmp(type, "press") == 0;

  InputEvent ev;
  int ok = 1;
  if (down) {
    if (keymap_make_event(&ev, INPUT_KEY_DOWN, code, 0, 0) == 0) { ev.synthetic = 1; backend_push_input(app->backend, &ev); }
    else ok = 0;
  }
  if (up && ok) {
    if (keymap_make_event(&ev, INPUT_KEY_UP, code, 0, 0) == 0) { ev.synthetic = 1; backend_push_input(app->backend, &ev); }
    else ok = 0;
  }

  JS_FreeCString(ctx, code);
  if (type) JS_FreeCString(ctx, type);
  if (!ok) return JS_ThrowRangeError(ctx, "pressKey: unknown key code");
  return JS_UNDEFINED;
}

// quix.input.pressPad(boton, t): el boton (indice del mapeo "standard") de un mando
// de prueba (backend_test_pad); t es "down", "up" o "press" (por defecto).
// Devuelve la posicion del mando en navigator.getGamepads(), o -1 si el
// backend no tiene mando de prueba (dummy)
static JSValue js_quix_input_pressPad(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  AppContext *app = JS_GetContextOpaque(ctx);
  int32_t button = -1;
  if (argc < 1 || JS_ToInt32(ctx, &button, argv[0])) return JS_ThrowTypeError(ctx, "pressPad: button required");
  const char *type = argc > 1 ? JS_ToCString(ctx, argv[1]) : NULL;
  int t = TEST_PAD_PRESS;
  if (type && strcmp(type, "down") == 0) t = TEST_PAD_DOWN;
  else if (type && strcmp(type, "up") == 0) t = TEST_PAD_UP;
  if (type) JS_FreeCString(ctx, type);
  return JS_NewInt32(ctx, backend_test_pad(app->backend, button, t));
}

static const JSCFunctionListEntry js_quix_input[] = {
  JS_CFUNC_DEF("pressKey", 2, js_quix_input_pressKey),
  JS_CFUNC_DEF("pressPad", 2, js_quix_input_pressPad),
};

void quix_add_input(JSContext *ctx, JSValueConst quix) {
  quix_add_group(ctx, quix, "input", js_quix_input, countof(js_quix_input));
}
