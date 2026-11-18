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

// Decodificar imagenes con SDL_image y guardar capturas en PNG. Ver
// image_loader.h.

#include <time.h>
#include "core/memstats.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdlib.h>
#include <string.h>
#include "image_loader.h"

static int surface_from_sdl(SDL_Surface *img, Surface *out) {
  if (!img) return -1;
  SDL_Surface *rgba = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(img);
  if (!rgba) return -1;

  if (surface_init(out, rgba->w, rgba->h) != 0) {
    SDL_FreeSurface(rgba);
    return -1;
  }
  SDL_LockSurface(rgba);
  for (int y = 0; y < rgba->h; y++) {
    memcpy(out->px + (size_t)y * rgba->w * 4, (uint8_t *)rgba->pixels + (size_t)y * rgba->pitch, (size_t)rgba->w * 4);
  }
  SDL_UnlockSurface(rgba);
  SDL_FreeSurface(rgba);
  return 0;
}

// A pixeles sueltos (malloc), sin Surface ni estadisticas: vale desde otro thread
static int pixels_from_sdl(SDL_Surface *img, uint8_t **px, int *w, int *h) {
  if (!img) return -1;
  SDL_Surface *rgba = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(img);
  if (!rgba) return -1;
  uint8_t *out = malloc((size_t)rgba->w * rgba->h * 4 + 1);
  if (!out) {
    SDL_FreeSurface(rgba);
    return -1;
  }
  SDL_LockSurface(rgba);
  for (int y = 0; y < rgba->h; y++) {
    memcpy(out + (size_t)y * rgba->w * 4, (uint8_t *)rgba->pixels + (size_t)y * rgba->pitch, (size_t)rgba->w * 4);
  }
  SDL_UnlockSurface(rgba);
  *px = out;
  *w = rgba->w;
  *h = rgba->h;
  SDL_FreeSurface(rgba);
  return 0;
}

int image_decode_pixels(const uint8_t *data, size_t len, const char *path, uint8_t **px, int *w, int *h) {
  if (path) return pixels_from_sdl(IMG_Load(path), px, w, h);
  SDL_RWops *rw = SDL_RWFromConstMem(data, (int)len);
  if (!rw) return -1;
  return pixels_from_sdl(IMG_Load_RW(rw, 1), px, w, h);
}

static double now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int image_load_file(const char *path, Surface *out) {
  double t = now_ms();
  int r = surface_from_sdl(IMG_Load(path), out);
  memstats_decode(now_ms() - t);
  return r;
}

int image_load_memory(const uint8_t *data, size_t len, Surface *out) {
  SDL_RWops *rw = SDL_RWFromConstMem(data, (int)len);
  if (!rw) return -1;
  double t = now_ms();
  int r = surface_from_sdl(IMG_Load_RW(rw, 1), out);
  memstats_decode(now_ms() - t);
  return r;
}

int image_save_png(const uint8_t *rgba, int w, int h, const char *path) {
  SDL_Surface *s = SDL_CreateRGBSurfaceWithFormatFrom((void *)rgba, w, h, 32, w * 4, SDL_PIXELFORMAT_RGBA32);
  if (!s) return -1;
  int r = IMG_SavePNG(s, path);
  SDL_FreeSurface(s);
  return r;
}
