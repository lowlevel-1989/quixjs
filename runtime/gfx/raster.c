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

// Rasterizado por software: paths, mascaras de recorte y relleno sobre una
// Surface. Ver raster.h.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "raster.h"

// ------------------------------------------------------------------ path

void path_init(Path *p) {
  p->subs = NULL;
  p->count = 0;
  p->cap = 0;
}

void path_clear(Path *p) {
  for (int i = 0; i < p->count; i++) free(p->subs[i].pts);
  p->count = 0;
}

void path_free(Path *p) {
  path_clear(p);
  free(p->subs);
  path_init(p);
}

int path_copy(Path *dst, const Path *src) {
  path_init(dst);
  for (int i = 0; i < src->count; i++) {
    const SubPath *s = &src->subs[i];
    for (int j = 0; j < s->count; j++) {
      if (j == 0) path_move_to(dst, s->pts[j].x, s->pts[j].y);
      else        path_line_to(dst, s->pts[j].x, s->pts[j].y);
    }
    if (s->closed) path_close(dst);
  }
  return 0;
}

static SubPath *path_new_sub(Path *p) {
  if (p->count == p->cap) {
    int cap = p->cap ? p->cap * 2 : 4;
    SubPath *subs = realloc(p->subs, sizeof(SubPath) * cap);
    if (!subs) return NULL;
    p->subs = subs;
    p->cap = cap;
  }
  SubPath *s = &p->subs[p->count++];
  s->pts = NULL;
  s->count = 0;
  s->cap = 0;
  s->closed = 0;
  return s;
}

static void sub_add(SubPath *s, double x, double y) {
  if (s->count == s->cap) {
    int cap = s->cap ? s->cap * 2 : 8;
    PathPoint *pts = realloc(s->pts, sizeof(PathPoint) * cap);
    if (!pts) return;
    s->pts = pts;
    s->cap = cap;
  }
  s->pts[s->count].x = x;
  s->pts[s->count].y = y;
  s->count++;
}

void path_move_to(Path *p, double x, double y) {
  if (!isfinite(x) || !isfinite(y)) return;
  // un moveTo seguido de otro moveTo reutiliza el subpath vacio
  SubPath *s = NULL;
  if (p->count > 0 && p->subs[p->count - 1].count == 1 && !p->subs[p->count - 1].closed) {
    s = &p->subs[p->count - 1];
    s->count = 0;
  } else {
    s = path_new_sub(p);
  }
  if (s) sub_add(s, x, y);
}

void path_line_to(Path *p, double x, double y) {
  if (!isfinite(x) || !isfinite(y)) return;
  if (p->count == 0) {
    path_move_to(p, x, y);
    return;
  }
  SubPath *s = &p->subs[p->count - 1];
  if (s->closed) {
    // despues de closePath se empieza un subpath en el primer punto
    PathPoint first = s->pts[0];
    s = path_new_sub(p);
    if (!s) return;
    sub_add(s, first.x, first.y);
  }
  sub_add(s, x, y);
}

void path_close(Path *p) {
  if (p->count == 0) return;
  SubPath *s = &p->subs[p->count - 1];
  if (s->count > 0) s->closed = 1;
}

int path_has_current_point(const Path *p) {
  return p->count > 0 && p->subs[p->count - 1].count > 0;
}

void path_current_point(const Path *p, double *x, double *y) {
  const SubPath *s = &p->subs[p->count - 1];
  const PathPoint *pt = s->closed ? &s->pts[0] : &s->pts[s->count - 1];
  *x = pt->x;
  *y = pt->y;
}

// ------------------------------------------------------------------ mask

int mask_init(Mask *m, int w, int h) {
  m->w = w;
  m->h = h;
  m->m = (w > 0 && h > 0) ? calloc(1, (size_t)w * h) : NULL;
  m->x0 = w;
  m->y0 = h;
  m->x1 = 0;
  m->y1 = 0;
  return (w > 0 && h > 0 && !m->m) ? -1 : 0;
}

void mask_free(Mask *m) {
  free(m->m);
  m->m = NULL;
}

int mask_copy(Mask *dst, const Mask *src) {
  if (mask_init(dst, src->w, src->h) != 0) return -1;
  if (src->m) memcpy(dst->m, src->m, (size_t)src->w * src->h);
  dst->x0 = src->x0;
  dst->y0 = src->y0;
  dst->x1 = src->x1;
  dst->y1 = src->y1;
  return 0;
}

int mask_contains(const Mask *m, int x, int y) {
  if (x < 0 || y < 0 || x >= m->w || y >= m->h) return 0;
  return m->m[(size_t)y * m->w + x] != 0;
}

