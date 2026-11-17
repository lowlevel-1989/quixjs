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

// El objeto global quix. Ver quix.h.

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "core/app_context.h"
#include "core/keymap.h"
#include "core/timer.h"
#include "core/url.h"
#include "gfx/font.h"
#include "achievements/achievements.h"
#include "canvas.h"
#include "quix.h"

static JSValue js_quix_exit(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  AppContext *app = JS_GetContextOpaque(ctx);
  int code = 0;
  if (argc > 0 && JS_ToInt32(ctx, &code, argv[0])) return JS_EXCEPTION;
  app->exit_requested = 1;
  app->exit_code = code;
  return JS_UNDEFINED;
}

static JSValue js_quix_pressKey(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
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

// quix.registerFont(family, url): la usan FontFace y @font-face
static JSValue js_quix_registerFont(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  AppContext *app = JS_GetContextOpaque(ctx);
  if (argc < 2) return JS_ThrowTypeError(ctx, "registerFont: family and url required");
  const char *family = JS_ToCString(ctx, argv[0]);
  const char *url = JS_ToCString(ctx, argv[1]);
  int ok = 0;
  if (family && url) {
    char path[1024];
    if (url_resolve(app->cfg->game.game_dir, url, path, sizeof(path)) == URL_FILE) {
      font_init();
      ok = font_register(family, path) == 0;
    }
  }
  if (family) JS_FreeCString(ctx, family);
  if (url) JS_FreeCString(ctx, url);
  return JS_NewBool(ctx, ok);
}

static JSValue js_quix_hasFont(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1) return JS_FALSE;
  const char *family = JS_ToCString(ctx, argv[0]);
  if (!family) return JS_EXCEPTION;
  int r = font_is_registered(family);
  JS_FreeCString(ctx, family);
  return JS_NewBool(ctx, r);
}

// quix.achievementUnlocked(id): si el logro del toml del juego ya se consiguio
static JSValue js_quix_achievementUnlocked(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1) return JS_FALSE;
  const char *id = JS_ToCString(ctx, argv[0]);
  if (!id) return JS_EXCEPTION;
  int r = ach_unlocked_by_id(id);
  JS_FreeCString(ctx, id);
  return JS_NewBool(ctx, r);
}

// quix.achievements(): los logros del port, en su orden del toml:
// [{ id, title, description, type, color ("#rrggbb"), unlocked (fecha unix
// en segundos, 0 si no) }]. Para verlos dentro del juego (hook
// rpgmaker-mv/achievements-menu)
static JSValue js_quix_achievements(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  JSValue list = JS_NewArray(ctx);
  for (int i = 0; i < ach_count(); i++) {
    JSValue o = JS_NewObject(ctx);
    char color[8];
    snprintf(color, sizeof color, "#%06x", ach_color(i) & 0xffffff);
    JS_SetPropertyStr(ctx, o, "id", JS_NewString(ctx, ach_id(i)));
    JS_SetPropertyStr(ctx, o, "title", JS_NewString(ctx, ach_title(i)));
    JS_SetPropertyStr(ctx, o, "description", JS_NewString(ctx, ach_description(i) ? ach_description(i) : ""));
    JS_SetPropertyStr(ctx, o, "type", JS_NewString(ctx, ach_type(i) ? ach_type(i) : ""));
    JS_SetPropertyStr(ctx, o, "color", JS_NewString(ctx, color));
    JS_SetPropertyStr(ctx, o, "unlocked", JS_NewInt64(ctx, ach_unlocked(i)));
    JS_SetPropertyUint32(ctx, list, i, o);
  }
  return list;
}

// quix.achievementsIcon(): [achievements_menu] icon del toml (0 si no hay)
static JSValue js_quix_achievementsIcon(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_NewInt64(ctx, ach_menu_icon());
}

// quix.isGpuCanvas(canvas): si ese canvas dibuja en la GPU (hook gpu-canvas)
static JSValue js_quix_isGpuCanvas(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  CanvasData *cd = argc > 0 ? js_canvas_get(ctx, argv[0]) : NULL;
  return JS_NewBool(ctx, cd && (cd->gpu != NULL || cd->gpu_wanted));
}

// quix.collectGarbage(): una recoleccion completa de la memoria del JS ahora;
// quix.collectGarbage('due'): la que toca (parcial o completa). Devuelve
// cuanto tardo (ms). Para hacerla cuando no se nota (un cambio de mapa, con la
// pantalla en negro; el personaje quieto) y no a mitad de un desplazamiento
static JSValue js_quix_collectGarbage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  int due = 0;
  if (argc > 0 && JS_IsString(argv[0])) {
    const char *s = JS_ToCString(ctx, argv[0]);
    due = s && strcmp(s, "due") == 0;
    if (s) JS_FreeCString(ctx, s);
  }
  struct timespec a, b;
  clock_gettime(CLOCK_MONOTONIC, &a);
  if (due) JS_CollectDue(JS_GetRuntime(ctx));
  else JS_RunGC(JS_GetRuntime(ctx));
  clock_gettime(CLOCK_MONOTONIC, &b);
  return JS_NewFloat64(ctx, (b.tv_sec - a.tv_sec) * 1000.0 + (b.tv_nsec - a.tv_nsec) / 1e6);
}

// quix.gcPressure(): cuanto falta para que JavaScriptCore recolecte por su
// cuenta (0 recien hecha, 1 a punto)
static JSValue js_quix_gcPressure(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_NewFloat64(ctx, JS_GCPressure(JS_GetRuntime(ctx)));
}

static const JSCFunctionListEntry js_quix_funcs[] = {
  JS_CFUNC_DEF("isGpuCanvas", 1, js_quix_isGpuCanvas),
  JS_CFUNC_DEF("achievementUnlocked", 1, js_quix_achievementUnlocked),
  JS_CFUNC_DEF("achievements", 0, js_quix_achievements),
  JS_CFUNC_DEF("achievementsIcon", 0, js_quix_achievementsIcon),
  JS_CFUNC_DEF("registerFont", 2, js_quix_registerFont),
  JS_CFUNC_DEF("hasFont",      1, js_quix_hasFont),
  JS_CFUNC_DEF("exit",     1, js_quix_exit),
  JS_CFUNC_DEF("collectGarbage", 0, js_quix_collectGarbage),
  JS_CFUNC_DEF("gcPressure", 0, js_quix_gcPressure),
  JS_CFUNC_DEF("pressKey", 2, js_quix_pressKey),
  JS_CGETSET_DEF("frame",     js_quix_get_frame, NULL),
  JS_CGETSET_DEF("frameTime", js_quix_get_frame_time, NULL),
  JS_CGETSET_DEF("backend",   js_quix_get_backend, NULL),
  JS_CGETSET_DEF("fixedStep", js_quix_get_fixed, NULL),
  JS_PROP_STRING_DEF("version", QUIX_VERSION, JS_PROP_CONFIGURABLE),
};

void js_api_add_quix(JSContext *ctx) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue quix = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, quix, js_quix_funcs, countof(js_quix_funcs));
  JS_SetPropertyStr(ctx, global, "quix", quix);
  JS_FreeValue(ctx, global);
}
