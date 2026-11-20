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

// Las teclas del backend como KeyboardEvent, con las pulsaciones cortas, y
// las promesas pendientes.
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

static int run_pending_jobs(JSContext *ctx) {
  JSContext *ctx1;
  int err;
  while ((err = JS_ExecutePendingJob(JS_GetRuntime(ctx), &ctx1)) != 0) {
    if (err < 0) {
      js_std_dump_error(ctx1);
      return -1;
    }
  }
  return 0;
}

// Teclas de la cola del backend -> KeyboardEvent en document y window
// Una tecla pulsada y soltada en la misma imagen: el juego no la veria (RPG
// Maker mira el estado de las teclas al actualizar). El keyup de una tecla
// pulsada desde la ultima imagen espera a la siguiente.
#define FRESH_MAX 16
static char       fresh_keys[FRESH_MAX][24];   // keydown desde la ultima imagen
static int        fresh_count = 0;
static InputEvent held_ups[FRESH_MAX];         // keyup que esperan
static int        held_count = 0;

static int key_is_fresh(const char *code) {
  for (int i = 0; i < fresh_count; i++) {
    if (strcmp(fresh_keys[i], code) == 0) return 1;
  }
  return 0;
}

// Tras cada imagen: los keyup que esperaban vuelven a la cola
static void input_frame_done(Backend *backend) {
  fresh_count = 0;
  for (int i = 0; i < held_count; i++) backend_push_input(backend, &held_ups[i]);
  held_count = 0;
}

static int dispatch_input(JSContext *ctx, Backend *backend) {
  int result = 0;
  InputEvent ev;
  while (backend_pop_input(backend, &ev)) {
    if (ev.type == INPUT_KEY_DOWN && !ev.repeat && fresh_count < FRESH_MAX && !key_is_fresh(ev.code)) {
      snprintf(fresh_keys[fresh_count++], sizeof(fresh_keys[0]), "%s", ev.code);
    } else if (ev.type == INPUT_KEY_UP && key_is_fresh(ev.code) && held_count < FRESH_MAX) {
      held_ups[held_count++] = ev;
      continue;
    }
    const char *type = ev.type == INPUT_KEY_DOWN ? "keydown" : "keyup";

    JSValue init = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, init, "key", JS_NewString(ctx, ev.key));
    JS_SetPropertyStr(ctx, init, "code", JS_NewString(ctx, ev.code));
    JS_SetPropertyStr(ctx, init, "keyCode", JS_NewInt32(ctx, ev.key_code));
    JS_SetPropertyStr(ctx, init, "which", JS_NewInt32(ctx, ev.key_code));
    JS_SetPropertyStr(ctx, init, "repeat", JS_NewBool(ctx, ev.repeat));
    JS_SetPropertyStr(ctx, init, "location", JS_NewInt32(ctx, 0));
    JS_SetPropertyStr(ctx, init, "bubbles", JS_TRUE);
    JS_SetPropertyStr(ctx, init, "cancelable", JS_TRUE);
    JS_SetPropertyStr(ctx, init, "ctrlKey", JS_FALSE);
    JS_SetPropertyStr(ctx, init, "shiftKey", JS_FALSE);
    JS_SetPropertyStr(ctx, init, "altKey", JS_FALSE);
    JS_SetPropertyStr(ctx, init, "metaKey", JS_FALSE);
    JS_SetPropertyStr(ctx, init, "isTrusted", JS_TRUE);
    if (ev.synthetic) JS_SetPropertyStr(ctx, init, "quixSynthetic", JS_TRUE);

    JSValue event = events_new_keyboard(ctx, type, init);
    JS_FreeValue(ctx, init);
    if (JS_IsException(event)) {
      js_std_dump_error(ctx);
      return -1;
    }

    // el evento "sube" de document a window
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue document = JS_GetPropertyStr(ctx, global, "document");
    if (events_dispatch(ctx, document, event) != 0) result = -1;
    JSValue stop = JS_GetPropertyStr(ctx, event, "__stop");
    if (!JS_ToBool(ctx, stop)) {
      if (events_dispatch(ctx, global, event) != 0) result = -1;
    }
    JS_FreeValue(ctx, stop);
    JS_FreeValue(ctx, document);
    JS_FreeValue(ctx, global);
    JS_FreeValue(ctx, event);

    if (run_pending_jobs(ctx) != 0) result = -1;
  }
  return result;
}
