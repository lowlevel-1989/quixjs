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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tomlc17.h"
#include "config_loader.h"

int config_load(Config *out, const char *filename) {

  FILE *f = fopen(filename, "r");
  if (!f) return 1;

  toml_result_t result = toml_parse_file(f);
  fclose(f);

  if (!result.ok) {
    fprintf(stderr, "TOML parse error: %s\n", result.errmsg);
    return 2;
  }

  toml_datum_t engine  = toml_get(result.toptab, "engine");
  toml_datum_t game    = toml_get(result.toptab, "game");
  toml_datum_t scripts = toml_get(result.toptab, "scripts");

  strncpy(out->engine.name,      "vanilla", sizeof(out->engine.name));
  strncpy(out->engine.version,   "x.y.z",   sizeof(out->engine.version));
  strncpy(out->engine.canvas_id, "canvas",  sizeof(out->engine.canvas_id));

  if (engine.type == TOML_TABLE) {
    toml_datum_t name = toml_get(engine, "name");
    if (name.type == TOML_STRING) {
      strncpy(out->engine.name, name.u.s, sizeof(out->engine.name));
    }

    toml_datum_t version = toml_get(engine, "version");
    if (version.type == TOML_STRING) {
      strncpy(out->engine.version, version.u.s, sizeof(out->engine.version));
    }

    toml_datum_t canvas_id = toml_get(engine, "canvas_id");
    if (canvas_id.type == TOML_STRING) {
      strncpy(out->engine.canvas_id, canvas_id.u.s, sizeof(out->engine.canvas_id));
    }
  }

  if (game.type == TOML_TABLE) {
    toml_datum_t game_dir = toml_get(game, "game_dir");
    if (game_dir.type == TOML_STRING) {
      strncpy(out->game.game_dir, game_dir.u.s, sizeof(out->game.game_dir));
    }

    toml_datum_t cache_dir = toml_get(game, "cache_dir");
    if (cache_dir.type == TOML_STRING) {
      strncpy(out->game.cache_dir, cache_dir.u.s, sizeof(out->game.cache_dir));
    }

    toml_datum_t width = toml_get(game, "width");
    if (width.type == TOML_INT64) {
      out->game.width = (int)width.u.int64;
    }

    toml_datum_t height = toml_get(game, "height");
    if (height.type == TOML_INT64) {
      out->game.height = (int)height.u.int64;
    }
  }

  if (scripts.type == TOML_TABLE) {
    toml_datum_t entry = toml_get(scripts, "entry");
    if (entry.type == TOML_STRING) {
      strncpy(out->scripts.entry, entry.u.s, sizeof(out->scripts.entry));
    }

    toml_datum_t libs = toml_get(scripts, "libs");
    if (libs.type == TOML_ARRAY) {
      int count = libs.u.arr.size;
      out->scripts.libs_count = 0;

      for (int i = 0; i < count && out->scripts.libs_count < MAX_SCRIPT_FILES; i++) {
        toml_datum_t lib = libs.u.arr.elem[i];
        if (lib.type == TOML_STRING) {
          strncpy(out->scripts.libs[out->scripts.libs_count], lib.u.s, MAX_PATH_LEN);
          out->scripts.libs_count++;
        }
      }
    }

    toml_datum_t includes = toml_get(scripts, "includes");
    if (includes.type == TOML_ARRAY) {
      int count = includes.u.arr.size;
      out->scripts.includes_count = 0;

      for (int i = 0; i < count && out->scripts.includes_count < MAX_SCRIPT_FILES; i++) {
        toml_datum_t inc = includes.u.arr.elem[i];
        if (inc.type == TOML_STRING) {
          strncpy(out->scripts.includes[out->scripts.includes_count], inc.u.s, MAX_PATH_LEN);
          out->scripts.includes_count++;
        }
      }
    }

  }

  toml_free(result);
  return 0;
}
