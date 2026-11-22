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
#include "saves.h"
#include "core/sandbox.h"

static char game_dir[PATH_MAX];
static char patterns[SAVES_MAX_PATTERNS][PATH_MAX];
static int  pattern_count = 0;

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
  snprintf(game_dir, sizeof game_dir, "%s", dir);
  pattern_count = 0;
  for (int i = 0; i < count && pattern_count < SAVES_MAX_PATTERNS; i++) {
    if (!safe_relative(list[i]) || strncmp(list[i], ".quixjs", 7) == 0) {
      fprintf(stderr, "[saves] ignored pattern '%s': it must stay inside game_dir\n", list[i]);
      continue;
    }
    snprintf(patterns[pattern_count++], PATH_MAX, "%s", list[i]);
  }
  return pattern_count;
}

int saves_configured(void) {
  return pattern_count > 0;
}

int saves_allowed(const char *rel) {
  if (!safe_relative(rel)) return 0;
  for (int i = 0; i < pattern_count; i++) {
    if (fnmatch(patterns[i], rel, FNM_PATHNAME) == 0) return 1;
  }
  return 0;
}

static void root_of(int backup, char *out, size_t room) {
  if (backup) snprintf(out, room, "%s/.quixjs/backup", game_dir);
  else snprintf(out, room, "%s", game_dir);
}

