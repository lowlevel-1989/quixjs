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

// runtime/launcher/ui/draw.c: dibujar una pantalla de ui en ui_st.screen:
// rectangulos, triangulos (las flechas), el texto de stb_easy_font, la lista
// que se desliza con su barra, la leyenda de colores, el aviso y la ayuda.

#include <math.h>
#include <string.h>
#include "stb/stb_easy_font.h"
#include "common.h"
#include "backend/star.h"

static void fill(int x, int y, int w, int h, unsigned rgb) {
  int x0 = (int)lround(x * ui_st.k), y0 = (int)lround(y * ui_st.k);
  int x1 = (int)lround((x + w) * ui_st.k), y1 = (int)lround((y + h) * ui_st.k);
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > ui_st.screen.w) x1 = ui_st.screen.w;
  if (y1 > ui_st.screen.h) y1 = ui_st.screen.h;
  uint8_t r = (uint8_t)(rgb >> 16), g = (uint8_t)(rgb >> 8), b = (uint8_t)rgb;
  for (int yy = y0; yy < y1; yy++) {
    uint8_t *p = ui_st.screen.px + ((size_t)yy * ui_st.screen.w + x0) * 4;
    for (int xx = x0; xx < x1; xx++, p += 4) {
      p[0] = r; p[1] = g; p[2] = b; p[3] = 255;
    }
  }
}

static void fill_triangle(float ax, float ay, float bx, float by, float cx, float cy, unsigned rgb) {
  // pequeno: se recorre su caja y se mira el lado de cada arista
  float minx = fminf(ax, fminf(bx, cx)) * ui_st.k, maxx = fmaxf(ax, fmaxf(bx, cx)) * ui_st.k;
  float miny = fminf(ay, fminf(by, cy)) * ui_st.k, maxy = fmaxf(ay, fmaxf(by, cy)) * ui_st.k;
  ax *= ui_st.k; ay *= ui_st.k; bx *= ui_st.k; by *= ui_st.k; cx *= ui_st.k; cy *= ui_st.k;
  float area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
  if (area == 0) return;
  for (int y = (int)miny; y <= (int)maxy; y++) {
    for (int x = (int)minx; x <= (int)maxx; x++) {
      if (x < 0 || y < 0 || x >= ui_st.screen.w || y >= ui_st.screen.h) continue;
      float px = x + 0.5f, py = y + 0.5f;
      float w0 = (bx - ax) * (py - ay) - (by - ay) * (px - ax);
      float w1 = (cx - bx) * (py - by) - (cy - by) * (px - bx);
      float w2 = (ax - cx) * (py - cy) - (ay - cy) * (px - cx);
      if ((area > 0 && w0 >= 0 && w1 >= 0 && w2 >= 0) || (area < 0 && w0 <= 0 && w1 <= 0 && w2 <= 0)) {
        uint8_t *p = ui_st.screen.px + ((size_t)y * ui_st.screen.w + x) * 4;
        p[0] = (uint8_t)(rgb >> 16); p[1] = (uint8_t)(rgb >> 8); p[2] = (uint8_t)rgb; p[3] = 255;
      }
    }
  }
}

// El texto: stb_easy_font lo da como cuadrados (4 vertices de 16 bytes:
// x, y, z y el color), que se rellenan a la escala pedida
// El ancho de lo que se dibuja (stb_easy_font_width cuenta tambien el hueco
// detras de la ultima letra, y el texto quedaria corrido a la izquierda)
static int text_width(const char *text, int scale) {
  static char quads[60000];
  int n = stb_easy_font_print(0, 0, (char *)text, NULL, quads, sizeof quads);
  float right = 0;
  for (int q = 0; q < n; q++) {
    float x = ((const float *)(quads + q * 64 + 32))[0];
    if (x > right) right = x;
  }
  return (int)lround(right * scale);
}

static void draw_text(int x, int y, const char *text, int scale, unsigned rgb) {
  static char quads[60000];
  int n = stb_easy_font_print(0, 0, (char *)text, NULL, quads, sizeof quads);
  for (int q = 0; q < n; q++) {
    const float *a = (const float *)(quads + q * 64);        // arriba a la izquierda
    const float *c = (const float *)(quads + q * 64 + 32);   // abajo a la derecha
    int x0 = (int)lround((x + a[0] * scale) * ui_st.k), y0 = (int)lround((y + a[1] * scale) * ui_st.k);
    int x1 = (int)lround((x + c[0] * scale) * ui_st.k), y1 = (int)lround((y + c[1] * scale) * ui_st.k);
    if (x1 <= x0) x1 = x0 + 1;
    if (y1 <= y0) y1 = y0 + 1;
    for (int yy = y0 < 0 ? 0 : y0; yy < y1 && yy < ui_st.screen.h; yy++) {
      for (int xx = x0 < 0 ? 0 : x0; xx < x1 && xx < ui_st.screen.w; xx++) {
        uint8_t *p = ui_st.screen.px + ((size_t)yy * ui_st.screen.w + xx) * 4;
        p[0] = (uint8_t)(rgb >> 16);
        p[1] = (uint8_t)(rgb >> 8);
        p[2] = (uint8_t)rgb;
        p[3] = 255;
      }
    }
  }
}

