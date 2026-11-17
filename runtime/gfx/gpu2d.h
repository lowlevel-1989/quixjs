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

// Un canvas 2d dibujado en la GPU (OpenGL ES 2), para los que un hook marca
// como "gpu-canvas": las mascaras de luces de RPG Maker (TerraxLighting...),
// que se repintan enteras cada actualizacion y por software son lo mas caro.
//
// El canvas vive en una textura (alfa premultiplicado; la fila 0 es la de
// arriba) y se dibuja con un contexto GL propio que comparte texturas con el
// de WebGL (gl_aux_begin): no toca el estado GL del juego. Lo dibujado se
// apunta y se pinta de una vez cuando hace falta: al subirlo a WebGL
// (gpu2d_texture) o al leerlo (gpu2d_read).
//
// En la GPU: fillRect, clearRect, fill y stroke (la cobertura del path se
// calcula por software y se sube como mascara), con color o gradiente
// (lineal o radial concentrico), globalAlpha, la transformacion y los modos
// source-over, lighter, destination-out, destination-over, source-atop, xor y
// screen. Lo demas (clip, patrones, sombras, otros modos, texto, imagenes)
// devuelve -1: el canvas se lee y sigue por software.

#include "surface.h"
#include "paint.h"
#include "raster.h"

typedef struct Gpu2D Gpu2D;

// 1 si hay GPU para canvas (WebGL disponible)
int    gpu2d_available(void);
// Nuevo, con el contenido de s (puede estar vacio). NULL si no se puede.
Gpu2D *gpu2d_new(const Surface *s);
void   gpu2d_free(Gpu2D *g);
// Cambia de tamano y lo deja transparente (como el canvas)
int    gpu2d_resize(Gpu2D *g, int w, int h);

// 0 si queda apuntado, -1 si la GPU no lo hace
int    gpu2d_fill_rect(Gpu2D *g, double x, double y, double w, double h, const Matrix *ctm,
                       const Paint *paint, double alpha, CompositeOp op);
int    gpu2d_clear_rect(Gpu2D *g, double x, double y, double w, double h, const Matrix *ctm);
// Una cobertura ya calculada (fill / stroke de un path)
int    gpu2d_fill_mask(Gpu2D *g, const Mask *m, const Matrix *ctm, const Paint *paint, double alpha,
                       CompositeOp op);

// Lo dibujado, a s (RGBA sin premultiplicar, del tamano del canvas)
int    gpu2d_read(Gpu2D *g, Surface *s);
// Pinta lo pendiente y da la textura (premultiplicada) y su tamano
unsigned gpu2d_texture(Gpu2D *g, int *w, int *h);
// Dibujar desde ahora directamente en dst (una textura de WebGL del tamano
// del canvas, a la que se acaba de copiar entero): las siguientes subidas a
// esa textura no copian nada
int    gpu2d_retarget(Gpu2D *g, unsigned dst);
// 1 si g ya dibuja en tex
int    gpu2d_is_target(const Gpu2D *g, unsigned tex);
// WebGL borra tex: el canvas que dibujaba en ella vuelve a una textura propia
void   gpu2d_forget_texture(unsigned tex);
