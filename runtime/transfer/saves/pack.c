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

// runtime/transfer/saves/pack.c: el paquete de Transfer Save: empaquetar las
// partidas para enviarlas (saves_pack) y guardar las que llegan
// (saves_store: las de ahora pasan al backup).

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


static uint8_t *read_file(const char *path, long size) {
  FILE *file = sandbox_fopen(path, "rb", "transfer save: send");
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
    snprintf(full, sizeof full, "%s/%s", saves_st.dir, set->files[i].path);
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
  snprintf(inbox, sizeof inbox, "%s/.quixjs/incoming", saves_st.dir);
  saves_root_of(1, backup, sizeof backup);
  saves_remove_tree(inbox);

  // 1. lo nuevo, aparte
  for (int i = 0; i < incoming.count; i++) {
    char path[PATH_MAX * 2];
    snprintf(path, sizeof path, "%s/%s", inbox, incoming.files[i].path);
    if (!sandbox_check_write(path, "transfer save: receive")) {
      saves_remove_tree(inbox);
      return 0;
    }
    saves_make_parents(path);
    FILE *out = sandbox_fopen(path, "wb", "transfer save: receive");
    int ok = out && fwrite(bundle + at, 1, (size_t)incoming.files[i].size, out) == (size_t)incoming.files[i].size;
    if (out && fclose(out) != 0) ok = 0;
    if (!ok) {
      fprintf(stderr, "[saves] cannot write %s\n", path);
      saves_remove_tree(inbox);
      return 0;
    }
    at += incoming.files[i].size;
  }

  // 2. las de ahora pasan a ser el backup (el backup anterior se pierde)
  SaveSet current;
  saves_remove_tree(backup);
  if (saves_find(0, &current) && !saves_move_set(&current, saves_st.dir, backup)) {
    saves_move_set(&current, backup, saves_st.dir);
    saves_remove_tree(inbox);
    return 0;
  }

  // 3. lo nuevo, en su sitio
  int ok = saves_move_set(&incoming, inbox, saves_st.dir);
  saves_remove_tree(inbox);
  return ok;
}
