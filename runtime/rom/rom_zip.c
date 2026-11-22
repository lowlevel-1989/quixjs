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

// La rom en un .zip (runtime/zip): cada ruta, su archivo dentro del zip,
// debajo de root. Si viene comprimido, se pasa una vez a sin comprimir. Ver
// rom.h.

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include "zip/zip.h"
#include "rom.h"

static Zip *zip;
static char prefix[512];   // root dentro del zip, con "/" detras ("" la raiz)

// prefix + rel, sin "..": 0 si sale del zip
static int zip_name(const char *rel, char *out, size_t room) {
  char tmp[PATH_MAX];
  snprintf(tmp, sizeof tmp, "%s%s", prefix, rel);
  size_t n = 0;
  out[0] = 0;
  char *save = NULL;
  for (char *part = strtok_r(tmp, "/", &save); part; part = strtok_r(NULL, "/", &save)) {
    if (strcmp(part, ".") == 0) continue;
    if (strcmp(part, "..") == 0) {
      if (!n) return 0;
      char *slash = strrchr(out, '/');
      if (slash) *slash = 0;
      else out[0] = 0;
      n = strlen(out);
      continue;
    }
    n += (size_t)snprintf(out + n, n < room ? room - n : 0, "%s%s", n ? "/" : "", part);
  }
  return 1;
}

static const ZipEntry *entry(const char *rel) {
  char name[PATH_MAX];
  if (!zip_name(rel, name, sizeof name)) return NULL;
  const ZipEntry *e = zip_find(zip, name);
  return e && e->name[strlen(e->name) - 1] != '/' ? e : NULL;
}

// una carpeta del zip: con su entrada o con algo dentro
static int is_dir(const char *rel) {
  char name[PATH_MAX];
  if (!zip_name(rel, name, sizeof name)) return 0;
  size_t n = strlen(name);
  if (n == 0 || (strncmp(name, prefix, n) == 0 && prefix[n] == '/')) return 1;
  for (int i = 0; i < zip_count(zip); i++) {
    const char *e = zip_entry(zip, i)->name;
    if (strncmp(e, name, n) == 0 && (e[n] == '/' || name[n - 1] == '/')) return 1;
  }
  return 0;
}

static uint8_t *z_read(const char *rel, size_t *len) {
  const ZipEntry *e = entry(rel);
  return e ? zip_read(zip, e, len) : NULL;
}

static FILE *z_fopen(const char *rel) {
  const ZipEntry *e = entry(rel);
  return e ? zip_fopen(zip, e) : NULL;
}

static int z_stat(const char *rel, long long *size, long long *mtime, int *dir) {
  const ZipEntry *e = entry(rel);
  int d = 0;
  if (!e && !(d = is_dir(rel))) return -1;
  if (size) *size = e ? (long long)e->size : 0;
  if (mtime) *mtime = e ? zip_mtime(e) : 0;
  if (dir) *dir = d;
  return 0;
}

static int z_list(const char *rel, void (*each)(void *user, const char *name, int is_dir), void *user) {
  if (!is_dir(rel)) return -1;
  char name[PATH_MAX];
  zip_name(rel, name, sizeof name - 1);
  size_t n = strlen(name);
  if (n) {
    name[n++] = '/';
    name[n] = 0;
  }
  // cada hijo una vez (un zip puede no traer las carpetas como entradas)
  char **seen = calloc((size_t)zip_count(zip) + 1, sizeof(char *));
  int count = 0;
  for (int i = 0; seen && i < zip_count(zip); i++) {
    const char *e = zip_entry(zip, i)->name;
    if (strncmp(e, name, n) != 0 || !e[n]) continue;
    const char *child = e + n, *slash = strchr(child, '/');
    size_t clen = slash ? (size_t)(slash - child) : strlen(child);
    int dup = 0;
    for (int k = 0; k < count && !dup; k++) dup = strlen(seen[k]) == clen && strncmp(seen[k], child, clen) == 0;
    if (dup) continue;
    seen[count] = strndup(child, clen);
    each(user, seen[count], slash != NULL);
    count++;
  }
  for (int k = 0; k < count; k++) free(seen[k]);
  free(seen);
  return count;
}

static void *z_map(const char *rel, size_t *len, size_t *delta) {
  const ZipEntry *e = entry(rel);
  if (!e || e->method != 0) return NULL;   // comprimido: no se puede
  long page = sysconf(_SC_PAGESIZE);
  uint64_t start = e->data - e->data % (uint64_t)page;
  size_t d = (size_t)(e->data - start);
  void *map = e->size ? mmap(NULL, (size_t)e->size + d, PROT_READ, MAP_PRIVATE, zip_fd(zip), (off_t)start)
                      : MAP_FAILED;
  if (map == MAP_FAILED) return NULL;
  *len = (size_t)e->size;
  *delta = d;
  return (uint8_t *)map + d;
}

static const RomKind zip_kind = { z_read, z_fopen, z_stat, z_list, z_map };

// comprimido: una vez a sin comprimir, si hay sitio (si no, se lee
// comprimido: va, pero cada archivo se descomprime al leerlo)
static void uncompress_once(const char *rom,
                            void (*progress)(void *user, uint64_t done, uint64_t total), void *user) {
  char tmp[PATH_MAX + 8], dir[PATH_MAX];
  snprintf(tmp, sizeof tmp, "%s.tmp", rom);
  snprintf(dir, sizeof dir, "%s", rom);
  char *slash = strrchr(dir, '/');
  if (slash) *slash = 0;
  else snprintf(dir, sizeof dir, ".");
  struct statvfs fs;
  uint64_t need = zip_store_size(zip);
  uint64_t free_bytes = statvfs(dir, &fs) == 0 ? (uint64_t)fs.f_bavail * fs.f_frsize : 0;
  if (free_bytes < need + (16u << 20)) {
    fprintf(stderr, "[rom] %s is compressed and there is not enough free space to uncompress it "
                    "(%llu MB needed, %llu MB free): it is read compressed (slower)\n",
            rom, (unsigned long long)(need >> 20), (unsigned long long)(free_bytes >> 20));
    return;
  }
  printf("[rom] %s is compressed: uncompressing it once (%llu MB)\n", rom, (unsigned long long)(need >> 20));
  zip_close(zip);
  zip = NULL;
  if (zip_convert_store(rom, tmp, progress, user) == 0 && rename(tmp, rom) == 0) {
    printf("[rom] %s uncompressed\n", rom);
  } else {
    fprintf(stderr, "[rom] could not uncompress %s: it is read compressed (slower)\n", rom);
    remove(tmp);
  }
  zip = zip_open(rom);
}

const RomKind *rom_zip_open(const char *rom, const char *root,
                            void (*progress)(void *user, uint64_t done, uint64_t total), void *user) {
  zip = zip_open(rom);
  if (!zip) {
    fprintf(stderr, "[rom] %s is not a zip\n", rom);
    return NULL;
  }
  if (zip_compressed(zip)) uncompress_once(rom, progress, user);
  if (!zip) return NULL;
  snprintf(prefix, sizeof prefix, "%s%s", root, root[0] ? "/" : "");
  printf("[rom] %s: %d files\n", rom, zip_count(zip));
  return &zip_kind;
}