// Los ficheros de root que casan con los patrones
static int find_in(const char *root, SaveSet *set) {
  memset(set, 0, sizeof *set);
  size_t root_len = strlen(root);
  for (int i = 0; i < pattern_count; i++) {
    char full[PATH_MAX * 2];
    snprintf(full, sizeof full, "%s/%s", root, patterns[i]);
    glob_t g;
    if (glob(full, 0, NULL, &g) != 0) continue;
    for (size_t k = 0; k < g.gl_pathc && set->count < SAVES_MAX_FILES; k++) {
      const char *rel = g.gl_pathv[k] + root_len + 1;
      struct stat info;
      if (stat(g.gl_pathv[k], &info) != 0 || !S_ISREG(info.st_mode)) continue;
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
  root_of(backup, root, sizeof root);
  return find_in(root, set);
}

static uint8_t *read_file(const char *path, long size) {
  FILE *file = fopen(path, "rb");
  if (!file) return NULL;
  uint8_t *data = malloc(size > 0 ? (size_t)size : 1);
  if (data && fread(data, 1, (size_t)size, file) != (size_t)size) {
    free(data);
    data = NULL;
  }
  fclose(file);
  return data;
}

uint8_t *saves_pack(const SaveSet *set, long *size) {
  if (set->total > SAVES_MAX_BYTES) return NULL;
  size_t head = 16;
  for (int i = 0; i < set->count; i++) head += strlen(set->files[i].path) + 24;
  uint8_t *out = malloc(head + (size_t)set->total);
  if (!out) return NULL;
  int n = sprintf((char *)out, "QUIXSAVE 1\n");
  for (int i = 0; i < set->count; i++) n += sprintf((char *)out + n, "%ld %s\n", set->files[i].size, set->files[i].path);
  out[n++] = '\n';
  for (int i = 0; i < set->count; i++) {
    char full[PATH_MAX * 2];
    snprintf(full, sizeof full, "%s/%s", game_dir, set->files[i].path);
    uint8_t *data = sandbox_check_write(full, "transfer save: send") ? read_file(full, set->files[i].size) : NULL;
    if (!data) {
      free(out);
      return NULL;
    }
    memcpy(out + n, data, (size_t)set->files[i].size);
    n += (int)set->files[i].size;
    free(data);
  }
  *size = n;
  return out;
}

// mkdir -p de las carpetas de path (sin el ultimo componente)
static void make_parents(const char *path) {
  char tmp[PATH_MAX * 2];
  snprintf(tmp, sizeof tmp, "%s", path);
  for (char *p = tmp + 1; *p; p++) {
    if (*p != '/') continue;
    *p = 0;
    mkdir(tmp, 0755);
    *p = '/';
  }
}

static int remove_entry(const char *path, const struct stat *sb, int flag, struct FTW *ftw) {
  (void)sb; (void)flag; (void)ftw;
  remove(path);
  return 0;
}

// Borra una carpeta de .quixjs/ entera
static void remove_tree(const char *path) {
  nftw(path, remove_entry, 16, FTW_DEPTH | FTW_PHYS);
}

// Mueve los ficheros de set de la carpeta from a la carpeta to
static int move_set(const SaveSet *set, const char *from, const char *to) {
  int ok = 1;
  for (int i = 0; i < set->count; i++) {
    char a[PATH_MAX * 2], b[PATH_MAX * 2];
    snprintf(a, sizeof a, "%s/%s", from, set->files[i].path);
    snprintf(b, sizeof b, "%s/%s", to, set->files[i].path);
    // ni por un enlace simbolico fuera de la carpeta de las partidas
    if (!sandbox_check_write(a, "transfer save: move") || !sandbox_check_write(b, "transfer save: move")) {
      ok = 0;
      continue;
    }
    make_parents(b);
    if (rename(a, b) != 0) {
      fprintf(stderr, "[saves] cannot move %s to %s: %s\n", a, b, strerror(errno));
      ok = 0;
    }
  }
  return ok;
}

// Lee la cabecera del paquete en set (rutas y tamanos). Devuelve donde
// empiezan los datos, o -1 si esta mal formado o trae algo que no es partida.
static long parse_bundle(const uint8_t *bundle, long size, SaveSet *set) {
  memset(set, 0, sizeof *set);
  if (size < 12 || memcmp(bundle, "QUIXSAVE 1\n", 11) != 0) return -1;
  long at = 11;
  for (;;) {
    const uint8_t *end = memchr(bundle + at, '\n', (size_t)(size - at));
    if (!end) return -1;
    long len = (long)(end - (bundle + at));
    if (len == 0) {
      at++;
      break;
    }
    if (set->count == SAVES_MAX_FILES || len >= PATH_MAX) return -1;
    char line[PATH_MAX + 32];
    memcpy(line, bundle + at, (size_t)len);
    line[len] = 0;
    char *space = strchr(line, ' ');
    if (!space) return -1;
    *space = 0;
    char *stop;
    long fsize = strtol(line, &stop, 10);
    if (*stop || fsize < 0 || fsize > SAVES_MAX_BYTES || !saves_allowed(space + 1)) return -1;
    SaveFile *f = &set->files[set->count++];
    snprintf(f->path, sizeof f->path, "%s", space + 1);
    f->size = fsize;
    set->total += fsize;
    at += len + 1;
  }
  if (set->count == 0 || set->total != size - at) return -1;
  return at;
}

int saves_store(const uint8_t *bundle, long size) {
  SaveSet incoming;
  long at = parse_bundle(bundle, size, &incoming);
  if (at < 0) {
    fprintf(stderr, "[saves] the received package is not valid\n");
    return 0;
  }
  char inbox[PATH_MAX], backup[PATH_MAX];
  snprintf(inbox, sizeof inbox, "%s/.quixjs/incoming", game_dir);
  root_of(1, backup, sizeof backup);
  remove_tree(inbox);

  // 1. lo nuevo, aparte
  for (int i = 0; i < incoming.count; i++) {
    char path[PATH_MAX * 2];
    snprintf(path, sizeof path, "%s/%s", inbox, incoming.files[i].path);
    if (!sandbox_check_write(path, "transfer save: receive")) {
      remove_tree(inbox);
      return 0;
    }
    make_parents(path);
    FILE *out = fopen(path, "wb");
    int ok = out && fwrite(bundle + at, 1, (size_t)incoming.files[i].size, out) == (size_t)incoming.files[i].size;
    if (out && fclose(out) != 0) ok = 0;
    if (!ok) {
      fprintf(stderr, "[saves] cannot write %s\n", path);
      remove_tree(inbox);
      return 0;
    }
    at += incoming.files[i].size;
  }

  // 2. las de ahora pasan a ser el backup (el backup anterior se pierde)
  SaveSet current;
  remove_tree(backup);
  if (saves_find(0, &current) && !move_set(&current, game_dir, backup)) {
    move_set(&current, backup, game_dir);
    remove_tree(inbox);
    return 0;
  }

  // 3. lo nuevo, en su sitio
  int ok = move_set(&incoming, inbox, game_dir);
  remove_tree(inbox);
  return ok;
}

int saves_restore_backup(void) {
  SaveSet backup_set, current;
  if (!saves_find(1, &backup_set)) return 0;
  char backup[PATH_MAX], swap[PATH_MAX];
  root_of(1, backup, sizeof backup);
  snprintf(swap, sizeof swap, "%s/.quixjs/swap", game_dir);
  remove_tree(swap);
  int had = saves_find(0, &current);
  if (had && !move_set(&current, game_dir, swap)) return 0;
  if (!move_set(&backup_set, backup, game_dir)) {
    if (had) move_set(&current, swap, game_dir);
    return 0;
  }
  remove_tree(backup);
  if (had) move_set(&current, swap, backup);
  remove_tree(swap);
  return 1;
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
