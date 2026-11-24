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

// runtime/core/url/fs.c: los archivos del juego en disco o en la rom: su
// stat, listar una carpeta y mapear un archivo.


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "core/config/loader.h"
#include "url.h"
#include "core/sandbox.h"
#include "rom/rom.h"

// ---- en disco o en la rom

int url_stat(const char *path, long long *size, long long *mtime, int *is_dir) {
  struct stat st;
  if (sandbox_stat(path, &st, "file") == 0) {
    if (size) *size = (long long)st.st_size;
    if (mtime) *mtime = (long long)st.st_mtime;
    if (is_dir) *is_dir = S_ISDIR(st.st_mode);
    return 0;
  }
  return rom_stat(path, size, mtime, is_dir);
}

int url_exists(const char *path) { return url_stat(path, NULL, NULL, NULL) == 0; }

typedef struct {
  char (*names)[256];
  int count, room;
  void (*each)(void *user, const char *name, int is_dir);
  void *user;
} ListCtx;

static void list_add(void *u, const char *name, int is_dir) {
  ListCtx *c = u;
  for (int i = 0; i < c->count; i++) if (strcmp(c->names[i], name) == 0) return;
  if (c->count < c->room) snprintf(c->names[c->count++], 256, "%s", name);
  c->each(c->user, name, is_dir);
}

int url_list_dir(const char *dir, void (*each)(void *user, const char *name, int is_dir), void *user) {
  ListCtx c = { calloc(4096, 256), 0, 4096, each, user };
  if (!c.names) return -1;
  int found = 0;
  DIR *d = sandbox_opendir(dir, "file");
  if (d) {
    found = 1;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
      if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
      char full[2048];
      snprintf(full, sizeof full, "%s/%s", dir, e->d_name);
      struct stat st;
      list_add(&c, e->d_name, sandbox_stat(full, &st, "file") == 0 && S_ISDIR(st.st_mode));
    }
    closedir(d);
  }
  if (rom_list(dir, list_add, &c) >= 0) found = 1;
  free(c.names);
  return found ? c.count : -1;
}

void *url_map(const char *path, size_t *len, size_t *delta) {
  int fd = sandbox_open(path, O_RDONLY, 0, "file");
  if (fd < 0) return rom_map(path, len, delta);   // en la rom
  struct stat st;
  size_t size = fstat(fd, &st) == 0 ? (size_t)st.st_size : 0;
  void *map = size ? mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0) : MAP_FAILED;
  close(fd);
  if (map == MAP_FAILED) return NULL;
  *len = size;
  *delta = 0;
  return map;
}