void mask_intersect(Mask *m, const Mask *other) {
  if (!m->m) return;
  for (size_t i = 0, n = (size_t)m->w * m->h; i < n; i++) {
    if (!other->m || !other->m[i]) m->m[i] = 0;
  }
  if (other->x0 > m->x0) m->x0 = other->x0;
  if (other->y0 > m->y0) m->y0 = other->y0;
  if (other->x1 < m->x1) m->x1 = other->x1;
  if (other->y1 < m->y1) m->y1 = other->y1;
}

static void mask_span(Mask *m, int y, int x0, int x1) {
  if (y < 0 || y >= m->h) return;
  if (x0 < 0) x0 = 0;
  if (x1 > m->w) x1 = m->w;
  if (x0 >= x1) return;
  memset(m->m + (size_t)y * m->w + x0, 255, x1 - x0);
  if (x0 < m->x0) m->x0 = x0;
  if (x1 > m->x1) m->x1 = x1;
  if (y < m->y0) m->y0 = y;
  if (y + 1 > m->y1) m->y1 = y + 1;
}

typedef struct {
  double x0, y0, x1, y1;
  int    dir;
} Edge;

typedef struct {
  double x;
  int    dir;
} Crossing;

static int cmp_crossing(const void *a, const void *b) {
  double d = ((const Crossing *)a)->x - ((const Crossing *)b)->x;
  return d < 0 ? -1 : (d > 0 ? 1 : 0);
}

// Rellena un conjunto de poligonos (cada uno cerrado implicitamente).
static void fill_polygons(Mask *m, const PathPoint *const *polys, const int *counts, int npolys, FillRule rule) {
  int nedges = 0;
  for (int i = 0; i < npolys; i++) nedges += counts[i];
  if (nedges == 0 || !m->m) return;

  Edge *edges = malloc(sizeof(Edge) * nedges);
  Crossing *cross = malloc(sizeof(Crossing) * nedges);
  if (!edges || !cross) {
    free(edges);
    free(cross);
    return;
  }

  double miny = 1e300, maxy = -1e300;
  int ne = 0;
  for (int i = 0; i < npolys; i++) {
    const PathPoint *pts = polys[i];
    int n = counts[i];
    if (n < 2) continue;
    for (int j = 0; j < n; j++) {
      const PathPoint *a = &pts[j];
      const PathPoint *b = &pts[(j + 1) % n];
      if (a->y == b->y) continue;
      Edge *e = &edges[ne++];
      if (a->y < b->y) {
        e->x0 = a->x; e->y0 = a->y; e->x1 = b->x; e->y1 = b->y; e->dir = 1;
      } else {
        e->x0 = b->x; e->y0 = b->y; e->x1 = a->x; e->y1 = a->y; e->dir = -1;
      }
      if (e->y0 < miny) miny = e->y0;
      if (e->y1 > maxy) maxy = e->y1;
    }
  }

  int ystart = (int)floor(miny);
  int yend = (int)ceil(maxy);
  if (ystart < 0) ystart = 0;
  if (yend > m->h) yend = m->h;

  for (int y = ystart; y < yend; y++) {
    double yc = y + 0.5;
    int nc = 0;
    for (int i = 0; i < ne; i++) {
      const Edge *e = &edges[i];
      if (yc < e->y0 || yc >= e->y1) continue;
      cross[nc].x = e->x0 + (yc - e->y0) * (e->x1 - e->x0) / (e->y1 - e->y0);
      cross[nc].dir = e->dir;
      nc++;
    }
    if (nc < 2) continue;
    qsort(cross, nc, sizeof(Crossing), cmp_crossing);

    int wind = 0;
    for (int i = 0; i < nc - 1; i++) {
      wind += cross[i].dir;
      int inside = rule == FILL_NONZERO ? wind != 0 : (wind & 1);
      if (!inside) continue;
      // pixeles con el centro en [xa, xb)
      int xa = (int)ceil(cross[i].x - 0.5);
      int xb = (int)ceil(cross[i + 1].x - 0.5);
      mask_span(m, y, xa, xb);
    }
  }

  free(edges);
  free(cross);
}

void mask_fill_path(Mask *m, const Path *p, FillRule rule) {
  if (p->count == 0) return;
  const PathPoint **polys = malloc(sizeof(PathPoint *) * p->count);
  int *counts = malloc(sizeof(int) * p->count);
  if (!polys || !counts) {
    free(polys);
    free(counts);
    return;
  }
  for (int i = 0; i < p->count; i++) {
    polys[i] = p->subs[i].pts;
    counts[i] = p->subs[i].count;
  }
  fill_polygons(m, polys, counts, p->count, rule);
  free(polys);
  free(counts);
}

static void fill_poly(Mask *m, const PathPoint *pts, int n) {
  const PathPoint *polys[1] = { pts };
  int counts[1] = { n };
  fill_polygons(m, polys, counts, 1, FILL_NONZERO);
}

