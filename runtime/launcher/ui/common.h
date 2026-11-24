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

// runtime/launcher/ui/common.h: lo que comparten las partes de ui, y nadie
// mas (el resto de quixjs usa ui.h):
//   ui.c     abrir y cerrar, montar una pantalla, ui_run y ui_message
//   draw.c   dibujar una pantalla: rectangulos, texto, la lista y la leyenda
//   input.c  lo que se pulsa: teclado, mando y --script
#pragma once

#include "ui.h"

// Las medidas de tamatama (src/shared/screen.c): una pantalla de 480 de alto,
// el texto de stb_easy_font a escala 1, 2 o 3 (unos 7 de alto por escala)
#define VH      480     // alto de la pantalla virtual
// (un poco mas grandes que en tamatama: en la pantalla de una portatil, a
// escala 1 y 2 se leia mal)
#define ROW     38      // una opcion cada ROW
#define LINE    30
#define BAR_W   420     // la barra de la opcion elegida
#define BAR_H   32
#define ITEM    2       // escala de las opciones (item_scale la cambia: el menu principal, 3)

// El estado de las pantallas (ui_open lo llena)
typedef struct {
  Backend *backend;
  Surface  screen;
  UiColors colors;
  double   k;        // pixeles por unidad virtual
  int      vw;       // ancho virtual
  long     frames;
} UiState;
extern UiState ui_st;

// draw.c: dibujar la pantalla, y la medida de la lista (la usa ui_run)
void  ui_draw(const Screen *s);
int   ui_sliding(const Screen *s);
int   ui_rows_of(const Screen *s);
float ui_max_scroll(const Screen *s);
float ui_clampf(float v, float lo, float hi);

// input.c: lo que se pulsa, de una en una
enum { IN_NONE, IN_UP, IN_DOWN, IN_LEFT, IN_RIGHT, IN_ACCEPT, IN_BACK };
void ui_read_input(void);
int  ui_next_input(void);
void ui_input_clear(void);   // lo pulsado antes no cuenta
