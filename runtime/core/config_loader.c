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
#include "core/keymap.h"

// La plataforma del binario, para [engine.<plataforma>]
#ifdef QUIX_HANDHELD
#define QUIX_PLATFORM "handheld"
#else
#define QUIX_PLATFORM "pc"
#endif

// Lee el toml en partes: una funcion por seccion ([engine], [game],
// [scripts], [fonts], [launcher], [saves], [controls]). Lo que falta en el
// toml se queda con su valor por defecto.

static void copy_str(char *dst, size_t room, toml_datum_t v) {
  if (v.type == TOML_STRING) snprintf(dst, room, "%s", v.u.s);
}

// Lo que [engine] comparte con [engine.pc] / [engine.handheld] (estas
// cambian lo de [engine] en su plataforma)
static void engine_apply(toml_datum_t t, EngineConfig *e) {
  // fps = 30 en una portatil justa: RPG Maker sigue con 60 actualizaciones
  // por segundo (hace dos por imagen) y solo dibuja la mitad
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

static void read_engine(toml_datum_t engine, EngineConfig *e) {
  snprintf(e->name, sizeof(e->name), "vanilla");
  snprintf(e->version, sizeof(e->version), "x.y.z");
  snprintf(e->canvas_id, sizeof(e->canvas_id), "canvas");
  e->webgl = true;
  e->fps = 60;
  e->low_memory_mb = 96;
  e->vsync = true;
  e->present_ahead = true;
  if (engine.type != TOML_TABLE) return;

  copy_str(e->name, sizeof(e->name), toml_get(engine, "name"));
  copy_str(e->version, sizeof(e->version), toml_get(engine, "version"));
  copy_str(e->pixi, sizeof(e->pixi), toml_get(engine, "pixi"));
  copy_str(e->canvas_id, sizeof(e->canvas_id), toml_get(engine, "canvas_id"));
  toml_datum_t nwjs = toml_get(engine, "nwjs");
  if (nwjs.type == TOML_BOOLEAN) e->nwjs = nwjs.u.boolean;
  engine_apply(engine, e);

  // [engine.pc] / [engine.handheld]: lo que cambia en esa plataforma (la del
  // binario: el de PortMaster es handheld), por ejemplo 30 fps en una
  // portatil y 60 en el PC
  toml_datum_t plat = toml_get(engine, QUIX_PLATFORM);
  if (plat.type == TOML_TABLE) engine_apply(plat, e);
}

// [game]: carpetas y tamano; title (el nombre del launcher), id (Transfer
// Save solo pasa partidas del mismo juego; sin id, el titulo) y mods (los
// activos: mods/<nombre>/ junto al config, con las mismas rutas que dentro
// de game_dir)
static void read_game(toml_datum_t game, Config *out) {
  if (game.type != TOML_TABLE) return;
  copy_str(out->game.game_dir, sizeof(out->game.game_dir), toml_get(game, "game_dir"));
  copy_str(out->game.cache_dir, sizeof(out->game.cache_dir), toml_get(game, "cache_dir"));
  copy_str(out->game.version, sizeof(out->game.version), toml_get(game, "version"));
  toml_datum_t width = toml_get(game, "width");
  if (width.type == TOML_INT64) out->game.width = (int)width.u.int64;
  toml_datum_t height = toml_get(game, "height");
  if (height.type == TOML_INT64) out->game.height = (int)height.u.int64;

  // imagenes grandes a menos resolucion (opcional; ver gfx/image_scale.h)
  toml_datum_t scale = toml_get(game, "image_scale");
  if (scale.type == TOML_FP64) out->game.image_scale = scale.u.fp64;
  if (scale.type == TOML_INT64) out->game.image_scale = (double)scale.u.int64;
  // "auto": la del juego en la pantalla (core/main/start.c)
  if (scale.type == TOML_STRING && strcmp(scale.u.s, "auto") == 0) out->game.image_scale = -1;
  // dibujar a la resolucion de la pantalla (opcional; ver rpgmaker-mv/render-scale)
  toml_datum_t rscale = toml_get(game, "render_scale");
  if (rscale.type == TOML_FP64) out->game.render_scale = rscale.u.fp64;
  if (rscale.type == TOML_INT64) out->game.render_scale = (double)rscale.u.int64;
  if (rscale.type == TOML_STRING && strcmp(rscale.u.s, "auto") == 0) out->game.render_scale = -1;
  out->game.image_scale_min = 512;
  toml_datum_t scale_min = toml_get(game, "image_scale_min");
  if (scale_min.type == TOML_INT64) out->game.image_scale_min = (int)scale_min.u.int64;
  toml_datum_t scale_dirs = toml_get(game, "image_scale_dirs");
  if (scale_dirs.type == TOML_ARRAY) {
    out->game.image_scale_dirs_count = 0;
    for (int i = 0; i < scale_dirs.u.arr.size && out->game.image_scale_dirs_count < 8; i++) {
      toml_datum_t d = scale_dirs.u.arr.elem[i];
      if (d.type == TOML_STRING && d.u.s[0])
        snprintf(out->game.image_scale_dirs[out->game.image_scale_dirs_count++],
                 sizeof(out->game.image_scale_dirs[0]), "%s", d.u.s);
    }
  }

  copy_str(out->launcher.title, sizeof(out->launcher.title), toml_get(game, "title"));
  copy_str(out->saves.id, sizeof(out->saves.id), toml_get(game, "id"));
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

// una lista de rutas del toml (libs, includes), copiadas
static int read_paths(toml_datum_t list, char **dst, int max) {
  int n = 0;
  if (list.type != TOML_ARRAY) return 0;
  for (int i = 0; i < list.u.arr.size && n < max; i++) {
    toml_datum_t v = list.u.arr.elem[i];
    if (v.type == TOML_STRING) dst[n++] = strdup(v.u.s);
  }
  return n;
}

// [scripts]: libs, includes y entry, en el orden en que se cargan
static void read_scripts(toml_datum_t scripts, ScriptConfig *s) {
  if (scripts.type != TOML_TABLE) return;
  toml_datum_t entry = toml_get(scripts, "entry");
  if (entry.type == TOML_STRING) s->entry = strdup(entry.u.s);
  toml_datum_t libs = toml_get(scripts, "libs");
  if (libs.type == TOML_ARRAY) s->libs_count = read_paths(libs, s->libs, MAX_SCRIPT_FILES);
  toml_datum_t includes = toml_get(scripts, "includes");
  if (includes.type == TOML_ARRAY) s->includes_count = read_paths(includes, s->includes, MAX_SCRIPT_FILES);
}

// [fonts]: familia = "ruta"
static void read_fonts(toml_datum_t fonts, Config *out) {
  out->fonts_count = 0;
  if (fonts.type != TOML_TABLE) return;
  for (int i = 0; i < fonts.u.tab.size && out->fonts_count < MAX_FONTS; i++) {
    toml_datum_t v = fonts.u.tab.value[i];
    if (v.type != TOML_STRING) continue;
    FontConfig *f = &out->fonts[out->fonts_count++];
    snprintf(f->family, sizeof(f->family), "%s", fonts.u.tab.key[i]);
    snprintf(f->path, sizeof(f->path), "%s", v.u.s);
  }
}

// [launcher]: los colores del menu (el titulo es [game] title)
static void read_launcher(toml_datum_t launcher, LauncherConfig *l) {
  if (launcher.type != TOML_TABLE) return;
  copy_str(l->background, sizeof(l->background), toml_get(launcher, "background"));
  copy_str(l->text, sizeof(l->text), toml_get(launcher, "text"));
  copy_str(l->accent, sizeof(l->accent), toml_get(launcher, "accent"));
}

// [saves]: la carpeta de las partidas (relativa a [game] game_dir, salvo si
// es absoluta), el archivo de los logros y que archivos son partidas
static void read_saves(toml_datum_t saves, Config *out) {
  snprintf(out->saves.achievements, sizeof(out->saves.achievements), "quixjs-achievements.txt");
  if (saves.type != TOML_TABLE) return;
  toml_datum_t dir = toml_get(saves, "dir");
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
  copy_str(out->saves.achievements, sizeof(out->saves.achievements), toml_get(saves, "achievements"));
  toml_datum_t files = toml_get(saves, "files");
  if (files.type == TOML_ARRAY) {
    for (int i = 0; i < files.u.arr.size && out->saves.files_count < MAX_SAVE_PATTERNS; i++) {
      toml_datum_t v = files.u.arr.elem[i];
      if (v.type == TOML_STRING) snprintf(out->saves.files[out->saves.files_count++], MAX_PATH_LEN, "%s", v.u.s);
    }
  }
}

// [controls]: gamepad = false quita el mando; vibration = true deja vibrar
static void read_controls(toml_datum_t controls, Config *out) {
  if (controls.type != TOML_TABLE) return;
  toml_datum_t pad = toml_get(controls, "gamepad");
  if (pad.type == TOML_BOOLEAN) out->controls.gamepad = pad.u.boolean;
  toml_datum_t vib = toml_get(controls, "vibration");
  if (vib.type == TOML_BOOLEAN) out->controls.vibration = vib.u.boolean;
  ControlsConfig *c = &out->controls;
  toml_datum_t keys = toml_get(controls, "keys");
  for (int i = 0; keys.type == TOML_TABLE && i < keys.u.tab.size && c->keys_count < 8; i++) {
    toml_datum_t v = keys.u.tab.value[i];
    if (v.type != TOML_STRING) continue;
    const char *code = keymap_code_for(v.u.s);
    if (!code) {
      fprintf(stderr, "[controls] %s: unknown key \"%s\"\n", keys.u.tab.key[i], v.u.s);
      continue;
    }
    snprintf(c->key_button[c->keys_count], sizeof c->key_button[0], "%s", keys.u.tab.key[i]);
    snprintf(c->key_code[c->keys_count], sizeof c->key_code[0], "%s", code);
    c->keys_count++;
  }
  toml_datum_t names = toml_get(controls, "buttons");
  if (names.type != TOML_TABLE) return;
  for (int i = 0; i < names.u.tab.size && c->names_count < MAX_CONTROL_NAMES; i++) {
    toml_datum_t v = names.u.tab.value[i];
    if (v.type != TOML_STRING) continue;
    snprintf(c->button[c->names_count], sizeof c->button[0], "%s", names.u.tab.key[i]);
    snprintf(c->name[c->names_count], sizeof c->name[0], "%s", v.u.s);
    c->names_count++;
  }
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

  toml_datum_t top = result.toptab;
  read_engine(toml_get(top, "engine"), &out->engine);
  read_game(toml_get(top, "game"), out);
  read_scripts(toml_get(top, "scripts"), &out->scripts);
  read_fonts(toml_get(top, "fonts"), out);
  read_launcher(toml_get(top, "launcher"), &out->launcher);
  read_saves(toml_get(top, "saves"), out);   // despues de [game]: usa game_dir
  read_controls(toml_get(top, "controls"), out);

  // mods/ junto al config
  const char *slash = strrchr(filename, '/');
  if (slash) snprintf(out->mods.root, sizeof(out->mods.root), "%.*s/mods", (int)(slash - filename), filename);
  else snprintf(out->mods.root, sizeof(out->mods.root), "mods");

  toml_free(result);
  return 0;
}

const char *config_game_id(const Config *cfg) {
  return cfg->saves.id[0] ? cfg->saves.id : cfg->launcher.title[0] ? cfg->launcher.title : cfg->engine.name;
}
