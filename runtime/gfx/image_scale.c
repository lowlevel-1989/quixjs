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

// Imagenes grandes a menos resolucion. Ver image_scale.h.

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "core/url.h"
#include "image_loader.h"
#include "image_scale.h"

static double scale;                         // 0: apagado
static int    min_side;
static char   dirs[IMAGE_SCALE_MAX_DIRS][128];
static int    dirs_count;
static char   cache[1024];                   // <cache_dir>/images-<porcentaje> ("" sin cache)

void image_scale_init(const char *cache_dir, double s, int min, const char d[][128], int nd) {
  scale = s > 0 && s < 1 ? s : 0;
  min_side = min > 0 ? min : 0;
  dirs_count = 0;
  for (int i = 0; i < nd && i < IMAGE_SCALE_MAX_DIRS; i++) {
    // sin "/" al final: "img/pictures" vale para "img/pictures/a.png"
    snprintf(dirs[dirs_count], sizeof dirs[0], "%s", d[i]);
    size_t n = strlen(dirs[dirs_count]);
    while (n > 0 && dirs[dirs_count][n - 1] == '/') dirs[dirs_count][--n] = 0;
    if (n > 0) dirs_count++;
  }
  cache[0] = 0;
  if (scale && cache_dir && cache_dir[0])
    snprintf(cache, sizeof cache, "%s/images-%d", cache_dir, (int)(scale * 100 + 0.5));
  if (scale)
    printf("[image-scale] %d%% (images of %d px or more in %d folder(s)), cache: %s\n",
           (int)(scale * 100 + 0.5), min_side, dirs_count, cache[0] ? cache : "none");
}

int image_scale_wanted(const char *src) {
  if (!scale || !src) return 0;
  if (src[0] == '.' && src[1] == '/') src += 2;
  for (int i = 0; i < dirs_count; i++) {
    size_t n = strlen(dirs[i]);
    if (strncmp(src, dirs[i], n) == 0 && src[n] == '/') return 1;
  }
  return 0;
}

// El tamano de un PNG por su cabecera (IHDR: ancho y alto en los bytes 16 a
// 23), ya descifrada. 0 si es un PNG
static int png_size(const char *path, int *w, int *h) {
  size_t len = 0;
  uint8_t *b = url_read_head_transformed(path, 64, &len);
  if (!b) return -1;
  int ok = len >= 24 && memcmp(b, "\x89PNG\r\n\x1a\n", 8) == 0 && memcmp(b + 12, "IHDR", 4) == 0;
  if (ok) {
    *w = (int)((uint32_t)b[16] << 24 | (uint32_t)b[17] << 16 | (uint32_t)b[18] << 8 | b[19]);
    *h = (int)((uint32_t)b[20] << 24 | (uint32_t)b[21] << 16 | (uint32_t)b[22] << 8 | b[23]);
  }
  free(b);
  return ok ? 0 : -1;
}

// <cache>/<path sin "/" ni "../" delante>.png
static int cache_path(const char *path, char *out, size_t size) {
  if (!cache[0]) return -1;
  while (*path == '/' || (path[0] == '.' && path[1] == '/')) path += path[0] == '/' ? 1 : 2;
  if (strstr(path, "..")) return -1;
  int n = snprintf(out, size, "%s/%s.png", cache, path);
  return n > 0 && (size_t)n < size ? 0 : -1;
}

// La reducida esta en la cache y es mas nueva que la imagen
static int cache_fresh(const char *path, const char *cached) {
  struct stat a, b;
  if (stat(path, &a) != 0 || stat(cached, &b) != 0) return 0;
  return b.st_mtime >= a.st_mtime;
}

static void mkdirs_for(const char *file) {
  char tmp[1024];
  snprintf(tmp, sizeof tmp, "%s", file);
  for (char *p = tmp + 1; *p; p++) {
    if (*p != '/') continue;
    *p = 0;
    mkdir(tmp, 0755);
    *p = '/';
  }
}

