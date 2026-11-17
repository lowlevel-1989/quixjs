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

// El programa: lee las opciones y el toml del juego, crea el backend y el
// motor de JS, carga los scripts y corre el bucle principal (eventos,
// timers, requestAnimationFrame, presentar y esperar al siguiente frame).

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "platform/platform.h"
#include "core/url.h"
#include "symbols/symbols.h"
#include "cheats/cheats.h"
#include "backend/toast.h"
#include "js/js.h"
#include "core/args.h"
#include "core/config_loader.h"
#include "core/printer.h"
#include "core/compiler.h"
#include "core/runner.h"
#include "core/timer.h"
#include "core/app_context.h"
#include "core/init.h"
#include "core/input_script.h"
#include "core/debug_server.h"
#include "binding/timer.h"
#include "binding/events.h"
#include "binding/canvas.h"
#include "core/worker.h"
#include "binding/image.h"
#include "binding/document.h"
#include "binding/window.h"
#include "binding/xhr.h"
#include "binding/quix.h"
#include "binding/polyfill.h"
#include "binding/script.h"
#include "binding/nwjs.h"
#include "binding/blob.h"
#include "binding/audio.h"
#include "binding/webgl.h"
#include "binding/video.h"
#include "gfx/gl.h"
#include "audio/mixer.h"
#include "backend/backend.h"
#include "gfx/image_loader.h"
#include "gfx/raster.h"
#include "gfx/font.h"
#include "launcher/launcher.h"
#include "launcher/ui.h"
#include "launcher/settings.h"
#include "transfer/saves.h"
#include "achievements/achievements.h"
#include "hooks/hooks.h"
#include "engines/engine_api.h"
#include "core/profiler.h"
#include "core/watchdog.h"
#include "core/memstats.h"
#include "binding/console.h"

#define EXIT_TEST_TIMEOUT 3
#define MAX_SCREEN_CANVASES 8

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

// Junta los canvas visibles del documento en un frame. Con un solo canvas
// se usa su superficie directamente (sin copias).
static const Surface *compose_screen(JSContext *ctx, Surface *scratch) {
  CanvasData *list[MAX_SCREEN_CANVASES];
  int n = document_screen_canvases(ctx, list, MAX_SCREEN_CANVASES);
  if (n == 0) return NULL;
  for (int i = 0; i < n; i++) {
    if (list[i]->webgl) webgl_canvas_sync(list[i]);   // lo dibujado con WebGL
    canvas_sync_cpu(list[i]);                          // lo dibujado con gpu-canvas
  }
  if (n == 1) return &list[0]->surface;

  const Surface *base = &list[0]->surface;
  if (scratch->w != base->w || scratch->h != base->h) {
    if (surface_resize(scratch, base->w, base->h) != 0) return base;
  }
  memcpy(scratch->px, base->px, (size_t)base->w * base->h * 4);

  Matrix identity;
  matrix_identity(&identity);
  DrawParams dp = { .alpha = 1.0, .op = OP_SOURCE_OVER, .clip = NULL };
  for (int i = 1; i < n; i++) {
    const Surface *s = &list[i]->surface;
    raster_draw_image(scratch, s, 0, 0, s->w, s->h, 0, 0, s->w, s->h, &identity, &dp);
  }
  return scratch;
}

// --fps: frames por segundo con el reloj real (aunque el juego use el fijo)
typedef struct {
  int     enabled;
  double  first;       // inicio de la medida (para la media)
  double  last;        // ultimo frame dibujado
  long    total;
  double  window_start;
  int     frames;
  double  value;
  double  js_ms, present_ms;   // suma desde la ultima linea
  double  other_ms;            // JS fuera del frame: timers, promesas, eventos
  double  sleep_ms;            // esperando al siguiente frame
  double  events_ms;           // eventos del backend y consola de debug
  double  wall_ms, prev_tick;  // tiempo real entre frames
  double  worst_ms;            // el frame mas largo del intervalo (los tirones)
  // en que se fue el tiempo desde el frame anterior, y el reparto del peor
  // frame del intervalo (lo escribe la linea FPS:, para explicar la caida)
  double  fr_timers, fr_events, fr_idle, prev_present;
  double  worst_parts[6];      // raf, state, present, timers, events, idle
  double  last_gap;            // tiempo real desde el frame anterior
  int     total_window;
} FpsMeter;

// cuanto antes del refresco al que apunta se entrega cada imagen (ms)
#define VB_LEAD 12.0