// Texto centrado en x; si no cabe, a una escala menos (como tamatama)
static void centered(int y, const char *text, int scale, unsigned rgb) {
  if (!text || !text[0]) return;
  while (scale > 1 && text_width(text, scale) > ui_st.vw - 24) {
    scale--;
    y += 4;
  }
  draw_text((ui_st.vw - text_width(text, scale)) / 2, y, text, scale, rgb);
}

// La lista: una opcion cada ROW desde list_top. De las que se deslizan se
// ven rows a la vez; si hay mas, unas flechas y una barra lo dicen. Las fijas
// van debajo, 16 mas separadas.
int ui_sliding(const Screen *s) { return s->count - s->fixed; }
// La leyenda: cuantas lineas ocupa (las partes que caben en cada una)
static int legend_width(const Screen *s, int from, int to) {
  int w = 0;
  for (int i = from; i < to; i++) w += 18 + text_width(s->legend[i], 2) + (i > from ? 24 : 0);
  return w;
}

static int legend_rows(const Screen *s) {
  int rows = 0;
  for (int i = 0; i < s->legend_count; rows++) {
    int j = i + 1;
    while (j < s->legend_count && legend_width(s, i, j + 1) <= ui_st.vw - 80) j++;
    i = j;
  }
  return rows;
}

// Donde empieza la lista: list_top, pero nunca encima de las lineas (con
// hueco para la flecha de arriba)
static int top_of(const Screen *s) {
  int end = s->lines_top;
  for (int i = 0; i < s->line_count; i++) end += s->line_scale[i] >= 4 ? LINE + 10 : LINE;
  end += legend_rows(s) * 22 + (s->legend_count ? 8 : 0);
  int min = s->line_count || s->legend_count ? end + 30 : 0;
  return s->list_top > min ? s->list_top : min;
}

// Cuantas se ven: rows, pero sin que la lista (y las fijas de debajo) lleguen
// al aviso y la ayuda de abajo
int ui_rows_of(const Screen *s) {
  int fixed = s->fixed ? s->fixed * ROW + 20 : 0;
  int room = (VH - (s->notice ? 92 : 66) - top_of(s) - fixed) / ROW;
  if (room < 1) room = 1;
  return s->rows < room ? s->rows : room;
}

static int visible(const Screen *s) { return ui_sliding(s) < ui_rows_of(s) ? ui_sliding(s) : ui_rows_of(s); }
float ui_max_scroll(const Screen *s) { return (float)(ui_sliding(s) - visible(s)); }
static int window_top(const Screen *s) { return top_of(s) - 8; }
static int window_bottom(const Screen *s) { return window_top(s) + visible(s) * ROW; }
float ui_clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }

static int item_y(const Screen *s, int i) {
  int n = ui_sliding(s);
  if (i < n) return top_of(s) + (int)((i - s->scroll) * ROW + (i >= s->scroll ? 0.5f : -0.5f));
  return top_of(s) + (visible(s) + i - n) * ROW + (n ? 20 : 0);
}

static int items_scale(const Screen *s) {
  int scale = s->item_scale ? s->item_scale : ITEM;
  for (int i = 0; i < s->count && scale > 1; i++) {
    while (scale > 1 && text_width(s->items[i], scale) > BAR_W - 16) scale--;
  }
  return scale;
}

