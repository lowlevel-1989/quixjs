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

// Arrancar: --help/--version/--gen-config, la ventana, lo del port, el
// launcher, el motor de JS y lo que necesita el bucle.
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "run.h"

// --help, --version y --gen-config: dicen algo y salen. 1 si hay que salir
// con *code
static int info_option(int argc, char **argv, Args *args, int *code) {
  if (parse_args(argc, argv, args) != 0) {
    print_help();
    *code = 1;
    return 1;
  }
  if (args->show_help) {
    print_help();
    *code = 0;
    return 1;
  }
  if (args->show_version) {
    printf("quixjs v%s\n", QUIX_VERSION);
    *code = 0;
    return 1;
  }
  // --gen-config escribe el toml por la salida: sin nada mas
  if (args->gen_config_dir) {
    *code = gen_config(args->gen_config_dir);
    return 1;
  }
  return 0;
}

// Cerrar la ventana al salir antes de crear el motor de JS (lo unico que hay
// que liberar a esas alturas)
STEP void quit_early(Loop *l) {
  backend_cleanup(l->backend);
  free(l->backend);
}

// El toml del juego, el backend y la ventana. 0 si falla

// La escala con la que el juego (width x height) cabe en la pantalla sin
// deformarse, para image_scale y render_scale "auto"; 0 si no se sabe
static double fit_scale(Backend *backend, int gw, int gh) {
  int sw = 0, sh = 0;
  if (gw <= 0 || gh <= 0 || backend_screen_size(backend, &sw, &sh) != 0) return 0;
  double fx = (double)sw / gw, fy = (double)sh / gh;
  printf("[screen] game %dx%d on a %dx%d screen\n", gw, gh, sw, sh);
  return fx < fy ? fx : fy;
}
STEP int start_window(Run *r, Loop *l) {
  // la version, la primera linea de los logs (para saber cual usa cada uno)
  printf("quixjs v%s\n", QUIX_VERSION);
  printf("\t\thttps://github.com/lowlevel-1989/quixjs\n");

  // los ajustes del malloc de la plataforma (en glibc: los bloques grandes
  // vuelven al sistema al liberarlos)
  platform_mem_init();

  printf("Using config: %s\n", r->args.config_path);
  if (config_load(&r->cfg, r->args.config_path) != 0) {
    fprintf(stderr, "Failed to load config file: %s\n", r->args.config_path);
    return 0;
  }
  print_config(&r->cfg);
  clock_init(r->args.fixed_step);

  // Crear el backend seleccionado
  // Como los navegadores (ANGLE): las variables de los shaders sin valor
  // inicial empiezan en 0. Mesa (Panfrost, llvmpipe...) lo hace con esta
  // opcion; sin ella, un vec3 sin valor inicial (la luz de un juego) salia negro
  setenv("glsl_zero_init", "true", 0);
  Backend *backend = backend_create(r->args.backend);
  if (!backend) {
    fprintf(stderr, "Failed to create backend: %d\n", r->args.backend);
    return 0;
  }
  l->backend = backend;
  backend->fullscreen = r->args.fullscreen;
  // vuelta al launcher (Select + R2) desde una ventana a pantalla completa
  // (runtime/core/main.c): la nueva, igual
  if (getenv("QUIXJS_FULLSCREEN")) {
    backend->fullscreen = 1;
    unsetenv("QUIXJS_FULLSCREEN");
  }
  backend->resizable = r->args.resizable;
  backend->no_vsync = !r->cfg.engine.vsync;
  backend->present_ahead = r->cfg.engine.present_ahead;
  backend->scaling = r->cfg.engine.scaling;
  // controller.map junto al toml del juego (Controls del launcher)
  const char *cfg_slash = strrchr(r->args.config_path, '/');
  if (cfg_slash) snprintf(backend->controller_map, sizeof backend->controller_map, "%.*s/controller.map",
                          (int)(cfg_slash - r->args.config_path), r->args.config_path);
  else snprintf(backend->controller_map, sizeof backend->controller_map, "controller.map");
  // [controls.keys]: acciones con nombre que pulsan una tecla
  for (int k = 0; k < r->cfg.controls.keys_count && k < 8; k++) {
    snprintf(backend->pad_keys[k].name, sizeof backend->pad_keys[k].name, "%s", r->cfg.controls.key_button[k]);
    snprintf(backend->pad_keys[k].code, sizeof backend->pad_keys[k].code, "%s", r->cfg.controls.key_code[k]);
  }
  backend->pad_keys_count = r->cfg.controls.keys_count < 8 ? r->cfg.controls.keys_count : 8;
  // la ventana: "quixjs | <juego>" ([game] title); document.title la cambia
  backend_set_title(backend, r->cfg.launcher.title);
  backend_init(backend, r->cfg.game.width, r->cfg.game.height);
  // los programas de WebGL ya enlazados, en <cache_dir>/shaders
  if (r->cfg.game.cache_dir[0]) {
    char shaders[MAX_PATH_LEN + 16];
    snprintf(shaders, sizeof shaders, "%s/shaders", r->cfg.game.cache_dir);
    shader_cache_init(shaders);
  }
  // [game] image_scale: las imagenes grandes de esas carpetas, reducidas (y
  // guardadas en <cache_dir>/images-<porcentaje>). "auto": a la escala con
  // la que el juego ([game] width x height) cabe en la pantalla sin
  // deformarse; en una pantalla igual o mas grande, no se reducen
  double fit = fit_scale(backend, r->cfg.game.width, r->cfg.game.height);
  double scale = r->cfg.game.image_scale;
  if (scale < 0) scale = fit;   // 0 sin pantalla: no se reducen
  // [game] render_scale: el juego se dibuja a menos resolucion (la logica
  // sigue en width x height; lo usa el hook rpgmaker-mv/render-scale con
  // quix.gpu.renderScale). "auto": la escala con la que se ve en la pantalla
  double rs = r->cfg.game.render_scale;
  if (rs < 0) rs = fit;
  if (rs > 0 && rs < 1) {
    quix_set_render_scale(rs);
    printf("[render-scale] the game is drawn at %.0f%% (%dx%d)\n", rs * 100, (int)(r->cfg.game.width * rs + 0.5),
           (int)(r->cfg.game.height * rs + 0.5));
  }
  image_scale_init(r->cfg.game.cache_dir, scale, r->cfg.game.image_scale_min,
                   (const char (*)[128])r->cfg.game.image_scale_dirs, r->cfg.game.image_scale_dirs_count);
  // Select + Start sale aunque el juego no responda (solo con ventana)
  if (r->args.backend == BACKEND_SDL) watchdog_start();
  return 1;
}