static void fill_circle(Mask *m, double cx, double cy, double r) {
  PathPoint pts[32];
  for (int i = 0; i < 32; i++) {
    double a = i * 2 * M_PI / 32;
    pts[i].x = cx + cos(a) * r;
    pts[i].y = cy + sin(a) * r;
  }
  fill_poly(m, pts, 32);
}

static void stroke_segment(Mask *m, PathPoint a, PathPoint b, double hw, int extend_a, int extend_b) {
  double dx = b.x - a.x;
  double dy = b.y - a.y;
  double len = sqrt(dx * dx + dy * dy);
  if (len == 0) return;
  double ux = dx / len, uy = dy / len;
  double nx = -uy * hw, ny = ux * hw;
  if (extend_a) { a.x -= ux * hw; a.y -= uy * hw; }
  if (extend_b) { b.x += ux * hw; b.y += uy * hw; }
  PathPoint q[4] = {
    { a.x + nx, a.y + ny },
    { b.x + nx, b.y + ny },
    { b.x - nx, b.y - ny },
    { a.x - nx, a.y - ny },
  };
  fill_poly(m, q, 4);
}

static void stroke_join(Mask *m, PathPoint prev, PathPoint p, PathPoint next, double hw, LineJoin join) {
  if (join == JOIN_ROUND) {
    fill_circle(m, p.x, p.y, hw);
    return;
  }
  double d0x = p.x - prev.x, d0y = p.y - prev.y;
  double d1x = next.x - p.x, d1y = next.y - p.y;
  double l0 = sqrt(d0x * d0x + d0y * d0y);
  double l1 = sqrt(d1x * d1x + d1y * d1y);
  if (l0 == 0 || l1 == 0) return;
  d0x /= l0; d0y /= l0;
  d1x /= l1; d1y /= l1;

  // bevel: triangulo entre los dos bordes exteriores (se pintan ambos lados)
  PathPoint t1[3] = {
    { p.x, p.y },
    { p.x - d0y * hw, p.y + d0x * hw },
    { p.x - d1y * hw, p.y + d1x * hw },
  };
  PathPoint t2[3] = {
    { p.x, p.y },
    { p.x + d0y * hw, p.y - d0x * hw },
    { p.x + d1y * hw, p.y - d1x * hw },
  };
  fill_poly(m, t1, 3);
  fill_poly(m, t2, 3);

  if (join == JOIN_MITER) {
    // punta del miter limitada a 10 (miterLimit por defecto)
    double cosang = d0x * d1x + d0y * d1y;
    double half = sqrt((1 + cosang) / 2);
    if (half > 0.1) {
      double bx = d1x - d0x, by = d1y - d0y;
      double bl = sqrt(bx * bx + by * by);
      if (bl > 0) {
        bx /= bl; by /= bl;
        double tip = hw / half;
        // el miter sale hacia el lado exterior del giro
        PathPoint mt[4] = {
          { p.x, p.y },
          { p.x - d0y * hw, p.y + d0x * hw },
          { p.x - bx * tip, p.y - by * tip },
          { p.x - d1y * hw, p.y + d1x * hw },
        };
        PathPoint mt2[4] = {
          { p.x, p.y },
          { p.x + d0y * hw, p.y - d0x * hw },
          { p.x - bx * tip, p.y - by * tip },
          { p.x + d1y * hw, p.y - d1x * hw },
        };
        fill_poly(m, mt, 4);
        fill_poly(m, mt2, 4);
      }
    }
  }
}

void mask_stroke_path(Mask *m, const Path *p, double width, LineCap cap, LineJoin join) {
  if (width <= 0) return;
  double hw = width / 2;

  for (int i = 0; i < p->count; i++) {
    const SubPath *s = &p->subs[i];
    int n = s->count;
    if (n < 2) continue;

    int closed = s->closed;
    int segs = closed ? n : n - 1;
    for (int j = 0; j < segs; j++) {
      PathPoint a = s->pts[j];
      PathPoint b = s->pts[(j + 1) % n];
      int ext_a = !closed && j == 0 && cap == CAP_SQUARE;
      int ext_b = !closed && j == segs - 1 && cap == CAP_SQUARE;
      stroke_segment(m, a, b, hw, ext_a, ext_b);
    }

    // uniones
    for (int j = closed ? 0 : 1; j < (closed ? n : n - 1); j++) {
      PathPoint prev = s->pts[(j - 1 + n) % n];
      PathPoint cur = s->pts[j];
      PathPoint next = s->pts[(j + 1) % n];
      stroke_join(m, prev, cur, next, hw, join);
    }

    if (!closed && cap == CAP_ROUND) {
      fill_circle(m, s->pts[0].x, s->pts[0].y, hw);
      fill_circle(m, s->pts[n - 1].x, s->pts[n - 1].y, hw);
    }
  }
}

// ------------------------------------------------------------- composite

static inline int alpha_scale(int a, double alpha) {
  int v = (int)(a * alpha + 0.5);
  return v < 0 ? 0 : (v > 255 ? 255 : v);
}

