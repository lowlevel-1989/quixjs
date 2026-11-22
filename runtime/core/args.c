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

// Lee las opciones de la linea de comandos (--help las lista). Ver args.h.

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "args.h"

// Devuelve 0 si todo bien, -1 si hay un argumento invalido.
int parse_args(int argc, char **argv, Args *args) {
  // todo a cero: una opcion nueva sin valor inicial no puede salir con basura
  memset(args, 0, sizeof(*args));
  args->config_path = "config.toml";
  args->show_help = 0;
  args->show_version = 0;
  args->backend = BACKEND_SDL;
  args->max_frames = 0;
  args->fixed_step = 0;
  args->test = 0;
  args->screenshot = NULL;
  args->hold_ms = 0;
  args->gen_config_dir = NULL;
  args->fullscreen = 0;
  args->resizable = 0;
  args->script = NULL;
  args->debug_sock = NULL;
  args->show_fps = 0;
  args->no_webgl = 0;
  args->launcher = 0;
  args->controls = 0;
  int backend_given = 0;

  for (int i = 1; i < argc; i++) {
    const char *a = argv[i];
    int has_value = i + 1 < argc;

    if (strcmp(a, "--version") == 0) {
      args->show_version = 1;
      continue;
    }
    if (strcmp(a, "--help") == 0 || strcmp(a, "-h") == 0) {
      args->show_help = 1;
    } else if ((strcmp(a, "--config") == 0 || strcmp(a, "-c") == 0) && has_value) {
      args->config_path = argv[++i];
    } else if ((strcmp(a, "--backend") == 0 || strcmp(a, "-b") == 0) && has_value) {
      const char *b = argv[++i];
      backend_given = 1;
      if (strcmp(b, "sdl") == 0) {
        args->backend = BACKEND_SDL;
      } else if (strcmp(b, "dummy") == 0) {
        args->backend = BACKEND_DUMMY;
      } else {
        fprintf(stderr, "Unknown backend: %s\n", b);
        return -1;
      }
    } else if (strcmp(a, "--frames") == 0 && has_value) {
      args->max_frames = atol(argv[++i]);
    } else if (strcmp(a, "--fixed-step") == 0) {
      args->fixed_step = 1;
    } else if (strcmp(a, "--screenshot") == 0 && has_value) {
      args->screenshot = argv[++i];

    } else if (strcmp(a, "--expose-gc") == 0) {
      args->expose_gc = 1;
    } else if (strcmp(a, "--no-webgl") == 0) {
      args->no_webgl = 1;
    } else if (strcmp(a, "--fps") == 0) {
      args->show_fps = 1;
    } else if (strncmp(a, "--frame-rate=", 13) == 0) {
      int v = atoi(a + 13);
      if (v < 10 || v > 60) {
        fprintf(stderr, "--frame-rate: from 10 to 60\n");
        return -1;
      }
      args->frame_rate = v;
    } else if (strcmp(a, "--debug") == 0) {
      // --debug o --debug=RUTA (por defecto quixjs.sock en la carpeta actual)
      args->debug_sock = "quixjs.sock";
    } else if (strncmp(a, "--debug=", 8) == 0) {
      args->debug_sock = a + 8;
    } else if (strcmp(a, "--script") == 0 && has_value) {
      args->script = argv[++i];
    } else if (strcmp(a, "--resizable") == 0) {
      args->resizable = 1;
    } else if (strcmp(a, "--fullscreen") == 0) {
      args->fullscreen = 1;
    } else if (strcmp(a, "--gen-config") == 0 && has_value) {
      args->gen_config_dir = argv[++i];
    } else if (strcmp(a, "--hold") == 0 && has_value) {
      args->hold_ms = atoi(argv[++i]);
    } else if (strcmp(a, "--profile") == 0) {
      args->profile = "quixjs-profile.txt";
    } else if (strncmp(a, "--profile=", 10) == 0) {
      args->profile = a + 10;
    } else if (strcmp(a, "--launcher") == 0) {
      args->launcher = 1;
    } else if (strcmp(a, "--controls") == 0) {
      args->controls = 1;
    } else if (strcmp(a, "--test") == 0) {
      args->test = 1;
    } else {
      fprintf(stderr, "Unknown option: %s\n", a);
      return -1;
    }
  }

  // Modo test: sin ventana y con reloj fijo. Con "-b sdl" se ve en una
  // ventana y con el reloj real (a la velocidad normal del juego).
  if (args->test) {
    if (!backend_given) args->backend = BACKEND_DUMMY;
    if (args->backend == BACKEND_DUMMY) args->fixed_step = 1;
    if (args->max_frames == 0) args->max_frames = 600;
  }
  return 0;
}

void print_help() {
  printf("Usage: quixjs [options]\n");
  printf("Options:\n");
  printf("  -c, --config <path>    Path to config.toml file (default: config.toml)\n");
  printf("      --gen-config <dir|.html> Print a config.toml for the game and exit\n");
  printf("  -b, --backend <type>   Select backend: sdl or dummy (default: sdl)\n");
  printf("      --fullscreen       Fullscreen, keeping the aspect ratio (black bars)\n");
  printf("      --resizable        Resizable from the start (a tiling WM tiles it)\n");
  printf("      --frames <n>       Exit after n frames\n");
  printf("      --fixed-step       Fixed clock: each frame advances 1000/60 ms\n");
  printf("      --screenshot <png> Save the last frame as PNG on exit\n");
  printf("      --script <file>    Input script: keys and screenshots at given frames\n");
  printf("      --debug[=sock]     Debug console on a Unix socket (tools/console.py)\n");
  printf("      --fps              Show the frames per second (on screen and in the output)\n");
  printf("      --frame-rate=<n>   Frame rate cap (10 to 60), above the toml (for testing)\n");
  printf("      --profile[=file]   Sampling profiler: report in file (quixjs-profile.txt) every 10 s and on exit\n");
  printf("      --launcher         Menu before the game: play, achievements, transfer save, options\n");
  printf("      --controls         Only the Controls screen of the launcher (map the gamepad), then exit\n");
  printf("      --no-webgl         No WebGL: games use their canvas renderer\n");
  printf("      --expose-gc        A global gc() that collects now, as in V8 (benchmarks)\n");
  printf("      --hold <ms>        With a window: keep it open <ms> after the end\n");
  printf("      --test             Test mode: dummy backend, fixed clock, max 600 frames,\n");
  printf("                         the game must call quix.exit() (with -b sdl: window\n");
  printf("                         and real clock)\n");
  printf("      --version          Show the version and exit\n");
  printf("  -h, --help             Show this help message\n");
}
