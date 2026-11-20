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
#include "core/config_loader.h"
#include "url.h"

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

UrlKind url_resolve(const char *game_dir, const char *url, char *path, size_t size) {
  UrlKind kind = url_resolve_game(game_dir, url, path, size);
  if (kind == URL_FILE) mods_apply(path, size);
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

uint8_t *url_read_file_transformed(const char *path, size_t *len) {
  uint8_t *data = url_read_file(path, len);
  UrlTransform fn = data ? transform_of(path) : NULL;
  if (fn && fn(data, len) != 0) {
    free(data);
    return NULL;
  }
  return data;
}

uint8_t *url_read_head_transformed(const char *path, size_t n, size_t *len) {
  FILE *f = fopen(path, "rb");
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
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
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
