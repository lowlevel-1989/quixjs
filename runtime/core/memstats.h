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

#pragma once

#include <stddef.h>

// Cuenta de la memoria grande de quixjs fuera del motor de JS, para el
// comando mem de la consola de debug: los pixeles de los canvas e imagenes
// (gfx/surface.c) y las texturas que WebGL sube a la GPU (en las portatiles
// la GPU usa la misma RAM).
typedef struct {
  size_t surface_bytes;
  long   surface_count;
  size_t texture_bytes;
  long   texture_count;
  long   decode_count;     // imagenes decodificadas (PNG, JPG...) desde el arranque
  double decode_ms;        // y el tiempo que llevo, en el thread principal
} MemStats;

extern MemStats quix_mem;

// cambia la cuenta de bytes (delta puede ser negativo) y de objetos
static inline void memstats_surface(long delta_bytes, long delta_count) {
  quix_mem.surface_bytes += (size_t)delta_bytes;
  quix_mem.surface_count += delta_count;
}
static inline void memstats_decode(double ms) {
  quix_mem.decode_count++;
  quix_mem.decode_ms += ms;
}
static inline void memstats_texture(long delta_bytes, long delta_count) {
  quix_mem.texture_bytes += (size_t)delta_bytes;
  quix_mem.texture_count += delta_count;
}
