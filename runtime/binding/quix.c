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

// El objeto global quix. Ver quix.h.

#include <stdio.h>
#include <sys/resource.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "core/app_context.h"
#include "core/keymap.h"
#include "core/timer.h"
#include "core/url.h"
#include "gfx/font.h"
#include "gfx/gl.h"
#include "gfx/driver.h"
#include "achievements/achievements.h"
#include "canvas.h"
#include "quix.h"
#include "core/trace.h"

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

// quix.pressPad(boton, t): el boton (indice del mapeo "standard") de un mando
// de prueba (backend_test_pad); t es "down", "up" o "press" (por defecto).
// Devuelve la posicion del mando en navigator.getGamepads(), o -1 si el
// backend no tiene mando de prueba (dummy)
static JSValue js_quix_pressPad(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
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

// quix.renderScale(): a que resolucion dibujar el juego ([game] render_scale;
// 1 si no se pidio)
static double render_scale = 1;
void quix_set_render_scale(double scale) {
  render_scale = scale > 0 && scale < 1 ? scale : 1;
}
static JSValue js_quix_renderScale(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_NewFloat64(ctx, render_scale);
}

// quix.isGpuCanvas(canvas): si ese canvas dibuja en la GPU (hook gpu-canvas)
static JSValue js_quix_isGpuCanvas(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  CanvasData *cd = argc > 0 ? js_canvas_get(ctx, argv[0]) : NULL;
  return JS_NewBool(ctx, cd && (cd->gpu != NULL || cd->gpu_wanted));
}

// quix.collectGarbage(): un GC completo de la memoria del JS ahora;
// quix.collectGarbage('due'): el que toca (parcial o completo). Devuelve
// cuanto tardo (ms). Para hacerlo cuando no se nota (un cambio de mapa, con la
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
  double ms = (b.tv_sec - a.tv_sec) * 1000.0 + (b.tv_nsec - a.tv_nsec) / 1e6;
#if QUIX_TRACE
  if (ms >= 1.0) printf("[trace] collectGarbage %s: %.2f ms\n", due ? "due" : "full", ms);
#endif
  return JS_NewFloat64(ctx, ms);
}

// quix.gcPressure(): cuanto falta para que JavaScriptCore pase el GC por su
// cuenta (0 recien hecha, 1 a punto)
static JSValue js_quix_gcPressure(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  return JS_NewFloat64(ctx, JS_GCPressure(JS_GetRuntime(ctx)));
}

// quix.memoryUsage(): lo que da process.memoryUsage() en nw.js. rss y
// maxRss, del proceso (/proc/self/statm y getrusage); heapUsed, heapTotal y
// external, del heap de JavaScriptCore. Recorre el heap: no es para cada
// frame
static JSValue js_quix_memoryUsage(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  (void)argc;
  (void)argv;
  double rss = 0;
  FILE *f = fopen("/proc/self/statm", "r");
  if (f) {
    long size = 0, res = 0;
    if (fscanf(f, "%ld %ld", &size, &res) == 2) rss = (double)res * (double)sysconf(_SC_PAGESIZE);
    fclose(f);
  }
  struct rusage ru;
  double max_rss = getrusage(RUSAGE_SELF, &ru) == 0 ? (double)ru.ru_maxrss * 1024.0 : 0;
  JSMemoryUsage m;
  JS_ComputeMemoryUsage(JS_GetRuntime(ctx), &m);
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "rss", JS_NewFloat64(ctx, rss));
  JS_SetPropertyStr(ctx, o, "maxRss", JS_NewFloat64(ctx, max_rss));
  // heapSize cuenta lo vivo en el ultimo GC: antes del primero es
  // 0, y entonces lo reservado es la mejor lectura
  JS_SetPropertyStr(ctx, o, "heapUsed", JS_NewFloat64(ctx, (double)(m.heap_size > 0 ? m.heap_size : m.heap_capacity)));
  JS_SetPropertyStr(ctx, o, "heapTotal", JS_NewFloat64(ctx, (double)m.heap_capacity));
  JS_SetPropertyStr(ctx, o, "external", JS_NewFloat64(ctx, (double)m.extra_memory));
  return o;
}

// quix.gpuDriver(): el driver de OpenGL ES ("panfrost", "mali", "mali-g31" o "default",
// gfx/drivers/), o null sin WebGL; para que un hook haga
// lo que mejor va con cada uno
static JSValue js_quix_gpuDriver(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val; (void)argc; (void)argv;
  if (!gl_available()) return JS_NULL;
  return JS_NewString(ctx, gpu_driver()->name);
}

// quix.loading(true | false[, ms]): el aviso de carga ("Loading...") mientras
// el juego carga (una partida, un mapa, una batalla); con false se quita en
// cuanto las imagenes van normales (backend_set_loading). Con ms, solo sale
// si la carga dura mas que eso. Lo usa el hook rpgmaker-mv/loading-notice.
static JSValue js_quix_loading(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  AppContext *app = JS_GetContextOpaque(ctx);
  double after = 0;
  if (argc > 1 && JS_ToFloat64(ctx, &after, argv[1]) != 0) after = 0;
  if (after < 0 || after != after) after = 0;
  if (app && app->backend) backend_set_loading(app->backend, argc > 0 && JS_ToBool(ctx, argv[0]), after);
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_quix_funcs[] = {
  JS_CFUNC_DEF("loading", 1, js_quix_loading),
  JS_CFUNC_DEF("gpuDriver", 0, js_quix_gpuDriver),
  JS_CFUNC_DEF("memoryUsage", 0, js_quix_memoryUsage),
  JS_CFUNC_DEF("isGpuCanvas", 1, js_quix_isGpuCanvas),
  JS_CFUNC_DEF("renderScale", 0, js_quix_renderScale),
  JS_CFUNC_DEF("achievementUnlocked", 1, js_quix_achievementUnlocked),
  JS_CFUNC_DEF("achievements", 0, js_quix_achievements),
  JS_CFUNC_DEF("achievementsIcon", 0, js_quix_achievementsIcon),
  JS_CFUNC_DEF("registerFont", 2, js_quix_registerFont),
  JS_CFUNC_DEF("hasFont",      1, js_quix_hasFont),
  JS_CFUNC_DEF("exit",     1, js_quix_exit),
  JS_CFUNC_DEF("collectGarbage", 0, js_quix_collectGarbage),
  JS_CFUNC_DEF("gcPressure", 0, js_quix_gcPressure),
  JS_CFUNC_DEF("pressKey", 2, js_quix_pressKey),
  JS_CFUNC_DEF("pressPad", 2, js_quix_pressPad),
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
