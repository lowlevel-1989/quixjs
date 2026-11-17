/*
 * QuixJS - A lightweight JS runtime using SDL2 and JavaScriptCore
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

// El color de cada pixel de un gradiente o un patron (fillStyle /
// strokeStyle). Ver paint.h.

#include <math.h>
#include "paint.h"

void gradient_add_stop(Gradient *g, double offset, Color c) {
  if (g->stop_count >= GRADIENT_MAX_STOPS) return;
  if (offset < 0) offset = 0;
  if (offset > 1) offset = 1;

  // Se mantienen ordenados por offset; con el mismo offset, en orden de llegada
  int i = g->stop_count;
  while (i > 0 && g->stops[i - 1].offset > offset) {
    g->stops[i] = g->stops[i - 1];
    i--;
  }
  g->stops[i].offset = offset;
  g->stops[i].color = c;
  g->stop_count++;
}

static void gradient_color_at(const Gradient *g, double t, Color *out) {
  if (g->stop_count == 0) {
    out->r = out->g = out->b = out->a = 0;
    return;
  }
  if (t <= g->stops[0].offset) {
    *out = g->stops[0].color;
    return;
  }
  const GradientStop *last = &g->stops[g->stop_count - 1];
  if (t >= last->offset) {
    *out = last->color;
    return;
  }
  for (int i = 1; i < g->stop_count; i++) {
    const GradientStop *s0 = &g->stops[i - 1];
    const GradientStop *s1 = &g->stops[i];
    if (t <= s1->offset) {
      double span = s1->offset - s0->offset;
      double k = span > 0 ? (t - s0->offset) / span : 1.0;
      out->r = (unsigned char)(s0->color.r + (s1->color.r - s0->color.r) * k + 0.5);
      out->g = (unsigned char)(s0->color.g + (s1->color.g - s0->color.g) * k + 0.5);
      out->b = (unsigned char)(s0->color.b + (s1->color.b - s0->color.b) * k + 0.5);
      out->a = (unsigned char)(s0->color.a + (s1->color.a - s0->color.a) * k + 0.5);
      return;
    }
  }
  *out = last->color;
}

static int gradient_sample(const PaintSampler *ps, const Gradient *g, double x, double y, Color *out) {
  double t;
  if (g->type == GRADIENT_LINEAR) {
    double dx = g->x1 - g->x0;
    double dy = g->y1 - g->y0;
    double len2 = dx * dx + dy * dy;
    if (len2 == 0) return 0;  // gradiente degenerado: no pinta
    t = ((x - g->x0) * dx + (y - g->y0) * dy) / len2;
  } else if (g->x0 == g->x1 && g->y0 == g->y1 && g->r1 > g->r0) {
    // Radial concentrico (luces, sombras...): t por la distancia al centro
    double dx = x - g->x0, dy = y - g->y0;
    t = (sqrt(dx * dx + dy * dy) - g->r0) / (g->r1 - g->r0);
  } else {
    // Radial general: se resuelve la ecuacion del spec quedandose con el t mayor.
    double cdx = g->x1 - g->x0;
    double cdy = g->y1 - g->y0;
    double dr  = g->r1 - g->r0;
    double px  = x - g->x0;
    double py  = y - g->y0;
    double a = cdx * cdx + cdy * cdy - dr * dr;
    double b = px * cdx + py * cdy + g->r0 * dr;
    double c = px * px + py * py - g->r0 * g->r0;
    if (fabs(a) < 1e-9) {
      if (fabs(b) < 1e-9) return 0;
      t = c / (2 * b);
    } else {
      double disc = b * b - a * c;
      if (disc < 0) return 0;
      double sq = sqrt(disc);
      t = (b + sq) / a;
      if (g->r0 + t * dr < 0) {
        t = (b - sq) / a;
        if (g->r0 + t * dr < 0) return 0;
      }
    }
  }
  if (ps->has_lut) {
    int i = t <= 0 ? 0 : (t >= 1 ? GRADIENT_LUT - 1 : (int)(t * (GRADIENT_LUT - 1) + 0.5));
    *out = ps->lut[i];
  } else {
    gradient_color_at(g, t, out);
  }
  return 1;
}

static int wrap(int v, int size) {
  int m = v % size;
  return m < 0 ? m + size : m;
}

static int pattern_sample(const Pattern *p, double x, double y, Color *out) {
  const Surface *img = &p->image;
  if (img->w <= 0 || img->h <= 0) return 0;

  double px, py;
  Matrix inv;
  if (matrix_invert(&inv, &p->transform) != 0) return 0;
  matrix_apply(&inv, x, y, &px, &py);

  int ix = (int)floor(px);
  int iy = (int)floor(py);
  int rx = p->repeat == PATTERN_REPEAT || p->repeat == PATTERN_REPEAT_X;
  int ry = p->repeat == PATTERN_REPEAT || p->repeat == PATTERN_REPEAT_Y;

  if (rx) ix = wrap(ix, img->w);
  else if (ix < 0 || ix >= img->w) return 0;
  if (ry) iy = wrap(iy, img->h);
  else if (iy < 0 || iy >= img->h) return 0;

  const uint8_t *s = img->px + ((size_t)iy * img->w + ix) * 4;
  out->r = s[0];
  out->g = s[1];
  out->b = s[2];
  out->a = s[3];
  return 1;
}

void paint_sampler_init(PaintSampler *ps, const Paint *paint, const Matrix *ctm) {
  ps->paint = paint;
  ps->invertible = matrix_invert(&ps->inverse, ctm) == 0;
  ps->has_lut = 0;
  if (paint->type == PAINT_GRADIENT && paint->gradient) {
    for (int i = 0; i < GRADIENT_LUT; i++) {
      gradient_color_at(paint->gradient, (double)i / (GRADIENT_LUT - 1), &ps->lut[i]);
    }
    ps->has_lut = 1;
  }
}

int paint_sample(const PaintSampler *ps, int x, int y, Color *out) {
  const Paint *p = ps->paint;
  if (p->type == PAINT_COLOR) {
    *out = p->color;
    return 1;
  }
  if (!ps->invertible) return 0;

  double ux, uy;
  matrix_apply(&ps->inverse, x + 0.5, y + 0.5, &ux, &uy);

  if (p->type == PAINT_GRADIENT && p->gradient) return gradient_sample(ps, p->gradient, ux, uy, out);
  if (p->type == PAINT_PATTERN && p->pattern) return pattern_sample(p->pattern, ux, uy, out);
  return 0;
}
