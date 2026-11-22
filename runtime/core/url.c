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

// Las rutas de los recursos del juego, los mods y las transformaciones por
// extension. Ver url.h.

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
#include "core/config_loader.h"
#include "url.h"
#include "core/sandbox.h"
#include "rom/rom.h"

static int hexval(int c) {
  if (c >= '0' && c <= '9') return c - '0';
  c = tolower(c);
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

// %20 -> ' ' (en el mismo buffer)
static void percent_decode(char *s) {
  char *w = s;
  for (char *r = s; *r; r++) {
    if (r[0] == '%' && hexval(r[1]) >= 0 && hexval(r[2]) >= 0) {
      *w++ = (char)(hexval(r[1]) * 16 + hexval(r[2]));
      r += 2;
    } else {
      *w++ = *r;
    }
  }
  *w = 0;
}

// Los mods activos ([game] mods): si un archivo de la carpeta del juego
// ([game] game_dir) esta tambien en mods/<nombre>/ con la misma ruta, se usa el del
// mod; del ultimo de la lista al primero (gana el ultimo)
static char mods_data[MAX_PATH_LEN];
static char mods_root[MAX_PATH_LEN];
static char mods_names[MAX_MODS][64];
static int  mods_count;

void url_set_mods(const char *data_dir, const char *root, char names[][64], int count) {
  // la carpeta tal cual empiezan las rutas: sin ./ delante ni / detras
  const char *d = data_dir ? data_dir : "";
  while (d[0] == '.' && d[1] == '/') d += 2;
  snprintf(mods_data, sizeof mods_data, "%s", d);
  size_t n = strlen(mods_data);
  while (n > 1 && mods_data[n - 1] == '/') mods_data[--n] = 0;
  snprintf(mods_root, sizeof mods_root, "%s", root ? root : "mods");
  mods_count = 0;
  for (int i = 0; i < count && i < MAX_MODS; i++) snprintf(mods_names[mods_count++], 64, "%s", names[i]);
}

// path dentro de la carpeta del juego y en algun mod: la ruta del mod
static void mods_apply(char *path, size_t size) {
  if (!mods_count || !mods_data[0]) return;
  const char *p = path;
  while (p[0] == '.' && p[1] == '/') p += 2;
  size_t n = strlen(mods_data);
  if (strncmp(p, mods_data, n) != 0 || p[n] != '/') return;
  const char *rel = p + n + 1;
  char cand[MAX_PATH_LEN * 2];
  for (int i = mods_count - 1; i >= 0; i--) {
    snprintf(cand, sizeof cand, "%s/%s/%s", mods_root, mods_names[i], rel);
    if (access(cand, R_OK) == 0) {
      snprintf(path, size, "%s", cand);
      return;
    }
  }
}

UrlKind url_resolve_game(const char *game_dir, const char *url, char *path, size_t size) {
  if (!url || !*url) return URL_INVALID;
  if (strncmp(url, "data:", 5) == 0) return URL_DATA;
  if (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) {
    // http://localhost/... (window.location) se trata como ruta del juego
    const char *local = NULL;
    if (strncmp(url, "http://localhost/", 17) == 0) local = url + 17;
    if (!local) return URL_REMOTE;
    url = local;
  }

  char tmp[1024];
  int absolute = 0;
  if (strncmp(url, "file://", 7) == 0) {
    url += 7;
    absolute = 1;
  }
  snprintf(tmp, sizeof(tmp), "%s", url);

  // sin query ni fragmento
  char *q = strpbrk(tmp, "?#");
  if (q) *q = 0;
  percent_decode(tmp);

  const char *rel = tmp;
  while (rel[0] == '.' && rel[1] == '/') rel += 2;

  if (absolute || rel[0] == '/') {
    snprintf(path, size, "%s", rel);
  } else if (game_dir && *game_dir) {
    snprintf(path, size, "%s/%s", game_dir, rel);
  } else {
    snprintf(path, size, "%s", rel);
  }
  return URL_FILE;
}

// Lo que pide el juego (scripts, imagenes, sonido, XHR, fetch, fuentes):
// solo dentro de la carpeta del port (runtime/core/sandbox.c)
UrlKind url_resolve(const char *game_dir, const char *url, char *path, size_t size) {
  UrlKind kind = url_resolve_game(game_dir, url, path, size);
  if (kind != URL_FILE) return kind;
  if (!sandbox_check(path, "file")) return URL_INVALID;
  mods_apply(path, size);
  return kind;
}

static int b64val(int c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+' || c == '-') return 62;
  if (c == '/' || c == '_') return 63;
  return -1;
}

uint8_t *url_decode_data(const char *url, size_t *len) {
  const char *comma = strchr(url, ',');
  if (!comma) return NULL;
  int base64 = 0;
  for (const char *p = url; p < comma; p++) {
    if (strncmp(p, ";base64", 7) == 0) base64 = 1;
  }
  const char *data = comma + 1;
  size_t n = strlen(data);
  uint8_t *out = malloc(n + 1);
  if (!out) return NULL;

  size_t o = 0;
  if (base64) {
    unsigned int acc = 0;
    int bits = 0;
    for (size_t i = 0; i < n; i++) {
      int v = b64val((unsigned char)data[i]);
      if (v < 0) continue;  // '=', espacios, saltos de linea
      acc = (acc << 6) | v;
      bits += 6;
      if (bits >= 8) {
        bits -= 8;
        out[o++] = (uint8_t)((acc >> bits) & 0xff);
      }
    }
  } else {
    memcpy(out, data, n + 1);
    percent_decode((char *)out);
    o = strlen((char *)out);
  }
  out[o] = 0;
  *len = o;
  return out;
}

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
    } else if (*w++ != *h++) {
      return 0;
    }
  }
  return !*w && !*h;
}

FILE *url_fopen_loose(const char *path) {
  FILE *f = fopen(path, "rb");
  if (f) return f;
  const char *slash = strrchr(path, '/');
  const char *name = slash ? slash + 1 : path;
  const char *p = name;
  while (*p && (unsigned char)*p < 0x80) p++;
  if (!*p) return NULL;        // nombre ASCII: no esta
  char dir[1024];
  snprintf(dir, sizeof(dir), "%.*s", slash ? (int)(slash - path) : 1, slash ? path : ".");
  DIR *d = opendir(dir);
  if (!d) return NULL;
  struct dirent *e;
  while (!f && (e = readdir(d)) != NULL) {
    if (!loose_match(name, e->d_name)) continue;
    char found[2048];
    snprintf(found, sizeof(found), "%s/%s", dir, e->d_name);
    f = fopen(found, "rb");
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

// ---- en disco o en la rom

int url_stat(const char *path, long long *size, long long *mtime, int *is_dir) {
  struct stat st;
  if (stat(path, &st) == 0) {
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
  DIR *d = opendir(dir);
  if (d) {
    found = 1;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
      if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
      char full[2048];
      snprintf(full, sizeof full, "%s/%s", dir, e->d_name);
      struct stat st;
      list_add(&c, e->d_name, stat(full, &st) == 0 && S_ISDIR(st.st_mode));
    }
    closedir(d);
  }
  if (rom_list(dir, list_add, &c) >= 0) found = 1;
  free(c.names);
  return found ? c.count : -1;
}

void *url_map(const char *path, size_t *len, size_t *delta) {
  int fd = open(path, O_RDONLY | O_CLOEXEC);
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
