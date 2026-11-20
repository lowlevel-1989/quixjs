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

// navigator: userAgent, platform, language, getGamepads() (la Gamepad API) y
// vibrate() (la Vibration API).

#include <stdio.h>
#include "core/app_context.h"
#include "core/timer.h"
#include "navigator.h"

// navigator.getGamepads() esta en JS (runtime/binding/gamepads.js): pide aqui
// el estado de los mandos en un Float64Array, de una vez. Construir los objetos
// desde C (17 botones con 3 propiedades cada uno) eran ~100 llamadas a la API
// de JavaScriptCore: ~300 us por llamada en una RG CubeXX, y RPG Maker la llama
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

// rumble(strong, weak, ms): vibrar, cada motor con su fuerza (0 a 1; 0 ms
// para). Lo usan navigator.vibrate y gamepad.vibrationActuator, en
// runtime/binding/gamepads.js (los patrones y las esperas van en JS)
static JSValue js_pad_rumble(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  AppContext *app = JS_GetContextOpaque(ctx);
  double strong = 0, weak = 0;
  int32_t ms = 0;
  if (argc > 0) JS_ToFloat64(ctx, &strong, argv[0]);
  if (argc > 1) JS_ToFloat64(ctx, &weak, argv[1]);
  if (argc > 2) JS_ToInt32(ctx, &ms, argv[2]);
  backend_vibrate(app->backend, (float)strong, (float)weak, ms);
  return JS_UNDEFINED;
}

// getGamepads esta en runtime/binding/gamepads.js. tools/embed_js.py lo mete en
// el binario al compilar: js_gamepads.h (en obj/gen/) define GAMEPADS_JS, su
// texto. Ver docs/embedded-js.md.
#include "js_gamepads.h"

JSValue js_create_navigator(JSContext *ctx) {
  JSValue navigator = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, navigator, "userAgent", JS_NewString(ctx, "Mozilla/5.0 (Linux) quixjs"));
  JS_SetPropertyStr(ctx, navigator, "platform", JS_NewString(ctx, "Linux"));
  JS_SetPropertyStr(ctx, navigator, "language", JS_NewString(ctx, "en-US"));
  JS_SetPropertyStr(ctx, navigator, "maxTouchPoints", JS_NewInt32(ctx, 0));
  JS_SetPropertyStr(ctx, navigator, "onLine", JS_FALSE);

  // runtime/binding/gamepads.js: lo que necesita, en la global temporal
  // __quixPads; el archivo se llama a si mismo con ella al final y la borra
  JSValue g = JS_GetGlobalObject(ctx);
  JSValue pads = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, pads, "navigator", JS_DupValue(ctx, navigator));
  JS_SetPropertyStr(ctx, pads, "state", JS_NewCFunction(ctx, js_pad_state, "padState", 1));
  JS_SetPropertyStr(ctx, pads, "id", JS_NewCFunction(ctx, js_pad_id, "padId", 1));
  JS_SetPropertyStr(ctx, pads, "rumble", JS_NewCFunction(ctx, js_pad_rumble, "rumble", 3));
  JS_SetPropertyStr(ctx, pads, "axes", JS_NewInt32(ctx, GAMEPAD_AXES));
  JS_SetPropertyStr(ctx, pads, "buttons", JS_NewInt32(ctx, GAMEPAD_BUTTONS));
  JS_SetPropertyStr(ctx, pads, "max", JS_NewInt32(ctx, GAMEPAD_MAX));
  // el reloj de timestamp: performance, o uno propio si aun no existe
  JSValue perf = JS_GetPropertyStr(ctx, g, "performance");
  if (!JS_IsObject(perf)) {
    JS_FreeValue(ctx, perf);
    perf = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, perf, "now", JS_NewCFunction(ctx, js_pad_now, "now", 0));
  }
  JS_SetPropertyStr(ctx, pads, "clock", perf);
  JS_SetPropertyStr(ctx, g, "__quixPads", pads);
  JSValue r = JS_Eval(ctx, GAMEPADS_JS, sizeof(GAMEPADS_JS) - 1, "<quix-gamepads>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(r)) js_std_dump_error(ctx);
  JS_FreeValue(ctx, r);
  JS_FreeValue(ctx, g);
  return navigator;
}
