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

// runtime/gfx/soft/raster/mask.c: la mask de recorte: el fill de un path (non-zero / even-odd) y su stroke
// (caps y joins), y la interseccion de dos masks.
// Parte de runtime/gfx/soft/raster.c (archivo indice).

#include "common.h"

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