// Reduce con la media de cada caja de pixeles; el color, pesado por su alfa
// (sin bordes oscuros alrededor de lo transparente)
static uint8_t *downscale(const uint8_t *src, int sw, int sh, int dw, int dh) {
  uint8_t *out = malloc((size_t)dw * dh * 4 + 1);
  if (!out) return NULL;
  for (int dy = 0; dy < dh; dy++) {
    int y0 = (int)((long long)dy * sh / dh);
    int y1 = (int)((long long)(dy + 1) * sh / dh);
    if (y1 <= y0) y1 = y0 + 1;
    for (int dx = 0; dx < dw; dx++) {
      int x0 = (int)((long long)dx * sw / dw);
      int x1 = (int)((long long)(dx + 1) * sw / dw);
      if (x1 <= x0) x1 = x0 + 1;
      uint64_t r = 0, g = 0, b = 0, a = 0, n = 0;
      for (int y = y0; y < y1; y++) {
        const uint8_t *p = src + ((size_t)y * sw + x0) * 4;
        for (int x = x0; x < x1; x++, p += 4) {
          r += (uint64_t)p[0] * p[3];
          g += (uint64_t)p[1] * p[3];
          b += (uint64_t)p[2] * p[3];
          a += p[3];
          n++;
        }
      }
      uint8_t *o = out + ((size_t)dy * dw + dx) * 4;
      o[0] = a ? (uint8_t)(r / a) : 0;
      o[1] = a ? (uint8_t)(g / a) : 0;
      o[2] = a ? (uint8_t)(b / a) : 0;
      o[3] = (uint8_t)(a / n);
    }
  }
  return out;
}

// Guarda la reducida: a un archivo temporal y despues con su nombre (dos
// threads pueden pedir la misma imagen)
static void cache_write(const char *cached, const uint8_t *px, int w, int h) {
  char tmp[1100];
  snprintf(tmp, sizeof tmp, "%s.%ld.%lx.tmp", cached, (long)getpid(), (unsigned long)pthread_self());
  mkdirs_for(cached);
  if (image_save_png(px, w, h, tmp) == 0 && rename(tmp, cached) == 0) return;
  unlink(tmp);
}

int image_scale_decode(const char *path, uint8_t **px, int *w, int *h, int *nat_w, int *nat_h) {
  char cached[1100];
  int have_cache = cache_path(path, cached, sizeof cached) == 0;
  int nw = 0, nh = 0;
  // desde la cache: el tamano natural sale de la cabecera de la imagen
  if (have_cache && cache_fresh(path, cached) && png_size(path, &nw, &nh) == 0 &&
      image_decode_pixels(NULL, 0, cached, px, w, h) == 0) {
    *nat_w = nw;
    *nat_h = nh;
    return 0;
  }
  // la imagen entera
  uint8_t *full = NULL;
  int fw = 0, fh = 0, r;
  if (url_has_transform(path)) {
    size_t len = 0;
    uint8_t *data = url_read_file_transformed(path, &len);
    r = data ? image_decode_pixels(data, len, NULL, &full, &fw, &fh) : -1;
    free(data);
  } else {
    r = image_decode_pixels(NULL, 0, path, &full, &fw, &fh);
  }
  if (r != 0) return -1;
  *nat_w = fw;
  *nat_h = fh;
  int dw = (int)(fw * scale + 0.5), dh = (int)(fh * scale + 0.5);
  if (!scale || (fw < min_side && fh < min_side) || dw < 1 || dh < 1) {
    *px = full;
    *w = fw;
    *h = fh;
    return 0;
  }
  uint8_t *small = downscale(full, fw, fh, dw, dh);
  if (!small) {
    *px = full;
    *w = fw;
    *h = fh;
    return 0;
  }
  free(full);
  // solo si se puede saber el tamano natural sin decodificarla (PNG)
  if (have_cache && png_size(path, &nw, &nh) == 0 && nw == fw && nh == fh) cache_write(cached, small, dw, dh);
  *px = small;
  *w = dw;
  *h = dh;
  return 0;
}
