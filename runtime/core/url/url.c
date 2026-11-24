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

// runtime/core/url/url.c: las rutas de los recursos del juego (ver url.h):
// una url del juego a una ruta del disco (img/a.png?v=1, file://,
// http://localhost/), los mods que la cambian y las data URIs. Leer los
// archivos esta en read.c; stat, listar y mapear, en fs.c.


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
  // el archivo de un mod tambien pasa el sandbox: un enlace simbolico dentro
  // de mods/ que apunte fuera del port, o una ruta con ".." que salga de
  // mods/<mod>/, no vale
  mods_apply(path, size);
  if (!sandbox_check(path, "mod file")) return URL_INVALID;
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
