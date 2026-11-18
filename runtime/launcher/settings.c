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

// quixjs-settings.toml. Ver settings.h.

#include <stdio.h>
#include <string.h>
#include "tomlc17.h"
#include "cheats/cheats.h"
#include "settings.h"

// quixjs-settings.toml en la carpeta del toml del juego
static void settings_path(const char *config_path, char *out, size_t room) {
  const char *slash = strrchr(config_path, '/');
  if (slash) snprintf(out, room, "%.*s/quixjs-settings.toml", (int)(slash - config_path), config_path);
  else snprintf(out, room, "quixjs-settings.toml");
}

void settings_load(Settings *s, const char *config_path) {
  memset(s, 0, sizeof(*s));
  char path[1024];
  settings_path(config_path, path, sizeof(path));
  FILE *f = fopen(path, "r");
  if (!f) return;
  toml_result_t r = toml_parse_file(f);
  fclose(f);
  if (!r.ok) return;
  toml_datum_t v = toml_get(r.toptab, "show_fps");
  if (v.type == TOML_BOOLEAN) s->show_fps = v.u.boolean;
  v = toml_get(r.toptab, "fullscreen");
  if (v.type == TOML_BOOLEAN) s->fullscreen = v.u.boolean;
  v = toml_get(r.toptab, "profile");
  if (v.type == TOML_BOOLEAN) s->profile = v.u.boolean;
  v = toml_get(r.toptab, "mods");
  if (v.type == TOML_ARRAY) {
    s->mods_set = 1;
    for (int i = 0; i < v.u.arr.size && s->mods_count < MAX_SETTING_MODS; i++) {
      toml_datum_t e = v.u.arr.elem[i];
      if (e.type == TOML_STRING) snprintf(s->mods[s->mods_count++], sizeof s->mods[0], "%s", e.u.s);
    }
  }
  v = toml_get(r.toptab, "cheats");
  if (v.type == TOML_ARRAY) {
    for (int i = 0; i < v.u.arr.size && s->cheats_count < MAX_SETTING_CHEATS; i++) {
      toml_datum_t e = v.u.arr.elem[i];
      if (e.type == TOML_STRING) snprintf(s->cheats[s->cheats_count++], sizeof s->cheats[0], "%s", e.u.s);
    }
  }
  toml_free(r);
}

int settings_save(const Settings *s, const char *config_path) {
  char path[1024], tmp[1100];
  settings_path(config_path, path, sizeof(path));
  snprintf(tmp, sizeof(tmp), "%s.tmp", path);
  FILE *f = fopen(tmp, "w");
  if (!f) {
    fprintf(stderr, "[settings] cannot write %s\n", path);
    return -1;
  }
  fprintf(f, "# quixjs-settings.toml: lo que se cambia en el launcher (Options)\n");
  fprintf(f, "show_fps = %s\n", s->show_fps ? "true" : "false");
  fprintf(f, "profile  = %s\n", s->profile ? "true" : "false");
  if (s->fullscreen) fprintf(f, "fullscreen = true\n");
  fprintf(f, "cheats   = [");
  for (int i = 0; i < s->cheats_count; i++) fprintf(f, "%s\"%s\"", i ? ", " : "", s->cheats[i]);
  fprintf(f, "]\n");
  if (s->mods_set) {
    fprintf(f, "mods     = [");
    for (int i = 0; i < s->mods_count; i++) fprintf(f, "%s\"%s\"", i ? ", " : "", s->mods[i]);
    fprintf(f, "]\n");
  }
  int ok = fclose(f) == 0;
  if (ok) ok = rename(tmp, path) == 0;
  return ok ? 0 : -1;
}

void settings_apply_cheats(const Settings *s) {
  const char *ids[MAX_SETTING_CHEATS];
  for (int i = 0; i < s->cheats_count; i++) ids[i] = s->cheats[i];
  cheats_set_active(ids, s->cheats_count);
}