void raster_fill_mask(Surface *dst, const Mask *mask, const PaintSampler *ps, const DrawParams *dp) {
  if (!dst->px || !mask->m) return;

  int global = composite_op_is_global(dp->op);
  int x0 = global ? 0 : mask->x0;
  int y0 = global ? 0 : mask->y0;
  int x1 = global ? dst->w : mask->x1;
  int y1 = global ? dst->h : mask->y1;

  for (int y = y0; y < y1; y++) {
    uint8_t *row = dst->px + (size_t)y * dst->w * 4;
    const uint8_t *mrow = mask->m + (size_t)y * mask->w;
    for (int x = x0; x < x1; x++) {
      if (dp->clip && !mask_contains(dp->clip, x, y)) continue;
      Color c = { 0, 0, 0, 0 };
      if (mrow[x]) {
        if (!paint_sample(ps, x, y, &c)) c.a = 0;
      } else if (!global) {
        continue;
      }
      blend_pixel(row + x * 4, c.r, c.g, c.b, alpha_scale(c.a, dp->alpha), dp->op);
    }
  }
}

static int matrix_is_axis_aligned(const Matrix *m) {
  return m->b == 0 && m->c == 0;
}

// Una luz (TerraxLighting y otros plugins de RPG Maker): fillRect con un
// gradiente radial concentrico y globalCompositeOperation = "lighter" sobre
// una mascara opaca. Lo caro del camino general es la raiz cuadrada y la
// mezcla de cada pixel; aqui:
//   - el color sale de una tabla indexada por la distancia al cuadrado (sin
//     raiz), ya multiplicada por su alfa: sumar es sumar la tabla
//   - si el ultimo color no suma nada (lo normal: negro o transparente en el
//     borde), cada fila solo recorre el tramo dentro del circulo
//   - con el destino opaco, tres sumas con tope por pixel
#define LIGHT_LUT 8192

static void raster_light(Surface *dst, int px0, int py0, int px1, int py1, const PaintSampler *ps,
                         const Gradient *g, double alpha) {
  static uint8_t add[LIGHT_LUT][4];   // r, g, b sumados y el alfa (para lo no opaco)
  const Matrix *iv = &ps->inverse;
  float r0 = (float)g->r0, r1 = (float)g->r1;
  float r1sq = r1 * r1;
  float to_index = (LIGHT_LUT - 1) / r1sq;
  for (int i = 0; i < LIGHT_LUT; i++) {
    float d = sqrtf(i / to_index);
    float t = (d - r0) / (r1 - r0);
    int k = t <= 0 ? 0 : (t >= 1 ? GRADIENT_LUT - 1 : (int)(t * (GRADIENT_LUT - 1) + 0.5f));
    const Color *c = &ps->lut[k];
    int a = alpha_scale(c->a, alpha);
    int ka = a + (a >> 7);
    add[i][0] = (uint8_t)((c->r * ka) >> 8);
    add[i][1] = (uint8_t)((c->g * ka) >> 8);
    add[i][2] = (uint8_t)((c->b * ka) >> 8);
    add[i][3] = (uint8_t)a;
  }
  // fuera del circulo vale el ultimo color: si no suma nada, no se recorre
  const Color *last = &ps->lut[GRADIENT_LUT - 1];
  int last_a = alpha_scale(last->a, alpha);
  // negro opaco aun cambia el alfa de lo que no es opaco: ahi se mira solo eso
  int outside_zero = last_a == 0 || (last->r == 0 && last->g == 0 && last->b == 0);
  int outside_alpha = outside_zero && last_a > 0;
  uint8_t out_add[4] = { add[LIGHT_LUT - 1][0], add[LIGHT_LUT - 1][1], add[LIGHT_LUT - 1][2], (uint8_t)last_a };

  float sx = (float)iv->a, sy = (float)iv->d;
  float ox = (float)(iv->e - g->x0), oy = (float)(iv->f - g->y0);
  for (int py = py0; py < py1; py++) {
    float uy = sy * (py + 0.5f) + oy;
    float uy2 = uy * uy;
    int x0 = px0, x1 = px1;
    uint8_t *row = dst->px + (size_t)py * dst->w * 4;
    if (outside_zero) {
      if (uy2 >= r1sq) x0 = x1 = px1;
      else {
        // |ux| < sqrt(r1^2 - uy^2), con ux = sx * (px + 0.5) + ox
        float half = sqrtf(r1sq - uy2);
        float a = (-half - ox) / sx - 0.5f, b = (half - ox) / sx - 0.5f;
        if (a > b) { float t = a; a = b; b = t; }
        int lo = (int)floorf(a), hi = (int)ceilf(b) + 1;
        if (lo > x0) x0 = lo;
        if (hi < x1) x1 = hi;
        if (x1 < x0) x1 = x0;
      }
      if (outside_alpha) {
        for (int px = px0; px < px1; px++) {
          if (px == x0) px = x1;
          if (px >= px1) break;
          if (row[px * 4 + 3] != 255) blend_pixel(row + px * 4, 0, 0, 0, last_a, OP_LIGHTER);
        }
      }
    }
    float ux = sx * (x0 + 0.5f) + ox;
    for (int px = x0; px < x1; px++, ux += sx) {
      float d2 = ux * ux + uy2;
      const uint8_t *c = d2 >= r1sq ? out_add : add[(int)(d2 * to_index)];
      uint8_t *d = row + px * 4;
      if (d[3] == 255) {
        int v0 = d[0] + c[0], v1 = d[1] + c[1], v2 = d[2] + c[2];
        d[0] = (uint8_t)(v0 > 255 ? 255 : v0);
        d[1] = (uint8_t)(v1 > 255 ? 255 : v1);
        d[2] = (uint8_t)(v2 > 255 ? 255 : v2);
      } else if (c[3]) {
        // el color sin multiplicar, para la mezcla completa
        int ia = c[3];
        int r = c[0] * 255 / ia, gg = c[1] * 255 / ia, b = c[2] * 255 / ia;
        blend_pixel(d, r > 255 ? 255 : r, gg > 255 ? 255 : gg, b > 255 ? 255 : b, ia, OP_LIGHTER);
      }
    }
  }
}

