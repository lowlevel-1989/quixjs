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

// La ventana: el backend elegido, la escala con la que el juego cabe en la
// pantalla (fit_scale, para image_scale y render_scale "auto") y start_window.
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "run.h"

// La escala con la que el juego (width x height) cabe en la pantalla sin
// deformarse, para image_scale y render_scale "auto"; 0 si no se sabe
static double fit_scale(Backend *backend, int gw, int gh) {
  int sw = 0, sh = 0;
  if (gw <= 0 || gh <= 0 || backend_screen_size(backend, &sw, &sh) != 0) return 0;
  double fx = (double)sw / gw, fy = (double)sh / gh;
  printf("[screen] game %dx%d on a %dx%d screen\n", gw, gh, sw, sh);
  return fx < fy ? fx : fy;
}
// El toml del juego, el backend y la ventana. 0 si falla
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
  printer_print(&r->cfg);
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
  // [game] fit = "screen": el juego ve la forma de la pantalla (innerWidth,
  // innerHeight): el alto del toml se queda y el ancho crece o se encoge. Solo
  // a pantalla completa (en una portatil, siempre): en una ventana, la del toml
  int full = backend->fullscreen;
#ifdef QUIX_HANDHELD
  full = 1;
#endif
  int sw = 0, sh = 0;
  if (r->cfg.game.fit_screen && full && backend_screen_size(backend, &sw, &sh) == 0 && sh > 0) {
    int w = (int)((double)r->cfg.game.height * sw / sh + 0.5) & ~1;
    printf("[screen] fit: %dx%d -> %dx%d (screen %dx%d)\n", r->cfg.game.width, r->cfg.game.height, w,
           r->cfg.game.height, sw, sh);
    r->cfg.game.width = w;
  }
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
  // sigue en width x height; lo usa el hook rpgmaker-mv/render/render-scale con
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
