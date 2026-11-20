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

// Imagenes grandes a menos resolucion (opcional: [game] image_scale).
//
// Un juego hecho para 1280x720 (Firework) trae ilustraciones y fondos que en
// una portatil de 640x480 nunca se ven a su tamano, y cada una ocupa su
// tamano entero en RAM y en la GPU. Con image_scale, las imagenes de las
// carpetas elegidas (image_scale_dirs) que tienen un lado de al menos
// image_scale_min pixeles se reducen al decodificarlas, y la reducida se
// guarda en <cache_dir>/images-<porcentaje>/ (PNG): la siguiente vez se lee
// esa, sin descifrar ni decodificar la grande.
//
// El juego no se entera: la imagen sigue diciendo su tamano de siempre
// (width, naturalWidth) y quien dibuja con ella lo tiene en cuenta: WebGL
// (PIXI) usa coordenadas de 0 a 1 en la textura, y el canvas 2d
// (drawImage, createPattern) pasa las coordenadas a la escala reducida.

#include <stddef.h>
#include <stdint.h>

#define IMAGE_SCALE_MAX_DIRS 8

// Una vez al arrancar (thread principal). scale fuera de (0, 1): apagado.
// dirs: carpetas desde game_dir ("img/pictures"); sin ninguna, no reduce nada
void image_scale_init(const char *cache_dir, double scale, int min, const char dirs[][128], int ndirs);
// 1 si la imagen de src (la url del juego, "img/pictures/a.png") se reduce
int  image_scale_wanted(const char *src);
// Decodifica path (el archivo ya resuelto, con su transformacion si la tiene:
// descifrar), reducida si es grande, desde la cache si ya esta. Da sus
// pixeles (malloc), su tamano y el tamano natural (el de la imagen entera).
// Vale desde otro thread. 0 si bien
int  image_scale_decode(const char *path, uint8_t **px, int *w, int *h, int *nat_w, int *nat_h);
