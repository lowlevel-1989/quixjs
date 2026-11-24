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

// runtime/gfx/gl/gpu2d/common.h: lo que comparten las partes de
// runtime/gfx/gl/gpu2d.c: los comandos (Cmd), el canvas (Gpu2D) y la lista de
// los que dibujan en una textura de WebGL.

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../gl.h"
#include "../state.h"
#include "../gpu2d.h"
#include "shaders.h"   // runtime/gfx/gl/shaders, generado por tools/embed_shaders.py

#define LUT_ROWS 64          // gradientes por tanda (una fila de 256 colores cada uno)
#define MAX_CMDS 1024

typedef enum { MODE_COLOR = 0, MODE_LINEAR = 1, MODE_RADIAL = 2, MODE_CLEAR = 3 } PaintMode;

typedef struct {
  float       pos[8];        // las 4 esquinas en el dispositivo (tira de triangulos)
  float       uv[8];         // las mismas en el espacio de usuario (gradientes)
  float       mt[8];         // en la mascara (0..1)
  PaintMode   mode;
  float       color[4];      // color sin premultiplicar (0..1)
  float       alpha;         // globalAlpha
  float       grad[6];       // x0, y0, x1, y1, r0, r1
  int         lut;           // fila del gradiente
  CompositeOp op;
  uint8_t    *mask;          // cobertura de la caja (o NULL)
  int         mw, mh;
} Cmd;

struct Gpu2D {
  int       w, h;
  unsigned  tex;             // compartida con WebGL
  unsigned  fbo;             // del contexto auxiliar
  Cmd      *cmds;
  int       count, cap;
  uint8_t   lut[LUT_ROWS][256 * 4];
  int       luts;
  int       opaque;          // todo el canvas es opaco (tras un relleno opaco entero)
  int       external;        // tex es la textura de WebGL a la que se subia (gpu2d_retarget)
  // lo ultimo pintado entero (empieza con un relleno de todo el canvas): si
  // lo siguiente es igual, la textura ya lo tiene (ver flush)
  Cmd      *last;
  int       last_n, last_cap, last_luts;
  unsigned  last_tex;
  uint8_t  *last_lut;
  unsigned  mfbo, mfbo_tex;  // el framebuffer en el contexto de WebGL (ver flush)
};

// los canvas que dibujan en una textura de WebGL (para gpu2d_forget_texture)
#define MAX_EXTERNAL 16
static Gpu2D *external_list[MAX_EXTERNAL];

// en texture.c; lo usan target.c (liberar, cambiar tamano) y texture.c
static void external_set(Gpu2D *g, int on);
