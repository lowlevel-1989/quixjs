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

// El motor de JS: crearlo, agregar las APIs del navegador y de quixjs, los
// hooks, y compilar y ejecutar los scripts del juego (start_js).
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "run.h"

STEP int start_js(Run *r, Loop *l) {
  Config *cfg = &r->cfg;
  mount_rom(r, l);
  // el aviso de carga desde el principio, tambien sin launcher (con el, ya
  // sale desde Play): el juego tarda en leer y correr sus scripts
  if (!r->args.launcher && !r->args.test) backend_loading_now(l->backend);
  // Select + R2 vuelve al launcher, ya en el juego
  l->backend->menu_back = r->args.launcher;
  // [controls] vibration y Options > Vibration (ya con lo elegido en el
  // launcher), antes del juego
  l->backend->no_vibration = !cfg->controls.vibration || r->settings.no_vibration;
  // los mods activos: los elegidos en el launcher (Mods, en
  // quixjs-settings.toml) o, sin eso, [game] mods. Antes de leer el juego
  char (*names)[64] = r->settings.mods_set ? r->settings.mods : cfg->mods.names;
  int count = r->settings.mods_set ? r->settings.mods_count : cfg->mods.count;
  if (count > 0) {
    url_set_mods(cfg->game.game_dir, cfg->mods.root, names, count);
    printf("[mods]");
    for (int i = 0; i < count; i++) printf(" %s", names[i]);
    printf(" (%s, over %s)\n", cfg->mods.root, cfg->game.game_dir);
    // y sus hooks (mods/<nombre>/hooks.toml), encima de los del juego
    hooks_load_mods(cfg->mods.root, (const char (*)[64])names, count);
  }

  // [engine] js_memory: la RAM que JavaScriptCore cree tener. Con menos,
  // pasa el GC antes y su heap crece menos (antes de crear el motor; un
  // JSC_forceRAMSize del entorno manda)
  if (cfg->engine.js_memory > 0) {
    char bytes[32];
    snprintf(bytes, sizeof(bytes), "%lld", (long long)cfg->engine.js_memory * 1024 * 1024);
    setenv("JSC_forceRAMSize", bytes, 0);
    printf("JavaScriptCore memory: %d MB\n", cfg->engine.js_memory);
  }
  // el profiler de JSC: una muestra cada 10 ms (con 1 ms, en una portatil
  // pesaba mas que el juego)
  setenv("JSC_sampleInterval", "10000", 0);

  l->rt = JS_NewRuntime();
  if (!l->rt) {
    fprintf(stderr, "Failed to create JS runtime\n");
    quit_early(l);
    return 0;
  }
  l->ctx = JS_NewContext(l->rt);
  if (!l->ctx) {
    fprintf(stderr, "Failed to create JS context\n");
    JS_FreeRuntime(l->rt);
    quit_early(l);
    return 0;
  }
  JSContext *ctx = l->ctx;
  js_api_add_console(ctx);

  r->app.backend = l->backend;
  r->app.cfg = cfg;
  JS_SetContextOpaque(ctx, &r->app);

  webgl_set_enabled(cfg->engine.webgl && !r->args.no_webgl, r->args.backend != BACKEND_SDL);

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
  if (r->args.expose_gc) {
    static const char gc_js[] = "globalThis.gc = function () { quix.memory.collect(); };";
    JSValue v = JS_Eval(ctx, gc_js, strlen(gc_js), "<expose-gc>", 0);
    JS_FreeValue(ctx, v);
  }
  js_api_add_video(ctx);
  hooks_add_api(ctx);
  engines_add_api(ctx);
  // Como en un navegador, cada elemento con id es tambien una variable global
  // (window.c, o c a secas). Los que quixjs conoce de antemano: el
  // canvas (canvas_id) y los de [html] elements. Un var, let o function del
  // juego con ese nombre la tapa, como alli
  {
    static const char named_js[] =
      "(function (ids) { ids.forEach(function (id) {"
      " if (!id || id in globalThis) return;"
      " Object.defineProperty(globalThis, id, { configurable: true,"
      "  get: function () { return document.getElementById(id); },"
      "  set: function (v) { Object.defineProperty(globalThis, id, { value: v, writable: true, configurable: true, enumerable: true }); } });"
      "}); })";
    JSValue fn = JS_Eval(ctx, named_js, strlen(named_js), "<named-elements>", 0);
    JSValue ids = JS_NewArray(ctx);
    uint32_t k = 0;
    JS_SetPropertyUint32(ctx, ids, k++, JS_NewString(ctx, cfg->engine.canvas_id));
    for (int i = 0; i < cfg->html_count; i++) JS_SetPropertyUint32(ctx, ids, k++, JS_NewString(ctx, cfg->html[i].id));
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue res = JS_Call(ctx, fn, global, 1, (JSValueConst *)&ids);
    JS_FreeValue(ctx, res);
    JS_FreeValue(ctx, global);
    JS_FreeValue(ctx, ids);
    JS_FreeValue(ctx, fn);
  }
  // [engine] quality u Options > Quality: devicePixelRatio, los pixeles que
  // dibuja un juego 3D por cada pixel del juego (three.js lo usa: low dibuja
  // la cuarta parte). Lo demas (el marcador en otro canvas) sigue nitido
  int quality = r->settings.quality ? r->settings.quality : cfg->engine.quality;
  if (quality) {
    static const double scale[] = { 1, 0.5, 0.75, 1 };
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "devicePixelRatio", JS_NewFloat64(ctx, scale[quality]));
    JS_FreeValue(ctx, global);
    printf("[engine] quality %s: devicePixelRatio %.2f\n", quality == 1 ? "low" : quality == 2 ? "medium" : "high", scale[quality]);
  }

  // [fonts] del toml del juego (de un mod si lo trae) y despues los
  // @font-face de fonts/gamefont.css (RPG Maker MV y MZ), que mandan: un
  // parche de traduccion cambia ahi la fuente por una con sus letras
  font_init();
  for (int i = 0; i < cfg->fonts_count; i++) {
    char path[1024];
    if (url_resolve(cfg->game.game_dir, cfg->fonts[i].path, path, sizeof(path)) != URL_FILE) continue;
    if (font_register(cfg->fonts[i].family, path) == 0) printf("Font: %s -> %s\n", cfg->fonts[i].family, path);
  }
  register_css_fonts(cfg->game.game_dir, "fonts/gamefont.css");

  int failed = js_api_add_polyfill(ctx) != 0;
  if (!failed) failed = js_api_add_audio(ctx, r->args.backend == BACKEND_SDL) != 0;
  if (!failed && cfg->engine.nwjs) failed = js_api_add_nwjs(ctx) != 0;
  if (!failed && cfg->engine.webview2) failed = js_api_add_webview2(ctx) != 0;
  // runtime/engines/<motor>/entry.js ([engine] name), antes del juego
  if (!failed) failed = hooks_run_engine_entry(ctx, cfg->engine.name) != 0;
  // --trace-saves: las funciones de partida del juego (runtime/binding/quix/trace-saves.js)
  if (!failed) trace_saves_start(ctx);

  if (!failed && scripts_load(ctx, cfg, &r->bundle) != 0) {
    fprintf(stderr, "Failed to compile scripts\n");
    failed = 1;
  }
  if (!failed) script_add_static(ctx, r->bundle.filenames, r->bundle.count);
  if (!failed && scripts_run(ctx, &r->bundle, script_before_static) != 0) {
    fprintf(stderr, "Error executing bytecode\n");
    failed = 1;
  }
  // los hooks de las funciones que ya existen (los de los plugins, en cuanto se carguen)
  if (!failed && hooks_pending()) hooks_install(ctx);
  // DOMContentLoaded y window.onload, despues de los <script> del arranque
  if (!failed) script_document_loaded(ctx);
  // el aviso del arranque se va cuando el juego ya dibuja normal y no le
  // quedan cargas (archivos, imagenes, sonidos); si el juego tiene su propia
  // carga, sigue la suya
  backend_set_loading(l->backend, 0, 0);
  l->failed = failed;
  return 1;
}

// Lo que necesita el bucle: --fps, --profile, --debug y el tope de fps
