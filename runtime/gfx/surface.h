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

// Superficie RGBA en memoria (alpha sin premultiplicar, mismo orden que
// ImageData: r, g, b, a). Todo el canvas 2d se dibuja aqui por software,
// tanto con el backend sdl como con el dummy.

#include <stdint.h>

typedef struct {
  int      w;
  int      h;
  uint8_t *px;   // w * h * 4 bytes
} Surface;

// Matriz afin con el mismo orden que el canvas (setTransform(a, b, c, d, e, f)):
//   x' = a * x + c * y + e
//   y' = b * x + d * y + f
typedef struct {
  double a, b, c, d, e, f;
} Matrix;

// Operaciones de globalCompositeOperation soportadas
typedef enum {
  OP_SOURCE_OVER = 0,
  OP_SOURCE_IN,
  OP_SOURCE_OUT,
  OP_SOURCE_ATOP,
  OP_DESTINATION_OVER,
  OP_DESTINATION_IN,
  OP_DESTINATION_OUT,
  OP_DESTINATION_ATOP,
  OP_LIGHTER,
  OP_COPY,
  OP_XOR,
  OP_MULTIPLY,
  OP_SCREEN,
  OP_OVERLAY,
  OP_DARKEN,
  OP_LIGHTEN,
  OP_DIFFERENCE,
  OP_COUNT
} CompositeOp;

int  surface_init(Surface *s, int w, int h);
int  surface_resize(Surface *s, int w, int h);   // borra el contenido
void surface_free(Surface *s);
void surface_clear(Surface *s);
int  surface_copy(Surface *dst, const Surface *src);
// La Surface se queda con px (malloc, w * h * 4, RGBA) en vez de copiarlo
void surface_adopt(Surface *s, uint8_t *px, int w, int h);

const char *composite_op_name(CompositeOp op);
int         composite_op_parse(const char *name, CompositeOp *out);

// Mezcla un color (0..255, alpha 0..255 ya multiplicado por coverage y
// globalAlpha) sobre un pixel de destino. source-over y lighter (los de casi
// todos los dibujos) van en linea y con enteros; el resto en blend_pixel_slow.
void blend_pixel_slow(uint8_t *dst, int r, int g, int b, int a, CompositeOp op);

static inline void blend_pixel(uint8_t *dst, int r, int g, int b, int a, CompositeOp op) {
  if (op == OP_SOURCE_OVER) {
    if (a <= 0) return;
    if (a >= 255 || dst[3] == 0) {
      dst[0] = (uint8_t)r; dst[1] = (uint8_t)g; dst[2] = (uint8_t)b; dst[3] = (uint8_t)a;
      return;
    }
    int da = dst[3];
    int oa = a * 255 + da * (255 - a);           // alpha * 255
    int k = da * (255 - a) / 255;
    dst[0] = (uint8_t)((r * a + dst[0] * k) * 255 / oa);
    dst[1] = (uint8_t)((g * a + dst[1] * k) * 255 / oa);
    dst[2] = (uint8_t)((b * a + dst[2] * k) * 255 / oa);
    dst[3] = (uint8_t)((oa + 127) / 255);
    return;
  }
  if (op == OP_LIGHTER) {
    // suma de colores premultiplicados (con tope)
    if (a <= 0) return;
    int da = dst[3];
    if (da == 255) {
      // destino opaco (una mascara de luces): suma con tope, sin divisiones
      int ka = a + (a >> 7);   // a/255 como a/256 aproximado
      int v0 = dst[0] + ((r * ka + 128) >> 8), v1 = dst[1] + ((g * ka + 128) >> 8), v2 = dst[2] + ((b * ka + 128) >> 8);
      dst[0] = (uint8_t)(v0 > 255 ? 255 : v0);
      dst[1] = (uint8_t)(v1 > 255 ? 255 : v1);
      dst[2] = (uint8_t)(v2 > 255 ? 255 : v2);
      return;
    }
    int oa = a + da;
    if (oa > 255) oa = 255;
    int c[3] = { r, g, b };
    for (int i = 0; i < 3; i++) {
      int pc = (c[i] * a + dst[i] * da + 127) / 255;   // premultiplicado
      if (pc > 255) pc = 255;
      int v = pc * 255 / oa;
      dst[i] = (uint8_t)(v > 255 ? 255 : v);
    }
    dst[3] = (uint8_t)oa;
    return;
  }
  blend_pixel_slow(dst, r, g, b, a, op);
}

// Las operaciones "globales" (source-in, copy, destination-in...) tambien
// afectan a los pixeles de destino que la fuente no cubre. Devuelve 1 si op
// es una de ellas.
int composite_op_is_global(CompositeOp op);

void matrix_identity(Matrix *m);
void matrix_multiply(Matrix *out, const Matrix *m, const Matrix *n);  // out = m * n
int  matrix_invert(Matrix *out, const Matrix *m);
void matrix_apply(const Matrix *m, double x, double y, double *ox, double *oy);
