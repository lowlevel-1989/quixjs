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

#pragma once

// Rasterizado por software: paths (relleno y trazo), rectangulos e imagenes
// sobre una Surface. Sin antialiasing: un pixel se pinta si su centro queda
// dentro de la figura (como imageSmoothingEnabled = false).

#include "surface.h"
#include "paint.h"

typedef struct {
  double x, y;
} PathPoint;

typedef struct {
  PathPoint *pts;
  int        count;
  int        cap;
  int        closed;
} SubPath;

// Los puntos se guardan ya transformados (coordenadas de dispositivo), como
// dice el spec: la matriz que cuenta es la del momento de moveTo/lineTo.
typedef struct {
  SubPath *subs;
  int      count;
  int      cap;
} Path;

typedef enum {
  FILL_NONZERO,
  FILL_EVENODD
} FillRule;

typedef enum {
  CAP_BUTT,
  CAP_ROUND,
  CAP_SQUARE
} LineCap;

typedef enum {
  JOIN_MITER,
  JOIN_ROUND,
  JOIN_BEVEL
} LineJoin;

// Mascara de cobertura (0 o 255 por pixel) del tamano de la superficie.
// Tambien se usa para clip().
typedef struct {
  int      w, h;
  uint8_t *m;
  int      x0, y0, x1, y1;   // caja con algo pintado (x1, y1 exclusivos)
} Mask;

void path_init(Path *p);
void path_free(Path *p);
void path_clear(Path *p);
int  path_copy(Path *dst, const Path *src);
void path_move_to(Path *p, double x, double y);
void path_line_to(Path *p, double x, double y);
void path_close(Path *p);
int  path_has_current_point(const Path *p);
void path_current_point(const Path *p, double *x, double *y);

int  mask_init(Mask *m, int w, int h);
void mask_free(Mask *m);
int  mask_copy(Mask *dst, const Mask *src);
void mask_fill_path(Mask *m, const Path *p, FillRule rule);
void mask_stroke_path(Mask *m, const Path *p, double width, LineCap cap, LineJoin join);
void mask_intersect(Mask *m, const Mask *other);   // m = m AND other
int  mask_contains(const Mask *m, int x, int y);

typedef struct {
  double      alpha;      // globalAlpha
  CompositeOp op;
  const Mask *clip;       // NULL si no hay clip
} DrawParams;

// Pinta la mascara con la pintura (color, gradiente o patron).
void raster_fill_mask(Surface *dst, const Mask *mask, const PaintSampler *ps, const DrawParams *dp);

// fillRect/clearRect con camino rapido si la matriz solo traslada/escala.
void raster_fill_rect(Surface *dst, double x, double y, double w, double h,
                      const Matrix *ctm, const PaintSampler *ps, const DrawParams *dp);
void raster_clear_rect(Surface *dst, double x, double y, double w, double h,
                       const Matrix *ctm, const Mask *clip);

// drawImage con los 9 parametros ya resueltos.
void raster_draw_image(Surface *dst, const Surface *src,
                       double sx, double sy, double sw, double sh,
                       double dx, double dy, double dw, double dh,
                       const Matrix *ctm, const DrawParams *dp);