static void draw_item(const Screen *s, int i) {
  int y = item_y(s, i);
  int box = BAR_W;
  // un texto largo: la barra lo cubre entero
  // todas las opciones de la pantalla a la misma escala: la mayor en la que
  // caben todas en la barra
  int want = s->item_scale ? s->item_scale : ITEM;
  int scale = items_scale(s), dy = (want - scale) * 3 + (3 - want) * 3;
  if (i == s->selected) fill((ui_st.vw - box) / 2, y - 8, box, BAR_H, ui_st.colors.accent);
  // la elegida, siempre en el color del texto (se lee sobre la barra)
  // (un logro sin conseguir sigue en gris: asi se distingue con la barra encima)
  unsigned rgb = s->item_rgb[i] && (i != s->selected || s->item_hollow[i]) ? s->item_rgb[i] : ui_st.colors.text;
  // la estrella (runtime/backend/star.h): despues del texto, cuenta para centrarlo
  int star = s->item_star[i] ? (STAR_SIZE + STAR_GAP) * scale : 0;
  int tx = (ui_st.vw - text_width(s->items[i], scale) - star) / 2;
  // a la izquierda, solo las que se deslizan (las fijas, como Back, centradas)
  if (s->align_left && i < ui_sliding(s)) tx = (ui_st.vw - box) / 2 + 16 + (s->item_badge[i] ? 4 * scale + 12 : 0);
  draw_text(tx, y + dy, s->items[i], scale, rgb);
  if (star) {
    int sx = tx + text_width(s->items[i], scale) + STAR_GAP * scale, sy = y + dy - scale;
    for (int r = 0; r < STAR_SIZE; r++)
      for (int c = 0; c < STAR_SIZE; c++)
        if (STAR_ART[r][c] == 'X') fill(sx + c * scale, sy + r * scale, scale, scale, s->item_star[i]);
  }
  // el tipo: un cuadrado de su color delante del texto
  // hueco (solo el borde): un logro sin conseguir; se ve igual con la barra
  if (s->item_badge[i]) {
    int bx = tx - 4 * scale - 10, size = 4 * scale + 2, b = scale > 1 ? 2 : 1;
    fill(bx, y + dy, size, size, s->item_badge[i]);
    if (s->item_hollow[i])
      fill(bx + b, y + dy + b, size - 2 * b, size - 2 * b, i == s->selected ? ui_st.colors.accent : ui_st.colors.background);
  }
}

void ui_draw(const Screen *s) {
  fill(0, 0, ui_st.vw, VH, ui_st.colors.background);

  // las que se deslizan; lo que se sale de la ventana lo tapa el fondo de despues
  for (int i = 0; i < ui_sliding(s); i++) {
    int y = item_y(s, i);
    if (y >= window_top(s) - ROW && y <= window_bottom(s) + ROW) draw_item(s, i);
  }
  fill(0, 0, ui_st.vw, window_top(s), ui_st.colors.background);
  fill(0, window_bottom(s), ui_st.vw, VH - window_bottom(s), ui_st.colors.background);

  if (s->logo) centered(s->logo_y, s->logo, s->logo_scale, ui_st.colors.logo);
  if (s->sub) centered(s->sub_y, s->sub, 2, ui_st.colors.muted);
  if (s->title) centered(s->title_y, s->title, 3, ui_st.colors.highlight);
  for (int i = 0, y = s->lines_top; i < s->line_count; y += s->line_scale[i] >= 4 ? LINE + 10 : LINE, i++) {
    centered(y, s->lines[i], s->line_scale[i], s->line_rgb[i]);
  }

  // la leyenda: cada parte con su cuadrado de color, todas en una linea
  if (s->legend_count) {
    int y = s->lines_top;
    for (int i = 0; i < s->line_count; i++) y += s->line_scale[i] >= 4 ? LINE + 10 : LINE;
    for (int i = 0; i < s->legend_count; y += 22) {
      int j = i + 1;
      while (j < s->legend_count && legend_width(s, i, j + 1) <= ui_st.vw - 80) j++;
      int x = (ui_st.vw - legend_width(s, i, j)) / 2;
      for (int k = i; k < j; k++) {
        if (k > i) x += 24;
        fill(x, y + 1, 10, 10, s->legend_rgb[k]);
        x += 18;
        draw_text(x, y, s->legend[k], 2, ui_st.colors.muted);
        x += text_width(s->legend[k], 2);
      }
      i = j;
    }
  }

  // si hay mas de las que caben: una flecha hacia donde quedan y, a la
  // derecha, la barra con la parte que se ve
  if (ui_sliding(s) > ui_rows_of(s)) {
    float c = ui_st.vw / 2.0f;
    if (s->scroll > 0.01f) {
      float y = (float)window_top(s) - 6;
      fill_triangle(c - 9, y, c + 9, y, c, y - 9, ui_st.colors.highlight);
    }
    if (s->scroll < ui_max_scroll(s) - 0.01f) {
      float y = (float)window_bottom(s) + 2;
      fill_triangle(c - 9, y, c + 9, y, c, y + 9, ui_st.colors.highlight);
    }
    int track = window_bottom(s) - window_top(s);
    int thumb = track * ui_rows_of(s) / ui_sliding(s);
    int at = window_top(s) + (int)((track - thumb) * s->scroll / ui_max_scroll(s) + 0.5f);
    int bar_x = (ui_st.vw + BAR_W) / 2 + 6;
    fill(bar_x, window_top(s), 3, track, ui_st.colors.accent);
    fill(bar_x, at, 3, thumb, ui_st.colors.highlight);
  }

  for (int i = ui_sliding(s); i < s->count; i++) draw_item(s, i);

  if (s->notice) centered(VH - 76, s->notice, 2, ui_st.colors.notice);
  if (!s->no_help) centered(VH - 48, "arrows, d-pad: move   enter, A: choose", 2, ui_st.colors.muted);
  if (s->can_back && !s->no_help) centered(VH - 26, "escape, B: go back", 2, ui_st.colors.muted);
}