void raster_fill_rect(Surface *dst, double x, double y, double w, double h,
                      const Matrix *ctm, const PaintSampler *ps, const DrawParams *dp) {
  if (!dst->px) return;

  if (matrix_is_axis_aligned(ctm) && !composite_op_is_global(dp->op)) {
    double ax, ay, bx, by;
    matrix_apply(ctm, x, y, &ax, &ay);
    matrix_apply(ctm, x + w, y + h, &bx, &by);
    if (ax > bx) { double t = ax; ax = bx; bx = t; }
    if (ay > by) { double t = ay; ay = by; by = t; }
    int px0 = (int)ceil(ax - 0.5), px1 = (int)ceil(bx - 0.5);
    int py0 = (int)ceil(ay - 0.5), py1 = (int)ceil(by - 0.5);
    if (px0 < 0) px0 = 0;
    if (py0 < 0) py0 = 0;
    if (px1 > dst->w) px1 = dst->w;
    if (py1 > dst->h) py1 = dst->h;

    // color solido sin clip: fila a fila, sin pedir el color a cada pixel
    if (ps->paint->type == PAINT_COLOR && !dp->clip) {
      Color c = ps->paint->color;
      int a = alpha_scale(c.a, dp->alpha);
      if (dp->op == OP_SOURCE_OVER && a == 255) {
        uint8_t px4[4] = { c.r, c.g, c.b, 255 };
        uint32_t v;
        memcpy(&v, px4, 4);
        for (int py = py0; py < py1; py++) {
          uint32_t *row = (uint32_t *)(dst->px + (size_t)py * dst->w * 4);
          for (int px = px0; px < px1; px++) row[px] = v;
        }
        return;
      }
      if (dp->op == OP_LIGHTER && c.r == 0 && c.g == 0 && c.b == 0) {
        // sumar negro no cambia el color: solo el alfa de lo que no es opaco
        // (TerraxLighting lo hace cada vez sobre toda la mascara)
        if (a == 0) return;
        for (int py = py0; py < py1; py++) {
          uint8_t *row = dst->px + (size_t)py * dst->w * 4;
          for (int px = px0; px < px1; px++) {
            if (row[px * 4 + 3] != 255) blend_pixel(row + px * 4, 0, 0, 0, a, OP_LIGHTER);
          }
        }
        return;
      }
      if (dp->op == OP_LIGHTER) {
        // suma con tope: con el destino opaco (una mascara de luces) no hace
        // falta mas; si no, la mezcla completa
        int ka = a + (a >> 7);
        uint8_t add[4] = { (uint8_t)((c.r * ka) >> 8), (uint8_t)((c.g * ka) >> 8), (uint8_t)((c.b * ka) >> 8), 0 };
        for (int py = py0; py < py1; py++) {
          uint8_t *row = dst->px + (size_t)py * dst->w * 4;
          for (int px = px0; px < px1; px++) {
            uint8_t *d = row + px * 4;
            if (d[3] == 255) {
              int v0 = d[0] + add[0], v1 = d[1] + add[1], v2 = d[2] + add[2];
              d[0] = (uint8_t)(v0 > 255 ? 255 : v0);
              d[1] = (uint8_t)(v1 > 255 ? 255 : v1);
              d[2] = (uint8_t)(v2 > 255 ? 255 : v2);
            } else {
              blend_pixel(d, c.r, c.g, c.b, a, OP_LIGHTER);
            }
          }
        }
        return;
      }
      for (int py = py0; py < py1; py++) {
        uint8_t *row = dst->px + (size_t)py * dst->w * 4;
        for (int px = px0; px < px1; px++) blend_pixel(row + px * 4, c.r, c.g, c.b, a, dp->op);
      }
      return;
    }

    // luz: gradiente radial concentrico sumado (lighter), sin clip ni giro
    const Gradient *lg = ps->paint->gradient;
    if (ps->paint->type == PAINT_GRADIENT && lg && ps->has_lut && ps->invertible && !dp->clip &&
        dp->op == OP_LIGHTER && lg->type == GRADIENT_RADIAL && lg->x0 == lg->x1 && lg->y0 == lg->y1 &&
        lg->r1 > lg->r0 && ps->inverse.b == 0 && ps->inverse.c == 0 && ps->inverse.a != 0) {
      raster_light(dst, px0, py0, px1, py1, ps, lg, dp->alpha);
      return;
    }

    // gradiente lineal o radial concentrico sin clip: bucle propio, con las
    // coordenadas de usuario incrementales y la tabla de colores
    const Gradient *gr = ps->paint->gradient;
    if (ps->paint->type == PAINT_GRADIENT && gr && ps->has_lut && ps->invertible && !dp->clip &&
        (gr->type == GRADIENT_LINEAR ||
         (gr->x0 == gr->x1 && gr->y0 == gr->y1 && gr->r1 > gr->r0))) {
      const Matrix *iv = &ps->inverse;
      int linear = gr->type == GRADIENT_LINEAR;
      double ldx = gr->x1 - gr->x0, ldy = gr->y1 - gr->y0;
      double len2 = ldx * ldx + ldy * ldy;
      if (linear && len2 == 0) return;
      float scale = linear ? 0 : (float)((GRADIENT_LUT - 1) / (gr->r1 - gr->r0));
      float r0 = (float)gr->r0;
      for (int py = py0; py < py1; py++) {
        uint8_t *row = dst->px + (size_t)py * dst->w * 4;
        double ux = iv->a * (px0 + 0.5) + iv->c * (py + 0.5) + iv->e - gr->x0;
        double uy = iv->b * (px0 + 0.5) + iv->d * (py + 0.5) + iv->f - gr->y0;
        for (int px = px0; px < px1; px++, ux += iv->a, uy += iv->b) {
          float t;
          if (linear) t = (float)((ux * ldx + uy * ldy) / len2 * (GRADIENT_LUT - 1));
          else t = (sqrtf((float)(ux * ux + uy * uy)) - r0) * scale;
          int i = t <= 0 ? 0 : (t >= GRADIENT_LUT - 1 ? GRADIENT_LUT - 1 : (int)(t + 0.5f));
          const Color *c = &ps->lut[i];
          blend_pixel(row + px * 4, c->r, c->g, c->b, alpha_scale(c->a, dp->alpha), dp->op);
        }
      }
      return;
    }

    for (int py = py0; py < py1; py++) {
      uint8_t *row = dst->px + (size_t)py * dst->w * 4;
      for (int px = px0; px < px1; px++) {
        if (dp->clip && !mask_contains(dp->clip, px, py)) continue;
        Color c;
        if (!paint_sample(ps, px, py, &c)) continue;
        blend_pixel(row + px * 4, c.r, c.g, c.b, alpha_scale(c.a, dp->alpha), dp->op);
      }
    }
    return;
  }

  Path p;
  path_init(&p);
  double px, py;
  matrix_apply(ctm, x, y, &px, &py);         path_move_to(&p, px, py);
  matrix_apply(ctm, x + w, y, &px, &py);     path_line_to(&p, px, py);
  matrix_apply(ctm, x + w, y + h, &px, &py); path_line_to(&p, px, py);
  matrix_apply(ctm, x, y + h, &px, &py);     path_line_to(&p, px, py);
  path_close(&p);

  Mask m;
  if (mask_init(&m, dst->w, dst->h) == 0) {
    mask_fill_path(&m, &p, FILL_NONZERO);
    raster_fill_mask(dst, &m, ps, dp);
    mask_free(&m);
  }
  path_free(&p);
}

