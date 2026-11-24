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

// La Surface RGBA y los modos de composicion (globalCompositeOperation) al
// mezclar un pixel. Ver surface.h.

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "surface.h"
#include "core/debug/memstats.h"

int surface_init(Surface *s, int w, int h) {
  s->w = 0;
  s->h = 0;
  s->px = NULL;
  return surface_resize(s, w, h);
}

void surface_adopt(Surface *s, uint8_t *px, int w, int h) {
  size_t size = px ? (size_t)w * (size_t)h * 4 : 0;
  size_t old = s->px ? (size_t)s->w * (size_t)s->h * 4 : 0;
  memstats_surface((long)size - (long)old, (px ? 1 : 0) - (s->px ? 1 : 0));
  free(s->px);
  s->px = px;
  s->w = px ? w : 0;
  s->h = px ? h : 0;
}

int surface_resize(Surface *s, int w, int h) {
  if (w < 0) w = 0;
  if (h < 0) h = 0;

  size_t size = (size_t)w * (size_t)h * 4;
  uint8_t *px = NULL;
  if (size > 0) {
    px = calloc(1, size);
    if (!px) return -1;
  }

  size_t old = s->px ? (size_t)s->w * (size_t)s->h * 4 : 0;
  memstats_surface((long)size - (long)old, (px ? 1 : 0) - (s->px ? 1 : 0));
  free(s->px);
  s->px = px;
  s->w = w;
  s->h = h;
  return 0;
}

void surface_free(Surface *s) {
  if (s->px) memstats_surface(-(long)((size_t)s->w * (size_t)s->h * 4), -1);
  free(s->px);
  s->px = NULL;
  s->w = 0;
  s->h = 0;
}

void surface_clear(Surface *s) {
  if (s->px) memset(s->px, 0, (size_t)s->w * s->h * 4);
}

int surface_copy(Surface *dst, const Surface *src) {
  if (surface_init(dst, src->w, src->h) != 0) return -1;
  if (src->px) memcpy(dst->px, src->px, (size_t)src->w * src->h * 4);
  return 0;
}

static const char *op_names[OP_COUNT] = {
  "source-over",
  "source-in",
  "source-out",
  "source-atop",
  "destination-over",
  "destination-in",
  "destination-out",
  "destination-atop",
  "lighter",
  "copy",
  "xor",
  "multiply",
  "screen",
  "overlay",
  "darken",
  "lighten",
  "difference",
};

const char *composite_op_name(CompositeOp op) {
  if (op < 0 || op >= OP_COUNT) return op_names[0];
  return op_names[op];
}

int composite_op_parse(const char *name, CompositeOp *out) {
  for (int i = 0; i < OP_COUNT; i++) {
    if (strcmp(name, op_names[i]) == 0) {
      *out = (CompositeOp)i;
      return 0;
    }
  }
  return -1;
}

int composite_op_is_global(CompositeOp op) {
  return op == OP_SOURCE_IN || op == OP_SOURCE_OUT || op == OP_DESTINATION_IN ||
         op == OP_DESTINATION_ATOP || op == OP_COPY;
}

static double blend_channel(CompositeOp op, double cb, double cs) {
  switch (op) {
    case OP_MULTIPLY:   return cb * cs;
    case OP_SCREEN:     return cb + cs - cb * cs;
    case OP_OVERLAY:
      return cb <= 0.5 ? 2.0 * cs * cb : 1.0 - 2.0 * (1.0 - cs) * (1.0 - cb);
    case OP_DARKEN:     return cb < cs ? cb : cs;
    case OP_LIGHTEN:    return cb > cs ? cb : cs;
    case OP_DIFFERENCE: return fabs(cb - cs);
    default:            return cs;
  }
}

static inline uint8_t to_u8(double v) {
  if (v <= 0.0) return 0;
  if (v >= 1.0) return 255;
  return (uint8_t)(v * 255.0 + 0.5);
}

void blend_pixel_slow(uint8_t *dst, int r, int g, int b, int a, CompositeOp op) {
  double as = a / 255.0;
  double ab = dst[3] / 255.0;
  double cs[3] = { r / 255.0, g / 255.0, b / 255.0 };
  double cb[3] = { dst[0] / 255.0, dst[1] / 255.0, dst[2] / 255.0 };
  double co[3];
  double ao;

  if (op >= OP_MULTIPLY) {
    ao = as + ab * (1.0 - as);
    for (int i = 0; i < 3; i++) {
      co[i] = cs[i] * as * (1.0 - ab) + cb[i] * ab * (1.0 - as) +
              as * ab * blend_channel(op, cb[i], cs[i]);
    }
  } else {
    double fa, fb;
    switch (op) {
      case OP_SOURCE_IN:        fa = ab;       fb = 0.0;      break;
      case OP_SOURCE_OUT:       fa = 1.0 - ab; fb = 0.0;      break;
      case OP_SOURCE_ATOP:      fa = ab;       fb = 1.0 - as; break;
      case OP_DESTINATION_OVER: fa = 1.0 - ab; fb = 1.0;      break;
      case OP_DESTINATION_IN:   fa = 0.0;      fb = as;       break;
      case OP_DESTINATION_OUT:  fa = 0.0;      fb = 1.0 - as; break;
      case OP_DESTINATION_ATOP: fa = 1.0 - ab; fb = as;       break;
      case OP_LIGHTER:          fa = 1.0;      fb = 1.0;      break;
      case OP_COPY:             fa = 1.0;      fb = 0.0;      break;
      case OP_XOR:              fa = 1.0 - ab; fb = 1.0 - as; break;
      default:                  fa = 1.0;      fb = 1.0 - as; break;
    }
    ao = fa * as + fb * ab;
    for (int i = 0; i < 3; i++) {
      co[i] = fa * cs[i] * as + fb * cb[i] * ab;
    }
  }

  if (ao > 1.0) ao = 1.0;
  if (ao <= 0.0) {
    dst[0] = dst[1] = dst[2] = dst[3] = 0;
    return;
  }
  dst[0] = to_u8(co[0] / ao);
  dst[1] = to_u8(co[1] / ao);
  dst[2] = to_u8(co[2] / ao);
  dst[3] = to_u8(ao);
}

void matrix_identity(Matrix *m) {
  m->a = 1; m->b = 0;
  m->c = 0; m->d = 1;
  m->e = 0; m->f = 0;
}

void matrix_multiply(Matrix *out, const Matrix *m, const Matrix *n) {
  Matrix r;
  r.a = m->a * n->a + m->c * n->b;
  r.b = m->b * n->a + m->d * n->b;
  r.c = m->a * n->c + m->c * n->d;
  r.d = m->b * n->c + m->d * n->d;
  r.e = m->a * n->e + m->c * n->f + m->e;
  r.f = m->b * n->e + m->d * n->f + m->f;
  *out = r;
}

int matrix_invert(Matrix *out, const Matrix *m) {
  double det = m->a * m->d - m->b * m->c;
  if (det == 0.0 || !isfinite(det)) return -1;
  Matrix r;
  r.a =  m->d / det;
  r.b = -m->b / det;
  r.c = -m->c / det;
  r.d =  m->a / det;
  r.e = (m->c * m->f - m->d * m->e) / det;
  r.f = (m->b * m->e - m->a * m->f) / det;
  *out = r;
  return 0;
}

void matrix_apply(const Matrix *m, double x, double y, double *ox, double *oy) {
  *ox = m->a * x + m->c * y + m->e;
  *oy = m->b * x + m->d * y + m->f;
}
