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

// Lo que se puede poner en fillStyle / strokeStyle: un color, un gradiente
// (lineal o radial) o un patron (imagen repetida).

#include "color.h"
#include "surface.h"

#define GRADIENT_MAX_STOPS 32

typedef struct {
  double offset;
  Color  color;
} GradientStop;

typedef enum {
  GRADIENT_LINEAR,
  GRADIENT_RADIAL
} GradientType;

typedef struct {
  GradientType type;
  double x0, y0, r0;
  double x1, y1, r1;
  GradientStop stops[GRADIENT_MAX_STOPS];
  int          stop_count;
} Gradient;

typedef enum {
  PATTERN_REPEAT,
  PATTERN_REPEAT_X,
  PATTERN_REPEAT_Y,
  PATTERN_NO_REPEAT
} PatternRepeat;

typedef struct {
  Surface       image;     // copia de la imagen en el momento de createPattern
  PatternRepeat repeat;
  Matrix        transform; // setTransform() del patron
} Pattern;

typedef enum {
  PAINT_COLOR,
  PAINT_GRADIENT,
  PAINT_PATTERN
} PaintType;

typedef struct {
  PaintType       type;
  Color           color;
  const Gradient *gradient;  // no es propietario (lo mantiene vivo el objeto JS)
  const Pattern  *pattern;
} Paint;

void gradient_add_stop(Gradient *g, double offset, Color c);

// Prepara la pintura para un dibujo con la matriz actual del contexto
// (los gradientes y patrones viven en el espacio de usuario).
#define GRADIENT_LUT 256

typedef struct {
  const Paint *paint;
  Matrix       inverse;   // dispositivo -> usuario
  int          invertible;
  Color        lut[GRADIENT_LUT];   // colores del gradiente precalculados
  int          has_lut;
} PaintSampler;

void paint_sampler_init(PaintSampler *ps, const Paint *paint, const Matrix *ctm);

// Color de la pintura en el centro del pixel (x, y) del dispositivo.
// Devuelve 0 si el pixel queda fuera (patron no-repeat).
int paint_sample(const PaintSampler *ps, int x, int y, Color *out);
