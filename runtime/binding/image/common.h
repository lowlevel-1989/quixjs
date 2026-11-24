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

// runtime/binding/image/common.h: lo que comparten las partes de Image, y
// nadie mas (el resto de quixjs usa image.h):
//   image.c   el objeto Image de JS y lo que pide el canvas
//   decode.c  decodificar, en segundo plano o al momento
#pragma once

#include <time.h>
#include "js/js.h"
#include "image.h"

typedef struct ImageData {
  Surface surface;
  int     loaded;
  int     fired;     // ya se envio el evento load (complete = true)
  int     width;     // width/height asignados a mano (-1 = los de la imagen)
  int     height;
  char   *src;
  unsigned gen;      // cambia con cada src: un resultado de un src anterior se descarta
  int     nat_w;     // el tamano de la imagen (sigue igual si se sueltan sus pixeles)
  int     nat_h;
  int     used_2d;   // se dibujo en un canvas 2d: sus pixeles se quedan
  int     uploaded;  // ya esta en una textura de WebGL
  int     dropped;   // pixeles soltados (se decodifica otra vez si hace falta)
  uint8_t *enc;      // la imagen sin decodificar si vino de un Blob (el Blob se puede revocar)
  size_t  enc_len;
  struct ImageData *prev, *next;   // todas las imagenes vivas
} ImageData;

// Una imagen que ya esta en una textura de WebGL y nunca se dibujo en 2d tiene
// sus pixeles dos veces (en las portatiles la GPU usa la misma RAM). Cuando al
// sistema le falta RAM, js_image_release_uploaded suelta los de la CPU de las
// de este tamano o mas; si vuelven a hacer falta, se decodifican otra vez (eso
// cuesta: por eso solo con poca RAM, no siempre)
#define IMAGE_DROP_MIN_BYTES (128 * 1024)

// la clase de JS de Image (image.c)
extern JSClassID js_image_class_id;

static inline double image_now_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

// decode.c: la imagen de src al momento (0 si va), o en segundo plano (0 si
// se pidio y llegara load o error; 1 si no es de archivo ni de Blob y se
// hace al momento; -1 si no existe)
int image_decode(JSContext *ctx, ImageData *img, const char *src);
int image_decode_async(JSContext *ctx, JSValueConst obj, ImageData *img, const char *src);
