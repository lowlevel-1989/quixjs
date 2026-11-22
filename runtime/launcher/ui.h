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

// Las pantallas propias de quixjs (el launcher, Transfer Save, los logros),
// las que no son del juego. Heredado de tamatama (src/shared/screen.h). Todas
// son iguales:
//   un logo y un titulo arriba (y una firma pequena, en el launcher)
//   unas lineas de texto
//   una lista de opciones: las primeras van en una ventana que se desliza si
//   no caben; las ultimas (tantas como Screen.fixed), debajo y quietas
//   un aviso y la ayuda abajo
// Se eligen con las flechas o la cruceta y aceptar (Enter, Z, A); volver con
// Escape, X o B. Se dibujan en una superficie del tamano del juego, con
// coordenadas de una pantalla de 480 de alto, y se muestran con el backend.

#include "backend/backend.h"
#include "gfx/surface.h"

#define UI_LINES 8
#define UI_ITEMS 300     // los logros de un port caben (256) y Back

// Colores 0xRRGGBB
typedef struct {
  unsigned background, text, logo, accent, muted, notice, highlight;
} UiColors;

typedef struct {
  const char *logo;          // grande, arriba
  int logo_scale, logo_y;     // escala de la letra: 1, 2, 3... (tamatama)
  const char *sub;           // pequeno, debajo del logo (la firma); o NULL
  int sub_y;
  const char *title;         // debajo del logo; o NULL
  int title_y;
  char lines[UI_LINES][128];
  unsigned line_rgb[UI_LINES];
  int line_scale[UI_LINES];
  int line_count, lines_top;
  char items[UI_ITEMS][128];
  unsigned item_rgb[UI_ITEMS];   // el color de cada opcion (0: el del texto)
  int item_scale;                // la escala de las opciones (0: 2)
  unsigned item_badge[UI_ITEMS]; // un cuadrado de color delante (0: sin el): el tipo
  int item_hollow[UI_ITEMS];     // el cuadrado solo con el borde (un logro sin conseguir)
  int align_left;                // las opciones a la izquierda de la barra (no centradas)
  // una linea de leyenda debajo de las lineas: un cuadrado de color y su texto
  char legend[4][48];
  unsigned legend_rgb[4];
  int legend_count;
  int count;
  int fixed;                 // las ultimas opciones, quietas debajo
  int list_top;              // la y de la primera opcion (como poco: debajo de las lineas)
  int rows;                  // cuantas de las que se deslizan se ven (como mucho: las que
                             // caben sin llegar a la ayuda)
  int can_back;              // volver sale de la pantalla
  const char *notice;        // un aviso abajo; o NULL
  int no_help;               // sin la ayuda de abajo (mover, elegir)
  // el estado: se conserva entre llamadas a ui_run
  int selected;
  float scroll, target;
} Screen;

// Lo que se hace mientras se ve una pantalla, una vez por imagen (la red).
// Puede cambiar la pantalla. Devuelve 0 para seguir.
typedef int (*ScreenTick)(Screen *s, void *context);

#define SCREEN_BACK  -1      // se pulso volver
#define SCREEN_EVENT 100     // SCREEN_EVENT + lo que devolvio tick
#define SCREEN_QUIT  -2      // se cerro la ventana (o Select + Start)

// Abre las pantallas sobre el backend (ya iniciado) con el tamano del juego
void ui_open(Backend *backend, int w, int h, const UiColors *colors);
void ui_close(void);
const UiColors *ui_colors(void);
// La superficie de la ultima imagen (para las capturas de --script)
const Surface *ui_surface(void);
// Cuantas imagenes lleva (el reloj de --script)
long ui_frames(void);
// El codigo de un exit de --script en las pantallas propias, o -1
int  ui_quit_code(void);

// Una pantalla vacia con el logo logo y el titulo title
void ui_reset(Screen *s, const char *logo, const char *title);
void ui_line(Screen *s, unsigned rgb, const char *format, ...) __attribute__((format(printf, 3, 4)));
// Una firma de Transfer Save: grande y resaltada, que es lo que se compara
void ui_signature(Screen *s, const char *signature);
void ui_item(Screen *s, const char *text);
// Ensena la pantalla hasta que se elige una opcion, se vuelve, se cierra o
// tick (si no es NULL) devuelve algo
int  ui_run(Screen *s, ScreenTick tick, void *context);
// Un aviso con OK. tick sigue funcionando mientras se ve.
int  ui_message(const char *logo, const char *title, const char *line1, const char *line2,
                ScreenTick tick, void *context);