static double real_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static void fps_tick(FpsMeter *f) {
  double now = real_ms();
  if (f->window_start == 0) f->window_start = now;
  if (f->first == 0) f->first = now;
  if (f->prev_tick > 0) {
    f->wall_ms += now - f->prev_tick;
    if (now - f->prev_tick > f->worst_ms) f->worst_ms = now - f->prev_tick;
    f->last_gap = now - f->prev_tick;
  }
  f->prev_tick = now;
  f->frames++;
  f->total++;
  f->last = now;
  // cada medio segundo en pantalla; una vez por segundo en la salida
  static double last_print = 0;
  if (now - f->window_start >= 500) {
    f->value = f->frames * 1000.0 / (now - f->window_start);
    f->frames = 0;
    f->window_start = now;
    if (now - last_print >= 1000) {
      // por frame: JS (timers + rAF), compose (incluye leer la GPU) y present
      int nf = f->total_window > 0 ? f->total_window : 1;
      double known = (f->js_ms + f->present_ms + f->other_ms + f->sleep_ms + f->events_ms) / nf;
      // en la salida, la media de todo el intervalo (no la de la ultima ventana:
      // un juego con un ciclo de ~1 s caeria siempre en la misma parte)
      double avg = f->wall_ms > 0 ? nf * 1000.0 / f->wall_ms : f->value;
      printf("FPS: %.1f (worst %.0f ms; js %.1f ms, present %.1f ms, timers %.1f ms, events %.1f ms, idle %.1f ms, rest %.1f ms)\n",
             avg, f->worst_ms, f->js_ms / nf, f->present_ms / nf, f->other_ms / nf, f->events_ms / nf,
             f->sleep_ms / nf, f->wall_ms / nf - known);
      // el peor frame, por partes: raf (el JS del frame), state (video, audio,
      // simbolos, trucos, logros), present (el del frame anterior), timers y
      // events (entre frames), idle (esperando) y el resto
      const double *w = f->worst_parts;
      double wsum = w[0] + w[1] + w[2] + w[3] + w[4] + w[5];
      printf("      worst %.0f ms = raf %.1f, state %.1f, present %.1f, timers %.1f, events %.1f, idle %.1f, rest %.1f\n",
             f->worst_ms, w[0], w[1], w[2], w[3], w[4], w[5], f->worst_ms - wsum);
      f->js_ms = f->present_ms = f->other_ms = f->sleep_ms = f->events_ms = f->wall_ms = f->worst_ms = 0;
      f->total_window = 0;
      last_print = now;
    }
  } else if (f->value == 0 && f->total > 1 && now > f->first) {
    // todavia sin medio segundo: la media hasta ahora
    f->value = (f->total - 1) * 1000.0 / (now - f->first);
  }
}

// El texto del contador para el backend (lo dibuja arriba a la derecha)
static void fps_overlay(FpsMeter *f, Backend *backend) {
  if (f->value > 0) snprintf(backend->overlay, sizeof(backend->overlay), "%.1f fps", f->value);
  else snprintf(backend->overlay, sizeof(backend->overlay), "-- fps");
}

// Muestra el frame. Si en pantalla hay un solo canvas y es WebGL, su textura
// va directa al backend (sin leerla de la GPU); si no, se compone en memoria.
static void present_screen(JSContext *ctx, Backend *backend, Surface *scratch) {
  // con GPU (backend sdl): cada canvas es una capa; los WebGL sin leerlos y
  // los 2d solo se suben si cambiaron
  if (backend_can_present_layers(backend)) {
    CanvasData *list[MAX_SCREEN_CANVASES];
    PresentLayer layers[MAX_SCREEN_CANVASES];
    int n = document_screen_canvases(ctx, list, MAX_SCREEN_CANVASES);
    int m = 0;
    for (int i = 0; i < n; i++) {
      CanvasData *cd = list[i];
      PresentLayer *l = &layers[m];
      memset(l, 0, sizeof(*l));
      l->key = cd;
      if (cd->webgl) {
        // entregada al hilo que presenta: el canvas sigue en otra textura
        if (backend->present_async) l->gl_tex = webgl_canvas_handoff(cd, &l->w, &l->h, &l->premultiplied);
        if (!l->gl_tex) {
          backend->present_async = 0;
          l->gl_tex = webgl_canvas_texture(cd, &l->w, &l->h, &l->premultiplied);
        }
        if (!l->gl_tex) continue;
      } else {
        canvas_sync_cpu(cd);   // gpu-canvas en pantalla (raro): se lee
        l->rgba = cd->surface.px;
        l->w = cd->surface.w;
        l->h = cd->surface.h;
        l->dirty = cd->dirty;
        cd->dirty = 0;
      }
      m++;
    }
    // un <video> que se ve: encima de todo
    int vchanged = 0;
    const Surface *vs = video_screen(&vchanged);
    if (vs && m < MAX_SCREEN_CANVASES) {
      PresentLayer *l = &layers[m++];
      memset(l, 0, sizeof(*l));
      l->key = vs;
      l->rgba = vs->px;
      l->w = vs->w;
      l->h = vs->h;
      l->dirty = vchanged;
    }
    backend_present_layers(backend, layers, m);
    return;
  }
  const Surface *screen = compose_screen(ctx, scratch);
  if (screen) backend_present(backend, screen->px, screen->w, screen->h);
}

