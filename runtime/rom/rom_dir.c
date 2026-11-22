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

// La rom en una carpeta: los tests del motor, o el juego descomprimido en el
// PC. Cada ruta, la de disco dentro de la carpeta. Ver rom.h.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "core/url.h"
#include "rom.h"

static char base[PATH_MAX];   // la carpeta donde empieza el juego

static void full(const char *rel, char *out, size_t room) {
  snprintf(out, room, "%s%s%s", base, rel[0] ? "/" : "", rel);
}

static uint8_t *dir_read(const char *rel, size_t *len) {
  char p[PATH_MAX * 2];
  full(rel, p, sizeof p);
  FILE *f = url_fopen_loose(p);   // tambien los nombres no ASCII de una SD FAT
  if (!f) return NULL;
  fseeko(f, 0, SEEK_END);
  off_t size = ftello(f);
  fseeko(f, 0, SEEK_SET);
  uint8_t *buf = size >= 0 ? malloc((size_t)size + 1) : NULL;
  size_t got = buf ? fread(buf, 1, (size_t)size, f) : 0;
  fclose(f);
  if (!buf) return NULL;
  buf[got] = 0;
  if (len) *len = got;
  return buf;
}

static FILE *dir_fopen(const char *rel) {
  char p[PATH_MAX * 2];
  full(rel, p, sizeof p);
  return url_fopen_loose(p);
}

static int dir_stat(const char *rel, long long *size, long long *mtime, int *is_dir) {
  char p[PATH_MAX * 2];
  full(rel, p, sizeof p);
  struct stat st;
  if (stat(p, &st) != 0) return -1;
  if (size) *size = (long long)st.st_size;
  if (mtime) *mtime = (long long)st.st_mtime;
  if (is_dir) *is_dir = S_ISDIR(st.st_mode);
  return 0;
}

static int dir_list(const char *rel, void (*each)(void *user, const char *name, int is_dir), void *user) {
  char p[PATH_MAX * 2];
  full(rel, p, sizeof p);
  DIR *d = opendir(p);
  if (!d) return -1;
  int count = 0;
  struct dirent *e;
  while ((e = readdir(d)) != NULL) {
    if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
    char child[PATH_MAX * 3];
    snprintf(child, sizeof child, "%s/%s", p, e->d_name);
    struct stat st;
    each(user, e->d_name, stat(child, &st) == 0 && S_ISDIR(st.st_mode));
    count++;
  }
  closedir(d);
  return count;
}

static void *dir_map(const char *rel, size_t *len, size_t *delta) {
  char p[PATH_MAX * 2];
  full(rel, p, sizeof p);
  int fd = open(p, O_RDONLY | O_CLOEXEC);
  if (fd < 0) return NULL;
  struct stat st;
  size_t size = fstat(fd, &st) == 0 ? (size_t)st.st_size : 0;
  void *map = size ? mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0) : MAP_FAILED;
  close(fd);
  if (map == MAP_FAILED) return NULL;
  *len = size;
  *delta = 0;
  return map;
}

static const RomKind dir_kind = { dir_read, dir_fopen, dir_stat, dir_list, dir_map };

const RomKind *rom_dir_open(const char *rom, const char *root) {
  snprintf(base, sizeof base, "%s%s%s", rom, root[0] ? "/" : "", root);
  return &dir_kind;
}
