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

// runtime/binding/quix/quix.c: el objeto global quix (ver quix.h): crearlo,
// lo basico (exit, loading, frame, frameTime, backend, fixedStep, version)
// y agregar cada grupo, que esta en su parte: input.c, gpu.c,
// achievements.c, storage.c, fonts.c, memory.c, lan.c y trace_saves.c; lo
// que comparten, en common.h.

#include "core/app_context.h"
#include "core/timer.h"
#include "game/cheats/cheats.h"
#include "quix.h"
#include "common.h"

static JSValue js_quix_exit(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  AppContext *app = JS_GetContextOpaque(ctx);
  int code = 0;
  if (argc > 0 && JS_ToInt32(ctx, &code, argv[0])) return JS_EXCEPTION;
  app->exit_requested = 1;
  app->exit_code = code;
  return JS_UNDEFINED;
}

static JSValue js_quix_get_frame(JSContext *ctx, JSValueConst this_val) {
  AppContext *app = JS_GetContextOpaque(ctx);
  return JS_NewInt64(ctx, app->frame);
}

// quix.frameTime: el momento programado de la imagen actual (ms, la escala de
// performance.now). A diferencia de performance.now no baila: a 30 fps cada
// imagen esta exactamente 33,333 ms despues de la anterior (66,667 si se
// perdio una). Para que la logica de un juego avance pareja
static JSValue js_quix_get_frame_time(JSContext *ctx, JSValueConst this_val) {
  (void)this_val;
  AppContext *app = JS_GetContextOpaque(ctx);
  return JS_NewFloat64(ctx, app->frame_time);
}

static JSValue js_quix_get_backend(JSContext *ctx, JSValueConst this_val) {
  AppContext *app = JS_GetContextOpaque(ctx);
  return JS_NewString(ctx, backend_name(app->backend));
}

static JSValue js_quix_get_fixed(JSContext *ctx, JSValueConst this_val) {
  return JS_NewBool(ctx, clock_is_fixed());
}

// quix.loading(true | false[, ms]): el aviso de carga ("Loading...") mientras
// el juego carga (una partida, un mapa, una batalla); con false se quita en
// cuanto las imagenes van normales (backend_set_loading). Con ms, solo sale
// si la carga dura mas que eso. Lo usa el hook rpgmaker-mv/port/loading-notice.
static JSValue js_quix_loading(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  AppContext *app = JS_GetContextOpaque(ctx);
  double after = 0;
  if (argc > 1 && JS_ToFloat64(ctx, &after, argv[1]) != 0) after = 0;
  if (after < 0 || after != after) after = 0;
  if (app && app->backend) backend_set_loading(app->backend, argc > 0 && JS_ToBool(ctx, argv[0]), after);
  return JS_UNDEFINED;
}

// La API: un grupo por tema (quix.gpu, quix.fonts...), verbos para lo que
// hace algo y propiedades para los valores (ver docs/usage.md, El objeto quix)
static const JSCFunctionListEntry js_quix_funcs[] = {
  JS_CFUNC_DEF("exit",     1, js_quix_exit),
  JS_CFUNC_DEF("loading", 1, js_quix_loading),
  JS_CGETSET_DEF("frame",     js_quix_get_frame, NULL),
  JS_CGETSET_DEF("frameTime", js_quix_get_frame_time, NULL),
  JS_CGETSET_DEF("backend",   js_quix_get_backend, NULL),
  JS_CGETSET_DEF("fixedStep", js_quix_get_fixed, NULL),
  JS_PROP_STRING_DEF("version", QUIX_VERSION, JS_PROP_CONFIGURABLE),
};

void quix_add_group(JSContext *ctx, JSValueConst quix, const char *name, const JSCFunctionListEntry *list, int n) {
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, o, list, n);
  JS_SetPropertyStr(ctx, quix, name, o);
}

void js_api_add_quix(JSContext *ctx) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue quix = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, quix, js_quix_funcs, countof(js_quix_funcs));
  quix_add_input(ctx, quix);
  quix_add_gpu(ctx, quix);
  quix_add_memory(ctx, quix);
  quix_add_fonts(ctx, quix);
  quix_add_storage(ctx, quix);
  quix_add_state(ctx, quix);
  quix_add_achievements(ctx, quix);   // y quix.steam
  quix_add_lan(ctx, quix);
  // quix.playthrough: la partida en curso, para los trucos (runtime/game/cheats)
  cheats_add_api(ctx, quix);
  // quix.traceSave, quix.tracingSaves y quix.saves.isSave (--trace-saves)
  quix_add_trace_saves(ctx, quix);
  JS_SetPropertyStr(ctx, global, "quix", quix);
  JS_FreeValue(ctx, global);
}
