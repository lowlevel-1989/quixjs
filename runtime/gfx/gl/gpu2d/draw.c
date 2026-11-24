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

// runtime/gfx/gl/gpu2d/draw.c: lo que llama el canvas 2d: fill_rect, clear_rect y
// fill_mask, cada uno un comando nuevo.
// Parte de runtime/gfx/gl/gpu2d.c (archivo indice).

#include "common.h"

// Un comando nuevo con la pintura; NULL si la GPU no la hace
static Cmd *new_cmd(Gpu2D *g, const Matrix *ctm, const Paint *paint, double alpha, CompositeOp op) {
  if (!op_supported(op)) return NULL;
  PaintMode mode = MODE_COLOR;
  const Gradient *gr = NULL;
  if (paint->type == PAINT_GRADIENT) {
    gr = paint->gradient;
    if (!gr) return NULL;
    if (gr->type == GRADIENT_LINEAR) mode = MODE_LINEAR;
    else if (gr->x0 == gr->x1 && gr->y0 == gr->y1 && gr->r1 > gr->r0) mode = MODE_RADIAL;
    else return NULL;   // radial con dos centros: por software
  } else if (paint->type != PAINT_COLOR) {
    return NULL;        // patrones: por software
  }
  if ((g->count == MAX_CMDS || (gr && g->luts == LUT_ROWS)) && flush(g) != 0) return NULL;
  if (g->count == g->cap) {
    int cap = g->cap ? g->cap * 2 : 32;
    Cmd *n = realloc(g->cmds, sizeof(Cmd) * cap);
    if (!n) return NULL;
    g->cmds = n;
    g->cap = cap;
  }
  Cmd *c = &g->cmds[g->count++];
  memset(c, 0, sizeof(*c));
  c->mode = mode;
  c->alpha = (float)alpha;
  c->op = op;
  if (mode == MODE_COLOR) {
    c->color[0] = paint->color.r / 255.0f;
    c->color[1] = paint->color.g / 255.0f;
    c->color[2] = paint->color.b / 255.0f;
    c->color[3] = paint->color.a / 255.0f;
  } else {
    // la misma tabla de 256 colores que usa el software
    PaintSampler ps;
    paint_sampler_init(&ps, paint, ctm);
    c->lut = g->luts++;
    uint8_t *row = g->lut[c->lut];
    for (int i = 0; i < 256; i++) {
      const Color *k = &ps.lut[i * (GRADIENT_LUT - 1) / 255];
      row[i * 4 + 0] = k->r;
      row[i * 4 + 1] = k->g;
      row[i * 4 + 2] = k->b;
      row[i * 4 + 3] = k->a;
    }
    c->grad[0] = (float)gr->x0;
    c->grad[1] = (float)gr->y0;
    c->grad[2] = (float)gr->x1;
    c->grad[3] = (float)gr->y1;
    c->grad[4] = (float)gr->r0;
    c->grad[5] = (float)gr->r1;
  }
  // sin mascara: la textura blanca (cualquier coordenada vale)
  for (int i = 0; i < 8; i++) c->mt[i] = 0.5f;
  return c;
}

// Las 4 esquinas de un rectangulo de usuario, en el orden de la tira
static void set_rect(Cmd *c, double x, double y, double w, double h, const Matrix *ctm) {
  const double ux[4] = { x, x + w, x, x + w };
  const double uy[4] = { y, y, y + h, y + h };
  for (int i = 0; i < 4; i++) {
    double dx, dy;
    matrix_apply(ctm, ux[i], uy[i], &dx, &dy);
    c->pos[i * 2] = (float)dx;
    c->pos[i * 2 + 1] = (float)dy;
    c->uv[i * 2] = (float)ux[i];
    c->uv[i * 2 + 1] = (float)uy[i];
  }
}

// Si el rectangulo cubre todo el canvas (sin giro)
static int covers_canvas(const Gpu2D *g, double x, double y, double w, double h, const Matrix *ctm) {
  if (ctm->b != 0 || ctm->c != 0) return 0;
  double x0, y0, x1, y1;
  matrix_apply(ctm, x, y, &x0, &y0);
  matrix_apply(ctm, x + w, y + h, &x1, &y1);
  if (x0 > x1) { double t = x0; x0 = x1; x1 = t; }
  if (y0 > y1) { double t = y0; y0 = y1; y1 = t; }
  return x0 <= 0.5 && y0 <= 0.5 && x1 >= g->w - 0.5 && y1 >= g->h - 0.5;
}