// Lo del port: partidas, logros, simbolos, trucos, hooks, --script y los
// mkdir -p
static void make_dirs(const char *path) {
  char tmp[PATH_MAX];
  snprintf(tmp, sizeof tmp, "%s", path);
  for (char *s = tmp + 1; *s; s++) {
    if (*s != '/') continue;
    *s = 0;
    mkdir(tmp, 0755);
    *s = '/';
  }
  mkdir(tmp, 0755);
}

// ajustes del jugador. 0 si falla (ya cerrado)
STEP int load_port(Run *r, Loop *l) {
  Config *cfg = &r->cfg;
  // [saves] y los logros: los usan el launcher y el juego
  const char *patterns[MAX_SAVE_PATTERNS];
  for (int i = 0; i < cfg->saves.files_count; i++) patterns[i] = cfg->saves.files[i];
  // dir: la carpeta de las partidas, como game_dir (sin dir, game_dir)
  const char *saves_dir = cfg->saves.dir[0] ? cfg->saves.dir : cfg->game.game_dir;
  // el sandbox: leer dentro de la carpeta del toml, escribir dentro de la de
  // las partidas; las rutas del toml tampoco pueden salir de ella
  if (sandbox_init(r->args.config_path, saves_dir) != 0 ||
      (cfg->game.game_dir[0] && !sandbox_check(cfg->game.game_dir, "[game] game_dir")) ||
      (cfg->game.rom[0] && !sandbox_check(cfg->game.rom, "[game] rom"))) {
    quit_early(l);
    return 0;
  }
  // la carpeta de las partidas, creada si aun no existe: con la rom
  // (www.zip) va fuera del juego, en gamedata/, y en un port recien
  // instalado no esta. Fear & Hunger escribe su copia .bak antes de crearla
  // y no guardaba
  struct stat st;
  r->saves_new = stat(saves_dir, &st) != 0;
  make_dirs(saves_dir);
  int npatterns = cfg->saves.files_count;
  // los logros van con las partidas: en su carpeta, y Transfer Save los manda
  // con ellas (el que llega pisa al que habia, como una partida)
  const char *ach_file = cfg->saves.achievements;
  if (ach_file[0] && ach_file[0] != '/') {
    // tambien sin partidas (Abbaye: solo logros)
    if (npatterns < MAX_SAVE_PATTERNS) patterns[npatterns++] = ach_file;
    // antes se guardaban en game_dir: se mueven la primera vez
    char old_path[MAX_PATH_LEN * 2], new_path[MAX_PATH_LEN * 2];
    snprintf(old_path, sizeof old_path, "%s/%s", cfg->game.game_dir, ach_file);
    snprintf(new_path, sizeof new_path, "%s/%s", saves_dir, ach_file);
    if (strcmp(old_path, new_path) != 0 && access(new_path, F_OK) != 0 && access(old_path, F_OK) == 0 &&
        rename(old_path, new_path) == 0) {
      printf("[achievements] moved to %s (they go with the saves)\n", new_path);
    }
  }
  saves_setup(saves_dir, patterns, npatterns);
  // [symbols] antes que los logros y los trucos, que los usan
  const char *config_path = r->args.config_path;
  int bad_state = symbols_load(config_path) < 0;
  const char *game_id = config_game_id(cfg);
  bad_state |= ach_load(config_path, saves_dir, cfg->saves.achievements, game_id) < 0;
  bad_state |= cheats_load(config_path) < 0;
  if ((bad_state || hooks_load(config_path) < 0) && r->args.test) { quit_early(l); return 0; }

  // --script tambien maneja el launcher: sus frames van antes que los del juego
  if (r->args.script && input_script_load(r->args.script) != 0) { quit_early(l); return 0; }

  // los ajustes del jugador (Options del launcher)
  settings_load(&r->settings, config_path);
  settings_apply_cheats(&r->settings);
  // Options > Scaling, si se toco; si no, [engine] scaling
  if (r->settings.scaling) l->backend->scaling = r->settings.scaling - 1;
#ifndef QUIX_HANDHELD
  // Options: a pantalla completa (--fullscreen ya la pone al crear la ventana)
  if (r->settings.fullscreen && !l->backend->fullscreen) backend_set_fullscreen(l->backend, 1);
#endif
  return 1;
}

