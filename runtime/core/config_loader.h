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

#pragma once

#include <stdbool.h>

#define MAX_SCRIPT_FILES 16
#define MAX_PATH_LEN 256

typedef struct {
  char name[64];
  char version[32];
  char canvas_id[64];
} EngineConfig;

typedef struct {
  char game_dir[MAX_PATH_LEN];
  char cache_dir[MAX_PATH_LEN];
  int width;
  int height;
} GameConfig;

typedef struct {
  char entry[MAX_PATH_LEN];
  char libs[MAX_SCRIPT_FILES][MAX_PATH_LEN];
  int  libs_count;
  char includes[MAX_SCRIPT_FILES][MAX_PATH_LEN];
  int  includes_count;
} ScriptConfig;

typedef struct {
  EngineConfig engine;
  GameConfig game;
  ScriptConfig scripts;
} Config;

int config_load(Config* out, const char* filename);
