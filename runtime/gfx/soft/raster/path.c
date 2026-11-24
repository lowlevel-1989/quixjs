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

// runtime/gfx/soft/raster/path.c: el path: subpaths de puntos (move_to, line_to, close) y el punto actual.
// Parte de runtime/gfx/soft/raster.c (archivo indice).

#include "common.h"

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
