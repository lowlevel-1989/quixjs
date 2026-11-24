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

// runtime/core/url/read.c: leer los archivos del juego: enteros o solo el
// principio, con la transformacion de su extension (los cifrados de RPG
// Maker), y abrirlos aunque el nombre no ASCII no coincida en una SD FAT.


#include <errno.h>
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

// ---------- transformaciones por extension ----------

#define MAX_TRANSFORMS 8
static struct { char ext[16]; UrlTransform fn; } transforms[MAX_TRANSFORMS];
static int transform_count = 0;

void url_add_transform(const char *ext, UrlTransform fn) {
  for (int i = 0; i < transform_count; i++) {
    if (strcmp(transforms[i].ext, ext) == 0) {
      transforms[i].fn = fn;
      return;
    }
  }
  if (transform_count == MAX_TRANSFORMS) return;
  snprintf(transforms[transform_count].ext, sizeof transforms[0].ext, "%s", ext);
  transforms[transform_count++].fn = fn;
}

static UrlTransform transform_of(const char *path) {
  const char *dot = strrchr(path, '.');
  if (!dot) return NULL;
  for (int i = 0; i < transform_count; i++) {
    if (strcasecmp(transforms[i].ext, dot) == 0) return transforms[i].fn;
  }
  return NULL;
}

int url_has_transform(const char *path) { return transform_of(path) != NULL; }

// ---- nombres no ASCII en una SD FAT
// En una tarjeta FAT montada con iocharset=ascii (la de los juegos de la
// RGB30 con ROCKNIX) un nombre como "e_\xd0\xa1ursed.png" (una C cirilica)
// no se puede guardar: se ve como "e_?ursed.png", o el usuario lo renombra
// a "e_Cursed.png". Si el archivo no esta, se busca en su carpeta uno cuyo
// nombre coincida tratando cada caracter no ASCII como "cualquier caracter".
// Y sin distinguir mayusculas, como en Windows, donde se hacen los juegos
// (Muffles' Life Sentence pide Leaf.rpgmvp y trae leaf.rpgmvp).

// un caracter UTF-8 que empieza en s: cuantos bytes ocupa
static int utf8_len(const unsigned char *s) {
  if (*s < 0x80) return 1;
  int n = (*s >= 0xf0) ? 4 : (*s >= 0xe0) ? 3 : 2;
  for (int i = 1; i < n; i++)
    if ((s[i] & 0xc0) != 0x80) return 1;
  return n;
}

static int loose_match(const char *want, const char *have) {
  const unsigned char *w = (const unsigned char *)want, *h = (const unsigned char *)have;
  while (*w && *h) {
    if (*w >= 0x80) {          // cualquier caracter (uno) en su lugar
      w += utf8_len(w);
      h += utf8_len(h);
    } else if (tolower(*w++) != tolower(*h++)) {
      return 0;
    }
  }
  return !*w && !*h;
}

FILE *url_fopen_loose(const char *path) {
  FILE *f = sandbox_fopen(path, "rb", "file");
  if (f || errno == EACCES) return f;
  const char *slash = strrchr(path, '/');
  const char *name = slash ? slash + 1 : path;
  char dir[1024];
  snprintf(dir, sizeof(dir), "%.*s", slash ? (int)(slash - path) : 1, slash ? path : ".");
  DIR *d = sandbox_opendir(dir, "file");
  if (!d) return NULL;
  struct dirent *e;
  while (!f && (e = readdir(d)) != NULL) {
    if (!loose_match(name, e->d_name)) continue;
    char found[2048];
    snprintf(found, sizeof(found), "%s/%s", dir, e->d_name);
    f = sandbox_fopen(found, "rb", "file");
  }
  closedir(d);
  return f;
}

uint8_t *url_read_file_transformed(const char *path, size_t *len) {
  uint8_t *data = url_read_file(path, len);
  UrlTransform fn = data ? transform_of(path) : NULL;
  if (fn && fn(data, len) != 0) {
    free(data);
    return NULL;
  }
  return data;
}

// en disco o, si no, en la rom ([game] rom: el .zip del juego)
static FILE *open_any(const char *path) {
  FILE *f = url_fopen_loose(path);
  if (f) return f;
  return rom_fopen(path);
}

FILE *url_fopen(const char *path) { return open_any(path); }

uint8_t *url_read_head_transformed(const char *path, size_t n, size_t *len) {
  FILE *f = open_any(path);
  if (!f) return NULL;
  uint8_t *buf = malloc(n + 1);   // +1: las transformaciones terminan con un 0
  size_t got = buf ? fread(buf, 1, n, f) : 0;
  fclose(f);
  if (!buf) return NULL;
  *len = got;
  UrlTransform fn = transform_of(path);
  if (fn && fn(buf, len) != 0) {
    free(buf);
    return NULL;
  }
  return buf;
}

uint8_t *url_read_file(const char *path, size_t *len) {
  FILE *f = url_fopen_loose(path);
  if (!f) {
    // de la rom, entero de una vez
    return rom_read(path, len);
  }
  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return NULL;
  }
  long size = ftell(f);
  if (size < 0) {
    fclose(f);
    return NULL;
  }
  fseek(f, 0, SEEK_SET);
  uint8_t *buf = malloc((size_t)size + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }
  size_t got = fread(buf, 1, (size_t)size, f);
  fclose(f);
  buf[got] = 0;
  *len = got;
  return buf;
}
