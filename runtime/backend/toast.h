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

// La tarjeta de un aviso (Toast), dibujada solo con codigo: sin imagenes.
//
//   +--------------------------------------------+
//   | +------+  Achievement unlocked      3/17   |
//   | |  F   |  First Blood                      |
//   | +------+  Defeat the first boss            |
//   +--------------------------------------------+
//
// Retro, como el launcher: un panel oscuro de esquinas rectas con un borde
// del color del distintivo; el distintivo es un cuadrado de ese color con la
// inicial; todo con la letra de pixeles (stb_easy_font) a escala entera. px es
// el tamano de letra del titulo, que el backend saca del alto de la ventana.

#include "backend.h"
#include "gfx/surface.h"

// Dibuja la tarjeta en una superficie nueva (RGBA, alfa normal). 0 si va bien.
int toast_render(const Toast *t, int px, Surface *out);

// La cola de avisos: cada modulo (logros, trucos...) pone el suyo y salen uno
// detras de otro, TOAST_MS cada uno.
void toast_push(const Toast *t);
// Una vez por imagen: si no hay uno en pantalla, el siguiente de la cola
void toast_tick(Backend *backend, double now_ms);
void toast_clear(void);
