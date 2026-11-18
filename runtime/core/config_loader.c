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

// Lee el toml del juego (config.toml, o el port.toml de un port). Ver
// config_loader.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tomlc17.h"
#include "config_loader.h"

// La plataforma del binario, para [engine.<plataforma>]
#ifdef QUIX_HANDHELD
#define QUIX_PLATFORM "handheld"
#else
#define QUIX_PLATFORM "pc"
#endif

// Los ajustes de [engine.pc] / [engine.handheld] (cambian los de [engine])
static void engine_apply(toml_datum_t t, EngineConfig *e) {
  toml_datum_t v = toml_get(t, "fps");
  if (v.type == TOML_INT64 && v.u.int64 >= 10 && v.u.int64 <= 60) e->fps = (int)v.u.int64;
  v = toml_get(t, "vsync");
  if (v.type == TOML_BOOLEAN) e->vsync = v.u.boolean;
  v = toml_get(t, "present_ahead");
  if (v.type == TOML_BOOLEAN) e->present_ahead = v.u.boolean;
  v = toml_get(t, "webgl");
  if (v.type == TOML_BOOLEAN) e->webgl = v.u.boolean;
  v = toml_get(t, "low_memory_mb");
  if (v.type == TOML_INT64 && v.u.int64 >= 0 && v.u.int64 <= 4096) e->low_memory_mb = (int)v.u.int64;
  v = toml_get(t, "js_memory");
  if (v.type == TOML_INT64 && v.u.int64 >= 32 && v.u.int64 <= 4096) e->js_memory = (int)v.u.int64;
}

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
  toml_datum_t fonts   = toml_get(result.toptab, "fonts");
  toml_datum_t launcher = toml_get(result.toptab, "launcher");
  toml_datum_t saves    = toml_get(result.toptab, "saves");
  toml_datum_t controls = toml_get(result.toptab, "controls");
  if (controls.type == TOML_TABLE) {
    toml_datum_t pad = toml_get(controls, "gamepad");
    if (pad.type == TOML_BOOLEAN) out->controls.gamepad = pad.u.boolean;
  }

  strncpy(out->engine.name,      "vanilla", sizeof(out->engine.name));
  strncpy(out->engine.version,   "x.y.z",   sizeof(out->engine.version));
  strncpy(out->engine.canvas_id, "canvas",  sizeof(out->engine.canvas_id));
  out->engine.webgl = true;
  out->engine.fps = 60;
  out->engine.low_memory_mb = 96;
  out->engine.vsync = true;
  out->engine.present_ahead = true;

  if (engine.type == TOML_TABLE) {
    toml_datum_t name = toml_get(engine, "name");
    if (name.type == TOML_STRING) {
      strncpy(out->engine.name, name.u.s, sizeof(out->engine.name));
    }

    toml_datum_t version = toml_get(engine, "version");
    if (version.type == TOML_STRING) {
      strncpy(out->engine.version, version.u.s, sizeof(out->engine.version));
    }

    toml_datum_t pixi = toml_get(engine, "pixi");
    if (pixi.type == TOML_STRING) {
      snprintf(out->engine.pixi, sizeof(out->engine.pixi), "%s", pixi.u.s);
    }

    toml_datum_t webgl = toml_get(engine, "webgl");
    if (webgl.type == TOML_BOOLEAN) {
      out->engine.webgl = webgl.u.boolean;
    }

    // fps = 30 en una portatil justa: RPG Maker sigue con 60 actualizaciones
    // por segundo (hace dos por imagen) y solo dibuja la mitad
    toml_datum_t fps = toml_get(engine, "fps");
    if (fps.type == TOML_INT64 && fps.u.int64 >= 10 && fps.u.int64 <= 60) {
      out->engine.fps = (int)fps.u.int64;
    }

    toml_datum_t vsync = toml_get(engine, "vsync");
    if (vsync.type == TOML_BOOLEAN) out->engine.vsync = vsync.u.boolean;
    toml_datum_t ahead = toml_get(engine, "present_ahead");
    if (ahead.type == TOML_BOOLEAN) out->engine.present_ahead = ahead.u.boolean;
    toml_datum_t lm = toml_get(engine, "low_memory_mb");
    if (lm.type == TOML_INT64 && lm.u.int64 >= 0 && lm.u.int64 <= 4096) out->engine.low_memory_mb = (int)lm.u.int64;
    toml_datum_t jsm = toml_get(engine, "js_memory");
    if (jsm.type == TOML_INT64 && jsm.u.int64 >= 32 && jsm.u.int64 <= 4096) out->engine.js_memory = (int)jsm.u.int64;

    toml_datum_t nwjs = toml_get(engine, "nwjs");
    if (nwjs.type == TOML_BOOLEAN) {
      out->engine.nwjs = nwjs.u.boolean;
    }

    toml_datum_t canvas_id = toml_get(engine, "canvas_id");
    if (canvas_id.type == TOML_STRING) {
      strncpy(out->engine.canvas_id, canvas_id.u.s, sizeof(out->engine.canvas_id));
    }

    // [engine.pc] / [engine.handheld]: lo que cambia en esa plataforma (la del
    // binario: el de PortMaster es handheld), por ejemplo 30 fps en una
    // portatil y 60 en el PC
    toml_datum_t plat = toml_get(engine, QUIX_PLATFORM);
    if (plat.type == TOML_TABLE) engine_apply(plat, &out->engine);
  }

  if (game.type == TOML_TABLE) {
    toml_datum_t game_dir = toml_get(game, "game_dir");
    if (game_dir.type == TOML_STRING) {
      strncpy(out->game.game_dir, game_dir.u.s, sizeof(out->game.game_dir));
    }

    toml_datum_t gversion = toml_get(game, "version");
    if (gversion.type == TOML_STRING) snprintf(out->game.version, sizeof(out->game.version), "%s", gversion.u.s);
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
      out->scripts.entry = strdup(entry.u.s);
    }

    toml_datum_t libs = toml_get(scripts, "libs");
    if (libs.type == TOML_ARRAY) {
      int count = libs.u.arr.size;
      out->scripts.libs_count = 0;

      for (int i = 0; i < count && out->scripts.libs_count < MAX_SCRIPT_FILES; i++) {
        toml_datum_t lib = libs.u.arr.elem[i];
        if (lib.type == TOML_STRING) {
          out->scripts.libs[out->scripts.libs_count] = strdup(lib.u.s);
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
          out->scripts.includes[out->scripts.includes_count] = strdup(inc.u.s);
          out->scripts.includes_count++;
        }
      }
    }

  }

  out->fonts_count = 0;
  if (fonts.type == TOML_TABLE) {
    for (int i = 0; i < fonts.u.tab.size && out->fonts_count < MAX_FONTS; i++) {
      toml_datum_t v = fonts.u.tab.value[i];
      if (v.type != TOML_STRING) continue;
      FontConfig *f = &out->fonts[out->fonts_count++];
      snprintf(f->family, sizeof(f->family), "%s", fonts.u.tab.key[i]);
      snprintf(f->path, sizeof(f->path), "%s", v.u.s);
    }
  }

  if (launcher.type == TOML_TABLE) {
    // los colores del menu (el titulo es [game] title)
    static const char *keys[] = { "background", "text", "accent" };
    char *dst[] = { out->launcher.background, out->launcher.text, out->launcher.accent };
    size_t room[] = { sizeof(out->launcher.background), sizeof(out->launcher.text), sizeof(out->launcher.accent) };
    for (int i = 0; i < 3; i++) {
      toml_datum_t v = toml_get(launcher, keys[i]);
      if (v.type == TOML_STRING) snprintf(dst[i], room[i], "%s", v.u.s);
    }
  }

  snprintf(out->saves.achievements, sizeof(out->saves.achievements), "quixjs-achievements.txt");
  if (saves.type == TOML_TABLE) {
    toml_datum_t dir = toml_get(saves, "dir");
    // desde [game] game_dir, salvo si es absoluta
    if (dir.type == TOML_STRING) {
      if (dir.u.s[0] != '/' && out->game.game_dir[0]) {
        const char *d = dir.u.s;
        while (d[0] == '.' && d[1] == '/') d += 2;
        char joined[MAX_PATH_LEN * 2 + 2];
        if (d[0] == 0 || strcmp(d, ".") == 0) snprintf(joined, sizeof joined, "%s", out->game.game_dir);
        else snprintf(joined, sizeof joined, "%s/%s", out->game.game_dir, d);
        snprintf(out->saves.dir, sizeof(out->saves.dir), "%.*s", (int)sizeof(out->saves.dir) - 1, joined);
      } else {
        snprintf(out->saves.dir, sizeof(out->saves.dir), "%s", dir.u.s);
      }
    }
    toml_datum_t ach = toml_get(saves, "achievements");
    if (ach.type == TOML_STRING) snprintf(out->saves.achievements, sizeof(out->saves.achievements), "%s", ach.u.s);
    toml_datum_t files = toml_get(saves, "files");
    if (files.type == TOML_ARRAY) {
      for (int i = 0; i < files.u.arr.size && out->saves.files_count < MAX_SAVE_PATTERNS; i++) {
        toml_datum_t v = files.u.arr.elem[i];
        if (v.type == TOML_STRING) {
          snprintf(out->saves.files[out->saves.files_count++], MAX_PATH_LEN, "%s", v.u.s);
        }
      }
    }
  }

  // [game] title, id y mods: el nombre (el del launcher), el id del juego
  // (Transfer Save solo pasa partidas del mismo; sin id, el titulo) y los
  // mods activos (mods/<nombre>/ junto al config, con las mismas rutas que
  // dentro de game_dir)
  if (game.type == TOML_TABLE) {
    toml_datum_t title = toml_get(game, "title");
    if (title.type == TOML_STRING) snprintf(out->launcher.title, sizeof(out->launcher.title), "%s", title.u.s);
    toml_datum_t id = toml_get(game, "id");
    if (id.type == TOML_STRING) snprintf(out->saves.id, sizeof(out->saves.id), "%s", id.u.s);
    toml_datum_t mods = toml_get(game, "mods");
    if (mods.type == TOML_ARRAY) {
      out->mods.count = 0;
      for (int i = 0; i < mods.u.arr.size && out->mods.count < MAX_MODS; i++) {
        toml_datum_t m = mods.u.arr.elem[i];
        // un nombre de carpeta, sin rutas
        if (m.type == TOML_STRING && m.u.s[0] && !strchr(m.u.s, '/') && strcmp(m.u.s, "..") != 0) {
          snprintf(out->mods.names[out->mods.count++], sizeof(out->mods.names[0]), "%s", m.u.s);
        }
      }
    }
  }
  {
    // mods/ junto al config
    const char *slash = strrchr(filename, '/');
    if (slash) snprintf(out->mods.root, sizeof(out->mods.root), "%.*s/mods", (int)(slash - filename), filename);
    else snprintf(out->mods.root, sizeof(out->mods.root), "mods");
  }

  toml_free(result);
  return 0;
}
