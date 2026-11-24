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

// Arrancar: --help/--version/--gen-config (info_option), salir antes de
// crear el motor de JS (quit_early) y las herramientas del final
// (start_tools). La ventana esta en window.c, el port en port.c y el motor
// de JS en js.c.
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "run.h"

// --help, --version y --gen-config: dicen algo y salen. 1 si hay que salir
// con *code
static int info_option(int argc, char **argv, Args *args, int *code) {
  if (args_parse(argc, argv, args) != 0) {
    args_print_help();
    *code = 1;
    return 1;
  }
  if (args->show_help) {
    args_print_help();
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
  // el resto necesita el toml del juego
  if (!args->config_path) {
    fprintf(stderr, "quixjs: the toml of the game is required: quixjs -c <game.toml> (quixjs --help)\n");
    *code = 1;
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
