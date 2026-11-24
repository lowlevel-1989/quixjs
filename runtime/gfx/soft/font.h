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

// Fuentes TrueType con SDL_ttf.
//
// Las familias se registran desde JS (FontFace, @font-face) con la ruta del
// .ttf/.otf. Si una familia no esta registrada se usa la fuente por defecto:
// la variable de entorno QUIXJS_FONT o la primera que exista de una lista de
// fuentes habituales de Linux (DejaVu, Noto, Liberation).

#include "surface.h"

typedef struct {
  int    ascent;    // pixeles sobre la linea base
  int    descent;   // pixeles bajo la linea base (positivo)
  int    height;
} FontMetrics;

// Se pueden usar desde varios threads: cada funcion entra sola (un candado)
typedef struct FontHandle FontHandle;

int  font_init(void);
void font_quit(void);

int  font_register(const char *family, const char *path);
// La fuente desde memoria (FontFace con un ArrayBuffer): se copia y dura el juego
int  font_register_data(const char *family, const void *data, size_t len);
int  font_is_registered(const char *family);

// Fuente para un valor CSS de ctx.font ("bold 20px 'GameFont', sans-serif").
// NULL si no hay ninguna fuente disponible.
FontHandle *font_from_css(const char *css);
// La misma, pero propia: no se cierra ni la mueve la cache del juego, y no
// cambia a otra fuente si le faltan letras. Para el thread que presenta (los
// avisos encima del juego), que dibuja a la vez que el juego. Hasta 8
// distintas; NULL si no cabe
FontHandle *font_from_css_fixed(const char *css);

void font_metrics(FontHandle *f, FontMetrics *m);
int  font_text_width(FontHandle *f, const char *utf8);

// Dibuja el texto en una superficie nueva (RGBA, color c). outline > 0 para
// el contorno (strokeText). Devuelve 0 si todo bien.
int  font_render(FontHandle *f, const char *utf8, unsigned char r, unsigned char g,
                 unsigned char b, unsigned char a, int outline, Surface *out);
