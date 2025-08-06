/*
 * QuixJS - A lightweight JS runtime using SDL2 and QuickJS
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

#include <string.h>
#include <stdio.h>
#include "args.h"

void parse_args(int argc, char **argv, Args *args) {
  args->config_path = "config.toml";
  args->force_recompile = 0;
  args->show_help = 0;
  args->backend = BACKEND_SDL;

  for (int i = 1; i < argc; i++) {
    if ((strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)) {
      args->show_help = 1;
    } else if ((strcmp(argv[i], "--config") == 0 || strcmp(argv[i], "-c") == 0) && i + 1 < argc) {
      args->config_path = argv[i + 1];
      i++;
    } else if (strcmp(argv[i], "--force") == 0 || strcmp(argv[i], "-f") == 0) {
      args->force_recompile = 1;
    } else if (strcmp(argv[i], "--backend") == 0 || strcmp(argv[i], "-b") == 0) {
      if (strcmp(argv[i + 1], "sdl") == 0){
        args->backend = BACKEND_SDL;
      }else if (strcmp(argv[i + 1], "dummy") == 0){
        args->backend = BACKEND_DUMMY;
      }
      i++;
    }
  }
}

void print_help() {
  printf("Usage: quixjs [--config <path>]\n");
  printf("Options:\n");
  printf("  -c, --config <path>  Path to config.toml file (default: config.toml)\n");
  printf("  -f, --force          Force recompilation ignoring cache\n");
  printf("  -b, --backend <type> Select backend: sdl or dummy (default: sdl)\n");
  printf("  -h, --help           Show this help message\n");
}
