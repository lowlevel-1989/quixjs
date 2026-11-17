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

// navigator: userAgent, platform, language y getGamepads() (la Gamepad API).

#include <stdio.h>
#include "core/app_context.h"
#include "core/timer.h"
#include "navigator.h"

// navigator.getGamepads() esta en JS (pad_js): pide aqui el estado de los
// mandos en un Float64Array, de una vez. Construir los objetos desde C (17
// botones con 3 propiedades cada uno) eran ~100 llamadas a la API de
// JavaScriptCore: ~300 us por llamada en una RG CubeXX, y RPG Maker la llama
// en cada actualizacion.
//
// Por mando, PAD_STRIDE numeros: conectado (0/1), los ejes, y por boton
// pulsado (0/1) y valor. Devuelve cuantos mandos mira.
#define PAD_STRIDE (1 + GAMEPAD_AXES + 2 * GAMEPAD_BUTTONS)
static JSValue js_pad_state(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  AppContext *app = JS_GetContextOpaque(ctx);
  size_t off = 0, len = 0, el = 0;
  if (argc < 1) return JS_NewInt32(ctx, 0);
  JSValue ab = JS_GetTypedArrayBuffer(ctx, argv[0], &off, &len, &el);
  if (JS_IsException(ab)) return ab;
  size_t size = 0;
  uint8_t *buf = JS_GetArrayBuffer(ctx, &size, ab);
  JS_FreeValue(ctx, ab);
  if (!buf || el != sizeof(double) || len < sizeof(double) * PAD_STRIDE * GAMEPAD_MAX) return JS_NewInt32(ctx, 0);
  double *out = (double *)(buf + off);
  GamepadState pads[GAMEPAD_MAX];
  int count = backend_get_gamepads(app->backend, pads, GAMEPAD_MAX);
  for (int i = 0; i < GAMEPAD_MAX; i++) {
    double *o = out + i * PAD_STRIDE;
    GamepadState *p = &pads[i];
    o[0] = i < count && p->connected ? 1 : 0;
    if (!o[0]) continue;
    for (int a = 0; a < GAMEPAD_AXES; a++) o[1 + a] = p->axes[a];
    for (int b = 0; b < GAMEPAD_BUTTONS; b++) {
      o[1 + GAMEPAD_AXES + 2 * b] = p->pressed[b] ? 1 : 0;
      o[2 + GAMEPAD_AXES + 2 * b] = p->value[b];
    }
  }
  return JS_NewInt32(ctx, GAMEPAD_MAX);
}

// el reloj de los mandos si aun no hay performance
static JSValue js_pad_now(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return JS_NewFloat64(ctx, clock_now_ms());
}

// el id de un mando (solo cuando aparece uno nuevo)
static JSValue js_pad_id(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  AppContext *app = JS_GetContextOpaque(ctx);
  int32_t i = 0;
  if (argc > 0) JS_ToInt32(ctx, &i, argv[0]);
  GamepadState pads[GAMEPAD_MAX];
  int count = backend_get_gamepads(app->backend, pads, GAMEPAD_MAX);
  if (i < 0 || i >= count || !pads[i].connected) return JS_NewString(ctx, "");
  return JS_NewString(ctx, pads[i].id);
}

// getGamepads en JS: siempre 4 posiciones, null si no hay mando. Los objetos
// de cada mando se guardan y se actualizan en su sitio (RPG Maker solo lee
// buttons[i].pressed y axes); uno nuevo si cambia el mando de esa posicion.
static const char pad_js[] =
  "(function (nav, state, idOf, AXES, BUTTONS, clock) {\n"
  "  var STRIDE = 1 + AXES + 2 * BUTTONS, buf = new Float64Array(STRIDE * 4), pads = [null, null, null, null];\n"
  "  function make(i) {\n"
  "    var b = [], a = [];\n"
  "    for (var k = 0; k < BUTTONS; k++) b.push({ pressed: false, touched: false, value: 0 });\n"
  "    for (var k = 0; k < AXES; k++) a.push(0);\n"
  "    return { id: idOf(i), index: i, connected: true, mapping: 'standard', timestamp: 0, axes: a, buttons: b };\n"
  "  }\n"
  "  function getGamepads() {\n"
  "    state(buf);\n"
  "    var list = [null, null, null, null], now = clock.now();\n"
  "    for (var i = 0; i < 4; i++) {\n"
  "      var o = i * STRIDE;\n"
  "      if (!buf[o]) { pads[i] = null; continue; }\n"
  "      var p = pads[i] || (pads[i] = make(i));\n"
  "      p.timestamp = now;\n"
  "      for (var k = 0; k < AXES; k++) p.axes[k] = buf[o + 1 + k];\n"
  "      for (var k = 0; k < BUTTONS; k++) {\n"
  "        var bt = p.buttons[k], pr = buf[o + 1 + AXES + 2 * k] !== 0;\n"
  "        bt.pressed = pr; bt.touched = pr; bt.value = buf[o + 2 + AXES + 2 * k];\n"
  "      }\n"
  "      list[i] = p;\n"
  "    }\n"
  "    return list;\n"
  "  }\n"
  "  nav.getGamepads = getGamepads;\n"
  "  nav.webkitGetGamepads = getGamepads;\n"
  "})";

JSValue js_create_navigator(JSContext *ctx) {
  JSValue navigator = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, navigator, "userAgent", JS_NewString(ctx, "Mozilla/5.0 (Linux) quixjs"));
  JS_SetPropertyStr(ctx, navigator, "platform", JS_NewString(ctx, "Linux"));
  JS_SetPropertyStr(ctx, navigator, "language", JS_NewString(ctx, "en-US"));
  JS_SetPropertyStr(ctx, navigator, "maxTouchPoints", JS_NewInt32(ctx, 0));
  JS_SetPropertyStr(ctx, navigator, "onLine", JS_FALSE);

  JSValue setup = JS_Eval(ctx, pad_js, sizeof(pad_js) - 1, "<quix-gamepads>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(setup)) {
    js_std_dump_error(ctx);
  } else {
    JSValue g = JS_GetGlobalObject(ctx);
    JSValue perf = JS_GetPropertyStr(ctx, g, "performance");
    JSValue clock = JS_IsObject(perf) ? perf : JS_NewObject(ctx);
    if (!JS_IsObject(perf)) {
      JS_FreeValue(ctx, perf);
      JS_SetPropertyStr(ctx, clock, "now", JS_NewCFunction(ctx, js_pad_now, "now", 0));
    }
    JSValue args[6] = { navigator, JS_NewCFunction(ctx, js_pad_state, "padState", 1),
                        JS_NewCFunction(ctx, js_pad_id, "padId", 1), JS_NewInt32(ctx, GAMEPAD_AXES),
                        JS_NewInt32(ctx, GAMEPAD_BUTTONS), clock };
    JSValue r = JS_Call(ctx, setup, JS_UNDEFINED, 6, args);
    if (JS_IsException(r)) js_std_dump_error(ctx);
    JS_FreeValue(ctx, r);
    for (int i = 1; i < 6; i++) if (i < 3 || i == 5) JS_FreeValue(ctx, args[i]);
    JS_FreeValue(ctx, g);
  }
  JS_FreeValue(ctx, setup);
  return navigator;
}