// --launcher: el menu antes del juego. 1 para jugar; si no, ya cerrado y
// *code dice con que salir
STEP int launcher(Run *r, Loop *l, int *code) {
  *code = 0;
  // --controls: solo la pantalla Controls y salir (la app de Android tiene
  // su menu, pero el mando se asigna con el mismo codigo que aqui)
  if (r->args.controls) {
    launcher_controls(l->backend, &r->cfg);
    input_script_free();
    font_quit();
    quit_early(l);
    return 0;
  }
  if (!r->args.launcher) return 1;
  int play = launcher_run(l->backend, &r->cfg, r->args.config_path, &r->settings, code) == LAUNCHER_PLAY;
  l->script_offset = ui_frames();
  // el juego tarda en arrancar (leer y correr sus scripts): el aviso de
  // carga desde que se pulsa Play
  if (play) backend_loading_now(l->backend);
  if (play) return 1;
  input_script_free();
  font_quit();
  quit_early(l);
  return 0;
}

// Los mods, el motor de JS, las APIs, las fuentes y los scripts del juego.
// 0 si no se pudo crear el motor (ya cerrado); un fallo despues deja
// l->failed y el programa sale por el final, como siempre
// [game] rom (runtime/rom): el juego (un .zip o una carpeta, junto al toml),
// montado en gamedata/ antes de leer nada del juego. El primer script del port
// ([scripts]: libs, includes o entry) solo sirve para avisar si el juego no
// esta donde dice game_dir
static void mount_rom(Run *r, Loop *l) {
  Config *cfg = &r->cfg;
  if (!cfg->game.rom[0]) return;
  const char *first = "";
  const ScriptConfig *sc = &cfg->scripts;
  for (int i = 0; !first[0] && i < sc->libs_count; i++) if (strncmp(sc->libs[i], "../", 3) != 0) first = sc->libs[i];
  for (int i = 0; !first[0] && i < sc->includes_count; i++) if (strncmp(sc->includes[i], "../", 3) != 0) first = sc->includes[i];
  if (!first[0] && sc->entry && strncmp(sc->entry, "../", 3) != 0) first = sc->entry;
  launcher_mount_rom(r->args.test ? NULL : l->backend, cfg, first);
  // [saves] dir: solo la del disco (runtime/rom/rom.h); la primera vez, con
  // las partidas que traiga la rom. Sin dir, las partidas van en gamedata/
  // misma y no se toca
  const char *saves = cfg->saves.dir;
  if (saves[0] && strcmp(saves, cfg->game.game_dir) != 0 && rom_mounted()) {
    if (r->saves_new) rom_copy_dir(saves);
    rom_hide(saves);
  }
}

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
  js_api_add_lan(ctx);
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

  if (!failed && compile_scripts(ctx, cfg, &r->bundle) != 0) {
    fprintf(stderr, "Failed to compile scripts\n");
    failed = 1;
  }
  if (!failed) script_add_static(ctx, r->bundle.filenames, r->bundle.count);
  if (!failed && runner_run_bundle(ctx, &r->bundle, script_before_static) != 0) {
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
STEP void start_tools(Run *r, Loop *l) {
  surface_init(&r->scratch, 0, 0);
  r->shot_ctx = (ShotCtx){ l->ctx, &r->scratch };
  r->fps.enabled = r->args.show_fps || r->settings.show_fps;
  // Options > Profiler: como --profile, junto al toml del juego
  static char profile_path[1024];
  if (!r->args.profile && r->settings.profile) {
    const char *config_path = r->args.config_path;
    const char *slash = strrchr(config_path, '/');
    if (slash) snprintf(profile_path, sizeof(profile_path), "%.*s/quixjs-profile.txt", (int)(slash - config_path), config_path);
    else snprintf(profile_path, sizeof(profile_path), "quixjs-profile.txt");
    r->args.profile = profile_path;
  }
  // el tope de fps: el del toml (lo decide el port: en el launcher no se
  // cambia); --frame-rate para probar
  // Options > FPS y, por encima, --frame-rate
  if (r->settings.fps) r->cfg.engine.fps = r->settings.fps;
  if (r->args.frame_rate > 0) r->cfg.engine.fps = r->args.frame_rate;
  if (!l->failed && r->args.debug_sock && debug_init(r->args.debug_sock) != 0) l->failed = 1;
  r->debug_env = (DebugEnv){ l->ctx, l->backend, &r->app.frame, script_screen, &r->shot_ctx, &r->cfg.engine.fps };
  l->gc_retry_below = LONG_MAX;
  // --profile: el profiler desde el arranque y el informe en un archivo
  if (!l->failed && r->args.profile && profiler_start(l->ctx) == 0) {
    printf("Profiling: report in %s every 10 s\n", r->args.profile);
    l->next_profile_save = real_ms() + 10000;
  }
  l->next_frame = clock_now_ms();
  // [engine] fps: cada cuanto corre requestAnimationFrame (y se presenta)
  l->frame_ms = 1000.0 / (r->cfg.engine.fps > 0 ? r->cfg.engine.fps : 60);
  l->backend->fps = r->cfg.engine.fps;   // el thread de present_ahead lo mira
  if (r->cfg.engine.fps != 60) printf("Frame rate: %d fps\n", r->cfg.engine.fps);
  l->keep_window = r->args.backend == BACKEND_SDL;
}