void raster_clear_rect(Surface *dst, double x, double y, double w, double h,
                       const Matrix *ctm, const Mask *clip) {
  // sin clip ni giro: poner a cero las filas (RPG Maker limpia las ventanas
  // enteras a menudo; pixel a pixel con destination-out eran ~5 ms cada vez)
  if (!clip && dst->px && matrix_is_axis_aligned(ctm)) {
    double ax, ay, bx, by;
    matrix_apply(ctm, x, y, &ax, &ay);
    matrix_apply(ctm, x + w, y + h, &bx, &by);
    if (ax > bx) { double t = ax; ax = bx; bx = t; }
    if (ay > by) { double t = ay; ay = by; by = t; }
    int px0 = (int)ceil(ax - 0.5), px1 = (int)ceil(bx - 0.5);
    int py0 = (int)ceil(ay - 0.5), py1 = (int)ceil(by - 0.5);
    if (px0 < 0) px0 = 0;
    if (py0 < 0) py0 = 0;
    if (px1 > dst->w) px1 = dst->w;
    if (py1 > dst->h) py1 = dst->h;
    if (px1 <= px0 || py1 <= py0) return;
    if (px0 == 0 && px1 == dst->w) {
      memset(dst->px + (size_t)py0 * dst->w * 4, 0, (size_t)(py1 - py0) * dst->w * 4);
    } else {
      for (int py = py0; py < py1; py++) memset(dst->px + ((size_t)py * dst->w + px0) * 4, 0, (size_t)(px1 - px0) * 4);
    }
    return;
  }
  Paint paint = { .type = PAINT_COLOR, .color = { 0, 0, 0, 0 } };
  PaintSampler ps;
  paint_sampler_init(&ps, &paint, ctm);
  // destination-out con alpha 255 deja los pixeles transparentes
  paint.color.a = 255;
  DrawParams dp = { .alpha = 1.0, .op = OP_DESTINATION_OUT, .clip = clip };
  raster_fill_rect(dst, x, y, w, h, ctm, &ps, &dp);
}