typedef struct {
  JSContext *ctx;
  Surface   *scratch;
} ShotCtx;

static const Surface *script_screen(void *user) {
  ShotCtx *sc = user;
  return compose_screen(sc->ctx, sc->scratch);
}

static void sleep_ms(double ms) {
  if (ms <= 0) return;
  if (ms > 16) ms = 16;   // sin dormir mucho: los eventos tienen que llegar rapido
  usleep((useconds_t)(ms * 1000));
}


int main(int argc, char **argv) {
  // salida por lineas: los mensajes y los errores salen en orden en los logs
  setvbuf(stdout, NULL, _IOLBF, 0);

  Args args;
  if (parse_args(argc, argv, &args) != 0) {
    print_help();
    return 1;
  }

  if (args.show_help) {
    print_help();
    return 0;
  }

  if (args.show_version) {
    printf("quixjs v%s\n", QUIX_VERSION);
    return 0;
  }

  // --gen-config escribe el toml por la salida: sin nada mas
  if (args.init_dir) return init_generate(args.init_dir);

  // la version, la primera linea de los logs (para saber cual usa cada uno)
  printf("quixjs v%s\n", QUIX_VERSION);
  printf("\t\thttps://github.com/lowlevel-1989/quixjs\n");

  // los ajustes del malloc de la plataforma (en glibc: los bloques grandes
  // vuelven al sistema al liberarlos)
  platform_mem_init();

  printf("Using config: %s\n", args.config_path);

  Config cfg = {0};
  if (config_load(&cfg, args.config_path) != 0) {
    fprintf(stderr, "Failed to load config file: %s\n", args.config_path);
    return 1;
  }

  print_config(&cfg);

  clock_init(args.fixed_step);

  // Crear el backend seleccionado
  Backend* backend = backend_create(args.backend);
  if (!backend) {
    fprintf(stderr, "Failed to create backend: %d\n", args.backend);
    return 1;
  }

  backend->fullscreen = args.fullscreen;
  backend->resizable = args.resizable;
  backend->no_vsync = !cfg.engine.vsync;
  backend->present_ahead = cfg.engine.present_ahead;
  // controller.map junto al toml del juego (Controls del lanzador)
  const char *cfg_slash = strrchr(args.config_path, '/');
  if (cfg_slash) snprintf(backend->controller_map, sizeof backend->controller_map, "%.*s/controller.map",
                          (int)(cfg_slash - args.config_path), args.config_path);
  else snprintf(backend->controller_map, sizeof backend->controller_map, "controller.map");
  backend_init(backend, cfg.game.width, cfg.game.height);
  // Select + Start sale aunque el juego no responda (solo con ventana)
  if (args.backend == BACKEND_SDL) watchdog_start();

  // [saves] y los logros: los usan el lanzador y el juego
  const char *patterns[MAX_SAVE_PATTERNS];
  for (int i = 0; i < cfg.saves.files_count; i++) patterns[i] = cfg.saves.files[i];
  // dir: la carpeta de las partidas, como game_dir (sin dir, game_dir)
  const char *saves_dir = cfg.saves.dir[0] ? cfg.saves.dir : cfg.game.game_dir;
  int npatterns = cfg.saves.files_count;
  // los logros van con las partidas: en su carpeta, y Transfer Save los manda
  // con ellas (el que llega pisa al que habia, como una partida)
  const char *ach_file = cfg.saves.achievements;
  if (ach_file[0] && ach_file[0] != '/') {
    if (npatterns > 0 && npatterns < MAX_SAVE_PATTERNS) patterns[npatterns++] = ach_file;
    // antes se guardaban en game_dir: se mueven la primera vez
    char old_path[MAX_PATH_LEN * 2], new_path[MAX_PATH_LEN * 2];
    snprintf(old_path, sizeof old_path, "%s/%s", cfg.game.game_dir, ach_file);
    snprintf(new_path, sizeof new_path, "%s/%s", saves_dir, ach_file);
    if (strcmp(old_path, new_path) != 0 && access(new_path, F_OK) != 0 && access(old_path, F_OK) == 0 &&
        rename(old_path, new_path) == 0) {
      printf("[achievements] moved to %s (they go with the saves)\n", new_path);
    }
  }
  saves_setup(saves_dir, patterns, npatterns);
  // [symbols] antes que los logros y los trucos, que los usan
  int bad_state = symbols_load(args.config_path) < 0;
  const char *game_id = cfg.saves.id[0] ? cfg.saves.id : cfg.launcher.title[0] ? cfg.launcher.title : cfg.engine.name;
  bad_state |= ach_load(args.config_path, saves_dir, cfg.saves.achievements, game_id) < 0;
  bad_state |= cheats_load(args.config_path) < 0;
  if ((bad_state || hooks_load(args.config_path) < 0) && args.test) {
    backend_cleanup(backend);
    free(backend);
    return 1;
  }

  // --script tambien maneja el lanzador: sus frames van antes que los del juego
  if (args.script && input_script_load(args.script) != 0) {
    backend_cleanup(backend);
    free(backend);
    return 1;
  }

  // los ajustes del jugador (Options del lanzador)
  Settings settings;
  settings_load(&settings, args.config_path);
  settings_apply_cheats(&settings);
#ifndef QUIX_HANDHELD
  // Options: a pantalla completa (--fullscreen ya la pone al crear la ventana)
  if (settings.fullscreen && !backend->fullscreen) backend_set_fullscreen(backend, 1);
#endif

  long script_offset = 0;
  if (args.launcher) {
    int code = 0;
    int play = launcher_run(backend, &cfg, args.config_path, &settings, &code) == LAUNCHER_PLAY;
    script_offset = ui_frames();
    if (!play) {
      input_script_free();
      font_quit();
      backend_cleanup(backend);
      free(backend);
      return code;
    }
  }

  // los mods activos: los elegidos en el lanzador (Mods, en
  // quixjs-settings.toml) o, sin eso, [game] mods. Antes de leer el juego
  {
    char (*names)[64] = settings.mods_set ? settings.mods : cfg.mods.names;
    int count = settings.mods_set ? settings.mods_count : cfg.mods.count;
    if (count > 0) {
      url_set_mods(cfg.game.game_dir, cfg.mods.root, names, count);
      printf("[mods]");
      for (int i = 0; i < count; i++) printf(" %s", names[i]);
      printf(" (%s, over %s)\n", cfg.mods.root, cfg.game.game_dir);
    }
  }

  ScriptBundle bundle = {0};

  // [engine] js_memory: la RAM que JavaScriptCore cree tener. Con menos,
  // recolecta antes y su monton crece menos (antes de crear el motor; un
  // JSC_forceRAMSize del entorno manda)
  if (cfg.engine.js_memory > 0) {
    char bytes[32];
    snprintf(bytes, sizeof(bytes), "%lld", (long long)cfg.engine.js_memory * 1024 * 1024);
    setenv("JSC_forceRAMSize", bytes, 0);
    printf("JavaScriptCore memory: %d MB\n", cfg.engine.js_memory);
  }

  // el profiler de JSC: una muestra cada 10 ms (con 1 ms, en una portatil
  // pesaba mas que el juego)
  setenv("JSC_sampleInterval", "10000", 0);

  JSRuntime *rt = JS_NewRuntime();
  if (!rt) {
    fprintf(stderr, "Failed to create JS runtime\n");
    backend_cleanup(backend);
    free(backend);
    return 1;
  }

  JSContext *ctx = JS_NewContext(rt);
  if (!ctx) {
    fprintf(stderr, "Failed to create JS context\n");
    JS_FreeRuntime(rt);
    backend_cleanup(backend);
    free(backend);
    return 1;
  }
  js_api_add_console(ctx);

  AppContext app = {0};
  app.backend = backend;
  app.cfg = &cfg;
  JS_SetContextOpaque(ctx, &app);

  webgl_set_enabled(cfg.engine.webgl && !args.no_webgl, args.backend != BACKEND_SDL);

  // El orden importa: eventos y DOM base antes que canvas e imagenes
  js_api_add_events(ctx);
  js_api_add_timer(ctx);
  js_api_add_canvas(ctx);
  js_api_add_context2d(ctx);
  js_api_add_webgl(ctx);
  js_api_add_image(ctx);
  js_api_add_window(ctx);
  js_api_add_xhr(ctx);
  js_api_add_blob(ctx);
  js_api_add_quix(ctx);
  js_api_add_video(ctx);
  hooks_add_api(ctx);
  engines_add_api(ctx);

  // [fonts] del toml del juego
  for (int i = 0; i < cfg.fonts_count; i++) {
    char path[1024];
    snprintf(path, sizeof(path), "%s/%s", cfg.game.game_dir, cfg.fonts[i].path);
    font_init();
    if (font_register(cfg.fonts[i].family, path) == 0) printf("Font: %s -> %s\n", cfg.fonts[i].family, path);
  }

  int exit_code = 0;
  int failed = js_api_add_polyfill(ctx) != 0;
  if (!failed) failed = js_api_add_audio(ctx, args.backend == BACKEND_SDL) != 0;
  if (!failed && cfg.engine.nwjs) failed = js_api_add_nwjs(ctx) != 0;

  if (!failed && compile_scripts(ctx, &cfg, &bundle) != 0) {
    fprintf(stderr, "Failed to compile scripts\n");
    failed = 1;
  }

  if (!failed) script_add_static(ctx, bundle.filenames, bundle.count);
  if (!failed && runner_run_bundle(ctx, &bundle, script_before_static) != 0) {
    fprintf(stderr, "Error executing bytecode\n");
    failed = 1;
  }
  // los hooks de las funciones que ya existen (los de los plugins, en cuanto se carguen)
  if (!failed && hooks_pending()) hooks_install(ctx);
  // DOMContentLoaded y window.onload, despues de los <script> del arranque
  if (!failed) script_document_loaded(ctx);

  Surface scratch;
  surface_init(&scratch, 0, 0);
  ShotCtx shot_ctx = { ctx, &scratch };
  FpsMeter fps = { 0 };
  fps.enabled = args.show_fps || settings.show_fps;
  // Options > Profiler: como --profile, junto al toml del juego
  static char profile_path[1024];
  if (!args.profile && settings.profile) {
    const char *slash = strrchr(args.config_path, '/');
    if (slash) snprintf(profile_path, sizeof(profile_path), "%.*s/quixjs-profile.txt", (int)(slash - args.config_path), args.config_path);
    else snprintf(profile_path, sizeof(profile_path), "quixjs-profile.txt");
    args.profile = profile_path;
  }
  // el tope de fps: el del toml (lo decide el port: en el lanzador no se
  // cambia); --frame-rate para probar
  if (args.frame_rate > 0) cfg.engine.fps = args.frame_rate;
  if (!failed && args.debug_sock && debug_init(args.debug_sock) != 0) failed = 1;
  DebugEnv debug_env = { ctx, backend, &app.frame, script_screen, &shot_ctx, &cfg.engine.fps };
  // --profile: el profiler desde el arranque y el informe en un archivo
  double next_profile_save = 0;
  double next_profile_drain = 0;
  double next_malloc_trim = 0;
  double next_mem_check = 0, next_mem_rescue = 0;
  if (!failed && args.profile && profiler_start(ctx) == 0) {
    printf("Profiling: report in %s every 10 s\n", args.profile);
    next_profile_save = real_ms() + 10000;
  }
  double next_frame = clock_now_ms();
  // [engine] fps: cada cuanto corre requestAnimationFrame (y se presenta)
  double frame_ms = 1000.0 / (cfg.engine.fps > 0 ? cfg.engine.fps : 60);
  if (cfg.engine.fps != 60) printf("Frame rate: %d fps\n", cfg.engine.fps);
  int keep_window = args.backend == BACKEND_SDL;

  // Bucle principal
  while (!failed) {
    double t_events = fps.enabled ? real_ms() : 0;
    watchdog_alive();
    // las muestras del profiler: una vez por segundo (JavaScriptCore las
    // guarda entre medias; recogerlas y leer su JSON en cada vuelta costaba
    // decenas de ms por imagen en la consola)
    // lo libre del monton de malloc, de vuelta al sistema (cada 10 s)
    if (real_ms() >= next_malloc_trim) {
      platform_mem_trim();
      next_malloc_trim = real_ms() + 10000;
    }
    // guardian de memoria: si al sistema le queda poca RAM (MemAvailable por
    // debajo de [engine] low_memory_mb), una recoleccion completa de JS y lo
    // libre de malloc de vuelta, como mucho cada 5 s. Sin esto, en una
    // portatil de 1 GB el sistema empezaba a releer el programa del disco
    // (tirones de mas de 1 s) y acababa bloqueado.
    if (cfg.engine.low_memory_mb > 0 && real_ms() >= next_mem_check) {
      next_mem_check = real_ms() + 1000;
      long avail = platform_mem_available_mb();
      if (avail >= 0 && avail < cfg.engine.low_memory_mb && real_ms() >= next_mem_rescue) {
        double g0 = real_ms();
        JS_RunGC(rt);
        platform_mem_trim();
        long after = platform_mem_available_mb();
        printf("[memory] %ld MB left: full collection (%.0f ms), now %ld MB\n", avail, real_ms() - g0, after);
        next_mem_rescue = real_ms() + 5000;
      }
    }
    if (profiler_running() && real_ms() >= next_profile_drain) {
      profiler_drain(ctx);
      next_profile_drain = real_ms() + 1000;
    }
    if (args.profile && next_profile_save > 0 && real_ms() >= next_profile_save) {
      profiler_save(ctx, args.profile, 60);
      next_profile_save = real_ms() + 10000;
    }
    profiler_phase("events");
    backend_process_events(backend);

    // consola de debug: entre frames, sin JS corriendo
    if (args.debug_sock) {
      if (debug_poll(&debug_env)) break;
      if (debug_paused()) {
        // en pausa: la ventana sigue respondiendo y mostrando el ultimo frame
        if (backend_should_close(backend)) break;
        present_screen(ctx, backend, &scratch);
        sleep_ms(16);
        continue;
      }
    }

    if (fps.enabled) {
      fps.events_ms += real_ms() - t_events;
      fps.fr_events += real_ms() - t_events;
    }

    // guion de entrada (--script): teclas y capturas del frame actual
    if (args.script) {
      int code = 0;
      if (input_script_run(app.frame + script_offset, backend, script_screen, &shot_ctx, &code)) {
        app.exit_requested = 1;
        app.exit_code = code;
        break;
      }
    }

    double t_other = fps.enabled ? real_ms() : 0;
    profiler_phase("timers");
    int err = dispatch_input(ctx, backend);
    if (err == 0) err = run_pending_jobs(ctx);
    worker_poll(ctx);   // lo cargado en segundo plano (imagenes, sonidos)
    if (err == 0) err = timer_run_due(ctx);
    if (fps.enabled) {
      fps.other_ms += real_ms() - t_other;
      fps.fr_timers += real_ms() - t_other;
    }

    double now = clock_now_ms();
    // el refresco al que apunta la ultima imagen (ms, CLOCK_MONOTONIC; 0 sin
    // refrescos) y cuantos dura cada imagen
    static double vb_target = 0;
    static int vb_frames = 0;
    if (err == 0 && now >= next_frame) {
      double t0 = fps.enabled ? real_ms() : 0;
      double frame_slot = next_frame;   // el momento que le tocaba a esta imagen
      // quix.frameTime: el refresco al que apuntara esta imagen (constante de
      // una a otra), o el momento que le tocaba sin refrescos
      if (vb_target > 0) {
        double vlast, vper;
        if (platform_vblank(&vlast, &vper) != 0) vper = 1000.0 / 60;
        app.frame_time = vb_target + vb_frames * vper + (clock_now_ms() - real_ms());
      } else {
        app.frame_time = frame_slot;
      }
      if (hooks_pending()) hooks_install(ctx);
      static unsigned hook_frames = 0;
      if (++hook_frames % 30 == 0) hooks_check(ctx);
      profiler_phase("frame (native)");
      err = timer_run_frame(ctx, now);
      double t_raf = fps.enabled ? real_ms() : 0;
      input_frame_done(backend);
      profiler_phase("video");
      video_tick();
      audio_tick(ctx);
      // el estado del juego: los simbolos, los trucos activos, los logros y
      // los avisos (en este orden)
      symbols_tick(ctx);
      cheats_tick(ctx);
      ach_tick(ctx, backend, real_ms());
      toast_tick(backend, real_ms());

      double t1 = fps.enabled ? real_ms() : 0;
      if (fps.enabled) {
        // el reparto de este frame, si es el peor del intervalo (antes de
        // fps_tick, que puede escribir la linea)
        if (fps.prev_tick > 0 && t1 - fps.prev_tick >= fps.worst_ms) {
          double parts[6] = { t_raf - t0, t1 - t_raf, fps.prev_present, fps.fr_timers, fps.fr_events, fps.fr_idle };
          memcpy(fps.worst_parts, parts, sizeof parts);
        }
        fps.fr_timers = fps.fr_events = fps.fr_idle = 0;
        fps_tick(&fps);
        fps_overlay(&fps, backend);
      }
      // Ritmo fijo en pantalla. Con la pantalla a 60 Hz y el juego a 30, si las
      // imagenes llegan unas antes y otras despues de un refresco, unas duran
      // 16,7 ms y otras 50: se ve a saltitos, o a medio camino, aunque vaya a 30.
      //
      // Con los refrescos de la pantalla (platform_vblank) y el juego a la mitad
      // del refresco o menos: cada imagen apunta a un refresco (el anterior mas
      // N, en la rejilla real de refrescos) y se entrega VB_LEAD ms antes. Con
      // Weston (el compositor Wayland de algunas portatiles) cae entre el refresco intermedio y el suyo (Weston prepara cada
      // uno ~7 ms antes); sin Weston (KMSDRM) el cambio es en el refresco
      // siguiente a la entrega, que es el suyo. Asi dura siempre N refrescos.
      // Si llega tarde, apunta al siguiente refresco. Sin refrescos (o a 60
      // fps, donde el vsync basta), en un punto fijo de su turno: cuando estan
      // listas casi todas las imagenes (el mas alto de las ultimas, que baja
      // poco a poco) mas 2 ms
      static double ready_ms = 0;
      double ready = clock_now_ms() - frame_slot;
      ready_ms = ready > ready_ms ? ready : ready_ms * 0.995 + ready * 0.005;
      if (cfg.engine.vsync && !clock_is_fixed()) {
        double present_at = frame_slot + (ready_ms + 2 < frame_ms - 3 ? ready_ms + 2 : frame_ms - 3);
        double vlast, vper;
        int vb_n = 0;
        // solo si los fps son una fraccion exacta del refresco y los
        // refrescos del sistema son los de la pantalla de la ventana (con
        // varias pantallas, la consulta podria ser de otra)
        if (platform_vblank(&vlast, &vper) == 0 && vper > 4) {
          double ratio = frame_ms / vper, hz = backend_refresh_rate(backend);
          vb_n = (int)(ratio + 0.5);
          if (fabs(ratio - vb_n) > 0.05 || (hz > 0 && fabs(hz - 1000.0 / vper) > 1.5)) vb_n = 0;
        }
        // con el hilo que presenta (present_ahead): esta imagen se entrega
        // cuando ya se haya mostrado la anterior (en su refresco menos
        // VB_LEAD) y el juego sigue sin esperar. Hasta entonces, el hueco
        // es para el recolector.
        int ahead = backend->present_thread && vb_n >= 2;
        double t_hold = fps.enabled ? real_ms() : 0;
        if (ahead) {
          double gap = vb_target > 0 ? vb_target - VB_LEAD - real_ms() : 0;
          if (gap > 3) JS_IdleWork(JS_GetRuntime(ctx), gap - 1.5);
          backend_present_wait(backend);
        }
        backend->present_async = 0;
        backend->present_at = 0;
        if (vb_n >= 2) {
          double r = real_ms();
          double target = vb_target > 0 ? vb_target + vb_n * vper : vlast + vb_n * vper;
          target = vlast + ceil((target - vlast) / vper - 0.5) * vper;   // en la rejilla de refrescos
          while (target - VB_LEAD < r) target += vper;                     // tarde: el siguiente
          vb_target = target;
          vb_frames = vb_n;
          present_at = target - VB_LEAD + (clock_now_ms() - r);           // a la escala del bucle
        } else {
          vb_target = 0;
        }
        if (ahead && vb_target > 0) {
          backend->present_async = 1;
          backend->present_at = vb_target - VB_LEAD;
        } else {
          // el hueco hasta presentar: para el recolector (lo que deja pendiente
          // lo haria si no en medio del JavaScript de las imagenes siguientes)
          if (present_at - clock_now_ms() > 3) JS_IdleWork(JS_GetRuntime(ctx), present_at - clock_now_ms() - 1.5);
          while (clock_now_ms() < present_at) sleep_ms(present_at - clock_now_ms());
        }
        if (fps.enabled) fps.fr_idle += real_ms() - t_hold;
      }
      profiler_phase("present");
      present_screen(ctx, backend, &scratch);
      if (args.debug_sock) {
        // cuando se mostraron de verdad (con el hilo que presenta, mas tarde)
        double shown[64];
        int k = backend_presented(backend, shown, 64);
        if (k < 0) debug_note_present(real_ms());
        for (int i = 0; i < k; i++) debug_note_present(shown[i]);
      }
      profiler_phase("other");
      if (fps.enabled) {
        double present = real_ms() - t1;
        fps.prev_present = present;
        fps.js_ms += t1 - t0;
        fps.present_ms += present;   // componer o leer de la GPU + mostrar
        fps.total_window++;
        // un tiron (mas de 100 ms desde el frame anterior): que lo explique el
        // log (el JS de este frame, el dibujado y las imagenes decodificadas)
        static long dec_count = 0;
        static double dec_ms = 0;
        if (fps.last_gap > 100) {
          printf("[slow frame] %.0f ms: js %.1f ms, present %.1f ms, images decoded %ld (%.0f ms)\n", fps.last_gap,
                 t1 - t0, present, quix_mem.decode_count - dec_count, quix_mem.decode_ms - dec_ms);
        }
        dec_count = quix_mem.decode_count;
        dec_ms = quix_mem.decode_ms;
      }

      app.frame++;
      if (args.debug_sock) debug_frame_done();
      // el tope puede cambiar en caliente (comando fps de la consola de debug)
      frame_ms = 1000.0 / (cfg.engine.fps > 0 ? cfg.engine.fps : 60);
      next_frame += frame_ms;
      if (next_frame <= now) next_frame = now + frame_ms;
      // con los refrescos: la siguiente imagen empieza ya (se entrega a su hora)
      if (vb_target > 0) next_frame = clock_now_ms();

      if (args.max_frames > 0 && app.frame >= args.max_frames) {
        if (!app.exit_requested) printf("Frame limit reached (%ld)\n", app.frame);
        break;
      }
    }

    // En un navegador un error en un callback no para la pagina; en los
    // tests si (para que el fallo se vea enseguida)
    if (err != 0 && args.test) {
      failed = 1;
      break;
    }

    if (app.exit_requested) break;
    if (backend_should_close(backend)) break;

    // ¿Queda algo por hacer?
    // el audio que suena tambien avanza frame a frame (onended, fundidos)
    int frames_pending = timer_frame_count() > 0 || mixer_active() > 0 || video_active();
    int timers_pending = timer_count() > 0;
    int jobs_pending = JS_IsJobPending(rt);
    int images_pending = worker_pending() > 0;   // cargandose en segundo plano
    CanvasData *list[1];
    int has_screen = document_screen_canvases(ctx, list, 1) > 0;
    int alive = frames_pending || timers_pending || jobs_pending || images_pending || backend->input_len > 0 ||
                (args.script && input_script_active()) || args.debug_sock ||
                (keep_window && has_screen);
    if (!alive) break;
    if (jobs_pending || backend->input_len > 0) continue;

    // Esperar hasta el siguiente timer o frame
    double wake = -1;
    if (timers_pending) wake = timer_next_deadline();
    if (frames_pending || (keep_window && has_screen)) {
      if (wake < 0 || next_frame < wake) wake = next_frame;
    }
    // algo cargandose en segundo plano: se vuelve a mirar en 2 ms (con el reloj fijo de
    // los tests se espera de verdad, sin adelantar el reloj del juego)
    if (images_pending && clock_is_fixed()) {
      // con el reloj fijo el reloj del juego salta al siguiente timer sin
      // esperar: un poco de tiempo real para que el hilo de trabajo avance
      sleep_ms(1);
      if (wake < 0) continue;
    } else if (images_pending && (wake < 0 || wake > clock_now_ms() + 2)) {
      wake = clock_now_ms() + 2;
    }
    if (clock_is_fixed()) {
      if (wake > 0) {
        clock_advance_to(wake);
        // el frame avanza con el reloj aunque solo haya timers
        if (!frames_pending && wake > next_frame) next_frame = wake;
      }
    } else if (wake >= 0) {
      double t_sleep = fps.enabled ? real_ms() : 0;
      profiler_phase("idle");
      sleep_ms(wake - clock_now_ms());
      profiler_phase("other");
      if (fps.enabled) {
        fps.sleep_ms += real_ms() - t_sleep;
        fps.fr_idle += real_ms() - t_sleep;
      }
    }
  }

  // --hold: la ventana se queda abierta mostrando el ultimo frame
  if (args.hold_ms > 0 && args.backend == BACKEND_SDL) {
    if (fps.enabled) {
      // en la pausa final, la media de lo que corrio el juego
      if (fps.total >= 2 && fps.last > fps.first) fps.value = (fps.total - 1) * 1000.0 / (fps.last - fps.first);
      fps_overlay(&fps, backend);
    }
    double until = clock_now_ms() + args.hold_ms;
    while (!backend_should_close(backend) && (clock_is_fixed() || clock_now_ms() < until)) {
      backend_process_events(backend);
      present_screen(ctx, backend, &scratch);
      if (clock_is_fixed()) clock_advance_to(clock_now_ms() + FRAME_MS);
      usleep(16000);
      if (clock_is_fixed() && clock_now_ms() >= until) break;
    }
  }

  if (args.screenshot) {
    const Surface *screen = compose_screen(ctx, &scratch);
    if (screen && screen->px) {
      if (image_save_png(screen->px, screen->w, screen->h, args.screenshot) == 0) {
        printf("Screenshot saved: %s\n", args.screenshot);
      } else {
        fprintf(stderr, "Failed to save screenshot: %s\n", args.screenshot);
      }
    } else {
      fprintf(stderr, "No canvas on screen: screenshot not saved\n");
    }
  }
  surface_free(&scratch);
  if (fps.enabled && fps.total >= 10) {
    // hasta el ultimo frame del juego (sin --hold ni el cierre)
    double secs = (fps.last - fps.first) / 1000.0;
    // sin ventana y con reloj fijo: el maximo que quixjs puede dibujar
    if (secs > 0) printf("FPS: %.1f (average of %ld frames)\n", (fps.total - 1) / secs, fps.total);
  }
  input_script_free();
  if (args.profile && profiler_running()) {
    profiler_save(ctx, args.profile, 60);
    printf("Profile: %s\n", args.profile);
  }
  profiler_stop();
  debug_shutdown();

  if (failed) {
    exit_code = 1;
  } else if (app.exit_requested) {
    exit_code = app.exit_code;
  } else if (args.test) {
    fprintf(stderr, "TEST TIMEOUT: quix.exit() was not called after %ld frames\n", app.frame);
    exit_code = EXIT_TEST_TIMEOUT;
  }

  free_script_bundle(&bundle);
  timer_free(ctx);
  video_free_all();
  mixer_shutdown();
  audio_free(ctx);
  script_free(ctx);
  free_element_registry(ctx);
  events_free(ctx);

  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  blob_free_all();
  gl_shutdown();
  font_quit();
  backend_cleanup(backend);
  free(backend);

  return exit_code;
}
