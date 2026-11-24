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

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "tomlc17.h"
#include "game/cheats/cheats.h"
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
  v = toml_get(r.toptab, "vibration");
  if (v.type == TOML_BOOLEAN) s->no_vibration = !v.u.boolean;
  v = toml_get(r.toptab, "hardcore");
  if (v.type == TOML_BOOLEAN) s->hardcore = v.u.boolean;
  // pixel_perfect = true/false: el de antes de Scaling
  v = toml_get(r.toptab, "pixel_perfect");
  if (v.type == TOML_BOOLEAN) s->scaling = v.u.boolean ? 3 : 1;
  v = toml_get(r.toptab, "scaling");
  if (v.type == TOML_STRING) {
    if (!strcmp(v.u.s, "sharp")) s->scaling = 1;
    else if (!strcmp(v.u.s, "smooth")) s->scaling = 2;
    else if (!strcmp(v.u.s, "pixel_perfect")) s->scaling = 3;
  }
  v = toml_get(r.toptab, "quality");
  if (v.type == TOML_STRING) {
    if (!strcmp(v.u.s, "low")) s->quality = 1;
    else if (!strcmp(v.u.s, "medium")) s->quality = 2;
    else if (!strcmp(v.u.s, "high")) s->quality = 3;
  }
  v = toml_get(r.toptab, "fps");
  if (v.type == TOML_INT64 && (v.u.int64 == 30 || v.u.int64 == 60)) s->fps = (int)v.u.int64;
  v = toml_get(r.toptab, "mods");
  if (v.type == TOML_ARRAY) {
    s->mods_set = 1;
    for (int i = 0; i < v.u.arr.size && s->mods_count < MAX_SETTING_MODS; i++) {
      toml_datum_t e = v.u.arr.elem[i];
      // un nombre de carpeta, sin rutas (como [game] mods): este archivo
      // tambien se puede editar a mano
      if (e.type == TOML_STRING && e.u.s[0] && !strchr(e.u.s, '/') && strcmp(e.u.s, "..") != 0 &&
          strcmp(e.u.s, ".") != 0) {
        snprintf(s->mods[s->mods_count++], sizeof s->mods[0], "%s", e.u.s);
      }
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
  fprintf(f, "vibration = %s\n", s->no_vibration ? "false" : "true");
  if (s->hardcore) fprintf(f, "hardcore = true\n");
  if (s->scaling >= 1 && s->scaling <= 3)
    fprintf(f, "scaling  = \"%s\"\n", s->scaling == 1 ? "sharp" : s->scaling == 2 ? "smooth" : "pixel_perfect");
  if (s->quality >= 1 && s->quality <= 3) fprintf(f, "quality = \"%s\"\n", s->quality == 1 ? "low" : s->quality == 2 ? "medium" : "high");
  if (s->fps) fprintf(f, "fps      = %d\n", s->fps);
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
  cheats_set_hardcore(s->hardcore);
  cheats_set_active(ids, s->cheats_count);
}

int settings_cheat_index(const Settings *s, const char *id) {
  for (int i = 0; i < s->cheats_count; i++) {
    if (strcmp(s->cheats[i], id) == 0) return i;
  }
  return -1;
}

int settings_mod_index(const Settings *s, const char *name) {
  for (int i = 0; i < s->mods_count; i++) {
    if (strcmp(s->mods[i], name) == 0) return i;
  }
  return -1;
}

void settings_toggle_cheat(Settings *s, const char *id) {
  int k = settings_cheat_index(s, id);
  if (k >= 0) {
    memmove(s->cheats[k], s->cheats[k + 1], sizeof s->cheats[0] * (size_t)(s->cheats_count - k - 1));
    s->cheats_count--;
  } else if (s->cheats_count < MAX_SETTING_CHEATS) {
    snprintf(s->cheats[s->cheats_count++], sizeof s->cheats[0], "%s", id);
  }
}

// Los activos llevan su numero: si dos traen el mismo archivo, gana el de
// numero mas alto (el ultimo de la lista)
void settings_toggle_mod(Settings *s, const char (*defaults)[64], int count, const char *name) {
  if (!s->mods_set) {
    s->mods_count = 0;
    for (int i = 0; i < count && i < MAX_SETTING_MODS; i++) {
      snprintf(s->mods[s->mods_count++], sizeof s->mods[0], "%s", defaults[i]);
    }
  }
  int k = settings_mod_index(s, name);
  if (k >= 0) {
    memmove(s->mods[k], s->mods[k + 1], sizeof s->mods[0] * (size_t)(s->mods_count - k - 1));
    s->mods_count--;
  } else if (s->mods_count < MAX_SETTING_MODS) {
    snprintf(s->mods[s->mods_count++], sizeof s->mods[0], "%s", name);
  }
  s->mods_set = 1;
}

static int mod_cmp(const void *a, const void *b) { return strcmp((const char *)a, (const char *)b); }

int settings_mod_dirs(const char *root, char names[][64]) {
  int n = 0;
  DIR *d = opendir(root);
  if (!d) return 0;
  struct dirent *e;
  while ((e = readdir(d)) != NULL && n < MAX_MOD_DIRS) {
    if (e->d_name[0] == '.' || strlen(e->d_name) >= 64) continue;
    char path[1024];
    struct stat st;
    snprintf(path, sizeof path, "%s/%s", root, e->d_name);
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) snprintf(names[n++], 64, "%s", e->d_name);
  }
  closedir(d);
  qsort(names, (size_t)n, 64, mod_cmp);
  return n;
}