void raster_draw_image(Surface *dst, const Surface *src,
                       double sx, double sy, double sw, double sh,
                       double dx, double dy, double dw, double dh,
                       const Matrix *ctm, const DrawParams *dp) {
  if (!dst->px || !src->px || sw == 0 || sh == 0 || dw == 0 || dh == 0) return;

  // rectangulos con ancho/alto negativo se normalizan (como el navegador)
  if (sw < 0) { sx += sw; sw = -sw; }
  if (sh < 0) { sy += sh; sh = -sh; }
  if (dw < 0) { dx += dw; dw = -dw; }
  if (dh < 0) { dy += dh; dh = -dh; }

  // recorte del rectangulo origen a la imagen, ajustando el destino
  if (sx < 0) { double k = -sx / sw; dx += dw * k; dw -= dw * k; sw += sx; sx = 0; }
  if (sy < 0) { double k = -sy / sh; dy += dh * k; dh -= dh * k; sh += sy; sy = 0; }
  if (sx + sw > src->w) { double k = (sx + sw - src->w) / sw; dw -= dw * k; sw = src->w - sx; }
  if (sy + sh > src->h) { double k = (sy + sh - src->h) / sh; dh -= dh * k; sh = src->h - sy; }
  if (sw <= 0 || sh <= 0 || dw <= 0 || dh <= 0) return;

  int global = composite_op_is_global(dp->op);

  // Camino rapido: solo traslacion entera, sin escala
  if (!global && ctm->a == 1 && ctm->b == 0 && ctm->c == 0 && ctm->d == 1 &&
      dw == sw && dh == sh && sx == floor(sx) && sy == floor(sy)) {
    double fx = dx + ctm->e, fy = dy + ctm->f;
    int ox = (int)ceil(fx - 0.5), oy = (int)ceil(fy - 0.5);
    int isx = (int)sx, isy = (int)sy, isw = (int)ceil(sw), ish = (int)ceil(sh);
    // el recorte al destino, una vez (no en cada pixel)
    int i0 = ox < 0 ? -ox : 0, i1 = ox + isw > dst->w ? dst->w - ox : isw;
    int j0 = oy < 0 ? -oy : 0, j1 = oy + ish > dst->h ? dst->h - oy : ish;
    // source-over sin globalAlpha ni clip (bitmap.blt de RPG Maker): un pixel
    // opaco, o sobre uno transparente, se copia tal cual; el resto se mezcla
    int plain = dp->op == OP_SOURCE_OVER && dp->alpha >= 1.0 && !dp->clip;
    for (int j = j0; j < j1; j++) {
      int ty = oy + j;
      const uint8_t *s = src->px + ((size_t)(isy + j) * src->w + isx + i0) * 4;
      uint8_t *d = dst->px + ((size_t)ty * dst->w + ox + i0) * 4;
      if (plain) {
        for (int i = i0; i < i1; i++, s += 4, d += 4) {
          int a = s[3];
          if (a == 0) continue;
          if (a == 255 || d[3] == 0) memcpy(d, s, 4);
          else blend_pixel(d, s[0], s[1], s[2], a, OP_SOURCE_OVER);
        }
        continue;
      }
      for (int i = i0; i < i1; i++, s += 4, d += 4) {
        if (dp->clip && !mask_contains(dp->clip, ox + i, ty)) continue;
        if (s[3] == 0) continue;
        blend_pixel(d, s[0], s[1], s[2], alpha_scale(s[3], dp->alpha), dp->op);
      }
    }
    return;
  }

  Matrix inv;
  if (matrix_invert(&inv, ctm) != 0) return;

  int x0 = 0, y0 = 0, x1 = dst->w, y1 = dst->h;
  if (!global) {
    double cx[4], cy[4];
    matrix_apply(ctm, dx, dy, &cx[0], &cy[0]);
    matrix_apply(ctm, dx + dw, dy, &cx[1], &cy[1]);
    matrix_apply(ctm, dx + dw, dy + dh, &cx[2], &cy[2]);
    matrix_apply(ctm, dx, dy + dh, &cx[3], &cy[3]);
    double minx = cx[0], maxx = cx[0], miny = cy[0], maxy = cy[0];
    for (int i = 1; i < 4; i++) {
      if (cx[i] < minx) minx = cx[i];
      if (cx[i] > maxx) maxx = cx[i];
      if (cy[i] < miny) miny = cy[i];
      if (cy[i] > maxy) maxy = cy[i];
    }
    x0 = (int)floor(minx); y0 = (int)floor(miny);
    x1 = (int)ceil(maxx);  y1 = (int)ceil(maxy);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > dst->w) x1 = dst->w;
    if (y1 > dst->h) y1 = dst->h;
  }

  double kx = sw / dw, ky = sh / dh;
  int sx_max = (int)ceil(sx + sw) - 1;
  int sy_max = (int)ceil(sy + sh) - 1;

  // Escala y traslacion sin giro (el fondo de una ventana de RPG Maker: su
  // patron estirado): la columna de origen solo depende de x y la fila de y.
  // Se calculan una vez, con la misma cuenta que el camino general (el
  // resultado es el mismo pixel a pixel)
  if (!global && inv.b == 0 && inv.c == 0 && x1 > x0 && y1 > y0) {
    int nx = x1 - x0;
    int *cols = malloc(sizeof(int) * (size_t)nx);
    if (cols) {
      double ux = inv.a * (x0 + 0.5) + inv.e;
      for (int i = 0; i < nx; i++, ux += inv.a) {
        if (ux >= dx && ux < dx + dw) {
          int ix = (int)floor(sx + (ux - dx) * kx);
          cols[i] = ix > sx_max ? sx_max : ix < 0 ? 0 : ix;
        } else {
          cols[i] = -1;
        }
      }
      int plain = dp->op == OP_SOURCE_OVER && dp->alpha >= 1.0 && !dp->clip;
      for (int y = y0; y < y1; y++) {
        double uy = inv.d * (y + 0.5) + inv.f;
        if (!(uy >= dy && uy < dy + dh)) continue;
        int iy = (int)floor(sy + (uy - dy) * ky);
        if (iy > sy_max) iy = sy_max;
        if (iy < 0) iy = 0;
        const uint8_t *srow = src->px + (size_t)iy * src->w * 4;
        uint8_t *d = dst->px + ((size_t)y * dst->w + x0) * 4;
        for (int i = 0; i < nx; i++, d += 4) {
          if (cols[i] < 0) continue;
          const uint8_t *s = srow + cols[i] * 4;
          if (plain) {
            int a = s[3];
            if (a == 0) continue;
            if (a == 255 || d[3] == 0) memcpy(d, s, 4);
            else blend_pixel(d, s[0], s[1], s[2], a, OP_SOURCE_OVER);
            continue;
          }
          if (dp->clip && !mask_contains(dp->clip, x0 + i, y)) continue;
          if (s[3] == 0) continue;
          blend_pixel(d, s[0], s[1], s[2], alpha_scale(s[3], dp->alpha), dp->op);
        }
      }
      free(cols);
      return;
    }
  }

  for (int y = y0; y < y1; y++) {
    uint8_t *drow = dst->px + (size_t)y * dst->w * 4;
    // en cada fila la coordenada de usuario avanza linealmente
    double ux = inv.a * (x0 + 0.5) + inv.c * (y + 0.5) + inv.e;
    double uy = inv.b * (x0 + 0.5) + inv.d * (y + 0.5) + inv.f;
    for (int x = x0; x < x1; x++, ux += inv.a, uy += inv.b) {
      if (dp->clip && !mask_contains(dp->clip, x, y)) continue;
      int inside = ux >= dx && ux < dx + dw && uy >= dy && uy < dy + dh;
      if (!inside) {
        if (global) blend_pixel(drow + x * 4, 0, 0, 0, 0, dp->op);
        continue;
      }
      int ix = (int)floor(sx + (ux - dx) * kx);
      int iy = (int)floor(sy + (uy - dy) * ky);
      if (ix > sx_max) ix = sx_max;
      if (iy > sy_max) iy = sy_max;
      if (ix < 0) ix = 0;
      if (iy < 0) iy = 0;
      const uint8_t *s = src->px + ((size_t)iy * src->w + ix) * 4;
      if (s[3] == 0 && !global) continue;
      blend_pixel(drow + x * 4, s[0], s[1], s[2], alpha_scale(s[3], dp->alpha), dp->op);
    }
  }
}