int gpu2d_fill_rect(Gpu2D *g, double x, double y, double w, double h, const Matrix *ctm,
                    const Paint *paint, double alpha, CompositeOp op) {
  if (paint->type == PAINT_COLOR) {
    int a = (int)(paint->color.a * alpha + 0.5);
    // Un relleno opaco de todo el canvas tapa lo que hubiera: lo apuntado y
    // aun sin pintar ya no hace falta (una mascara de luces se repinta en cada
    // actualizacion y solo se ve la ultima) y se pinta con glClear
    if (op == OP_SOURCE_OVER && a >= 255 && covers_canvas(g, x, y, w, h, ctm)) {
      drop_cmds(g);
      Cmd *c = new_cmd(g, ctm, paint, 1.0, op);
      if (!c) return -1;
      c->mode = MODE_CLEAR;
      c->color[3] = 1.0f;
      g->opaque = 1;
      return 0;
    }
    // sumar negro a un canvas opaco no cambia nada
    if (op == OP_LIGHTER && g->opaque && paint->color.r == 0 && paint->color.g == 0 && paint->color.b == 0) {
      return 0;
    }
  }
  // sumar o pintar encima de algo opaco lo deja opaco; los demas modos no
  if (op != OP_LIGHTER && op != OP_SOURCE_OVER) g->opaque = 0;
  Cmd *c = new_cmd(g, ctm, paint, alpha, op);
  if (!c) return -1;
  set_rect(c, x, y, w, h, ctm);
  return 0;
}

int gpu2d_clear_rect(Gpu2D *g, double x, double y, double w, double h, const Matrix *ctm) {
  Paint black = { .type = PAINT_COLOR, .color = { 0, 0, 0, 255 } };
  g->opaque = 0;
  if (covers_canvas(g, x, y, w, h, ctm)) {
    drop_cmds(g);
    Cmd *c = new_cmd(g, ctm, &black, 1.0, OP_SOURCE_OVER);
    if (!c) return -1;
    c->mode = MODE_CLEAR;
    c->color[0] = c->color[1] = c->color[2] = c->color[3] = 0.0f;
    return 0;
  }
  Cmd *c = new_cmd(g, ctm, &black, 1.0, OP_DESTINATION_OUT);
  if (!c) return -1;
  set_rect(c, x, y, w, h, ctm);
  return 0;
}

int gpu2d_fill_mask(Gpu2D *g, const Mask *m, const Matrix *ctm, const Paint *paint, double alpha,
                    CompositeOp op) {
  int x0 = m->x0 < 0 ? 0 : m->x0, y0 = m->y0 < 0 ? 0 : m->y0;
  int x1 = m->x1 > m->w ? m->w : m->x1, y1 = m->y1 > m->h ? m->h : m->y1;
  if (x1 <= x0 || y1 <= y0) return 0;   // nada que pintar
  Matrix inv;
  if (matrix_invert(&inv, ctm) != 0) return 0;
  if (op != OP_LIGHTER && op != OP_SOURCE_OVER) g->opaque = 0;
  Cmd *c = new_cmd(g, ctm, paint, alpha, op);
  if (!c) return -1;
  int mw = x1 - x0, mh = y1 - y0;
  c->mask = malloc((size_t)mw * mh);
  if (!c->mask) {
    g->count--;
    return -1;
  }
  for (int y = 0; y < mh; y++) memcpy(c->mask + (size_t)y * mw, m->m + (size_t)(y0 + y) * m->w + x0, mw);
  c->mw = mw;
  c->mh = mh;
  const double dx[4] = { x0, x1, x0, x1 };
  const double dy[4] = { y0, y0, y1, y1 };
  for (int i = 0; i < 4; i++) {
    double ux, uy;
    matrix_apply(&inv, dx[i], dy[i], &ux, &uy);
    c->pos[i * 2] = (float)dx[i];
    c->pos[i * 2 + 1] = (float)dy[i];
    c->uv[i * 2] = (float)ux;
    c->uv[i * 2 + 1] = (float)uy;
    c->mt[i * 2] = (i & 1) ? 1.0f : 0.0f;
    c->mt[i * 2 + 1] = (i & 2) ? 1.0f : 0.0f;
  }
  return 0;
}
