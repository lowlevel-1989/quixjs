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


// runtime/transfer/saves/saves.c: las partidas (ver saves.h): que es una
// partida ([saves] files), buscarlas, su fecha y su CRC. Empaquetar y
// guardar las que llegan esta en pack.c; el backup, en backup.c.
//
// Leer, empaquetar, guardar y restaurar las partidas. Basado en tamatama
// (src/transfer/saves.c), que pasa un solo fichero; aqui son los que diga
// [saves] files.
//
// El paquete, en texto y despues los bytes:
//
//   QUIXSAVE 1
//   <bytes> <ruta relativa>     una linea por fichero
//   <linea vacia>
//   <los ficheros, uno detras de otro>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <fnmatch.h>
#include <ftw.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "core/sandbox.h"
#include "common.h"

SavesState saves_st;

// Una ruta relativa que no sale de su carpeta: ni absoluta ni con ".."
static int safe_relative(const char *rel) {
  if (!rel[0] || rel[0] == '/') return 0;
  for (const char *p = rel; *p;) {
    const char *slash = strchr(p, '/');
    size_t n = slash ? (size_t)(slash - p) : strlen(p);
    if (n == 2 && p[0] == '.' && p[1] == '.') return 0;
    if (!slash) break;
    p = slash + 1;
  }
  return 1;
}


int saves_setup(const char *dir, const char *const *list, int count) {
  snprintf(saves_st.dir, sizeof saves_st.dir, "%s", dir);
  saves_st.pattern_count = 0;
  for (int i = 0; i < count && saves_st.pattern_count < SAVES_MAX_PATTERNS; i++) {
    if (!safe_relative(list[i]) || strncmp(list[i], ".quixjs", 7) == 0) {
      fprintf(stderr, "[saves] ignored pattern '%s': it must stay inside saves_st.dir\n", list[i]);
      continue;
    }
    snprintf(saves_st.patterns[saves_st.pattern_count++], PATH_MAX, "%s", list[i]);
  }
  return saves_st.pattern_count;
}


int saves_configured(void) {
  return saves_st.pattern_count > 0;
}


int saves_allowed(const char *rel) {
  if (!safe_relative(rel)) return 0;
  for (int i = 0; i < saves_st.pattern_count; i++) {
    if (fnmatch(saves_st.patterns[i], rel, FNM_PATHNAME) == 0) return 1;
  }
  return 0;
}


void saves_root_of(int backup, char *out, size_t room) {
  if (backup) snprintf(out, room, "%s/.quixjs/backup", saves_st.dir);
  else snprintf(out, room, "%s", saves_st.dir);
}

// Los ficheros de root que casan con los patrones
static int find_in(const char *root, SaveSet *set) {
  memset(set, 0, sizeof *set);
  size_t root_len = strlen(root);
  for (int i = 0; i < saves_st.pattern_count; i++) {
    char full[PATH_MAX * 2];
    snprintf(full, sizeof full, "%s/%s", root, saves_st.patterns[i]);
    glob_t g;
    if (glob(full, 0, NULL, &g) != 0) continue;
    for (size_t k = 0; k < g.gl_pathc && set->count < SAVES_MAX_FILES; k++) {
      const char *rel = g.gl_pathv[k] + root_len + 1;
      struct stat info;
      if (sandbox_stat(g.gl_pathv[k], &info, "transfer save: send") != 0 || !S_ISREG(info.st_mode)) continue;
      // un enlace simbolico que lleva fuera de la carpeta de las partidas
      // (en el zip de un port) no se manda: runtime/core/sandbox.c
      if (!sandbox_check_write(g.gl_pathv[k], "transfer save: send")) continue;
      int seen = 0;
      for (int j = 0; j < set->count; j++) {
        if (strcmp(set->files[j].path, rel) == 0) seen = 1;
      }
      if (seen) continue;
      SaveFile *f = &set->files[set->count++];
      snprintf(f->path, sizeof f->path, "%s", rel);
      f->size = (long)info.st_size;
      f->date = info.st_mtime;
      set->total += f->size;
      if (f->date > set->date) set->date = f->date;
    }
    globfree(&g);
  }
  snprintf(set->summary, sizeof set->summary, "%d file%s", set->count, set->count == 1 ? "" : "s");
  return set->count > 0;
}


int saves_find(int backup, SaveSet *set) {
  char root[PATH_MAX];
  saves_root_of(backup, root, sizeof root);
  return find_in(root, set);
}


uint32_t saves_crc32(const uint8_t *data, size_t n) {
  uint32_t crc = 0xffffffffu;
  while (n--) {
    crc ^= *data++;
    for (int k = 0; k < 8; k++) crc = crc >> 1 ^ (0xedb88320u & (0u - (crc & 1)));
  }
  return ~crc;
}


void saves_date_text(time_t date, char *out, size_t room) {
  struct tm parts;
  if (!localtime_r(&date, &parts) || !strftime(out, room, "%Y-%m-%d %H:%M", &parts)) snprintf(out, room, "?");
}


void saves_keep(const char *rel) {
  snprintf(saves_st.keep, sizeof saves_st.keep, "%s", rel ? rel : "");
}


int saves_is_save(const char *path) {
  if (!path || !saves_st.dir[0]) return 0;
  char abs_dir[PATH_MAX], abs_path[PATH_MAX];
  if (!realpath(saves_st.dir, abs_dir)) return 0;
  // la partida puede no existir aun: se resuelve su carpeta
  char dir[PATH_MAX];
  snprintf(dir, sizeof dir, "%s", path);
  char *slash = strrchr(dir, '/');
  const char *base = slash ? slash + 1 : path;
  if (slash) *slash = 0;
  else snprintf(dir, sizeof dir, ".");
  char real_dir[PATH_MAX];
  if (!realpath(dir, real_dir)) return 0;
  snprintf(abs_path, sizeof abs_path, "%s/%s", real_dir, base);
  size_t n = strlen(abs_dir);
  if (strncmp(abs_path, abs_dir, n) != 0 || abs_path[n] != '/') return 0;
  return saves_allowed(abs_path + n + 1);
}
