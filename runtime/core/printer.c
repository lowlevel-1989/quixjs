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

// Escribe en la salida la configuracion leida, al arrancar.

#include <stdio.h>
#include "printer.h"

void print_config(const Config *cfg) {
  printf("\n--- Loaded Configuration ---\n");
  printf("[engine]\n");
  printf("name = %s\n", cfg->engine.name);
  printf("version = %s\n", cfg->engine.version);
  printf("canvas id = %s\n", cfg->engine.canvas_id);
  if (cfg->engine.pixi[0]) printf("pixi = %s\n", cfg->engine.pixi);
  printf("nwjs = %s\n", cfg->engine.nwjs ? "true" : "false");
  printf("webgl = %s\n\n", cfg->engine.webgl ? "true" : "false");


  printf("[game]\n");
  printf("game_dir = %s\n", cfg->game.game_dir);
  printf("cache_dir = %s\n", cfg->game.cache_dir);
  printf("width = %d\n", cfg->game.width);
  printf("height = %d\n\n", cfg->game.height);

  printf("[scripts]\n");
  printf("entry = %s\n", cfg->scripts.entry);
  for (int i = 0; i < cfg->scripts.libs_count; i++) {
    printf("libs[%d] = %s\n", i, cfg->scripts.libs[i]);
  }
  for (int i = 0; i < cfg->scripts.includes_count; i++) {
    printf("includes[%d] = %s\n", i, cfg->scripts.includes[i]);
  }
  if (cfg->fonts_count > 0) {
    printf("\n[fonts]\n");
    for (int i = 0; i < cfg->fonts_count; i++) {
      printf("%s = %s\n", cfg->fonts[i].family, cfg->fonts[i].path);
    }
  }
  printf("----------------------------\n\n");
}

