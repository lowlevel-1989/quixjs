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

// La tarjeta de un aviso y la cola de avisos. Ver toast.h.

#include <stdio.h>
#include <string.h>
#include "stb/stb_easy_font.h"
#include "toast.h"

// Retro: rectangulos de bordes rectos y la letra de pixeles del launcher
// (stb_easy_font), a escala entera

static void fill(Surface *s, int x, int y, int w, int h, unsigned rgb) {
  for (int yy = y < 0 ? 0 : y; yy < y + h && yy < s->h; yy++) {
    for (int xx = x < 0 ? 0 : x; xx < x + w && xx < s->w; xx++) {
      uint8_t *p = s->px + ((size_t)yy * s->w + xx) * 4;
      p[0] = (uint8_t)(rgb >> 16);
      p[1] = (uint8_t)(rgb >> 8);
      p[2] = (uint8_t)rgb;
      p[3] = 255;
    }
  }
}

// El texto: stb_easy_font lo da como cuadrados (4 vertices de 16 bytes)
static char quads[40000];

static int text_w(const char *text, int scale) {
  int n = stb_easy_font_print(0, 0, (char *)text, NULL, quads, sizeof quads);
  float right = 0;
  for (int q = 0; q < n; q++) {
    float x = ((const float *)(quads + q * 64 + 32))[0];
    if (x > right) right = x;
  }
  return (int)right * scale;
}

static void text(Surface *s, int x, int y, const char *str, int scale, unsigned rgb) {
  int n = stb_easy_font_print(0, 0, (char *)str, NULL, quads, sizeof quads);
  for (int q = 0; q < n; q++) {
    const float *a = (const float *)(quads + q * 64);
    const float *c = (const float *)(quads + q * 64 + 32);
    int x0 = x + (int)a[0] * scale, y0 = y + (int)a[1] * scale;
    fill(s, x0, y0, ((int)c[0] - (int)a[0]) * scale, ((int)c[1] - (int)a[1]) * scale, rgb);
  }
}

// Un texto que no cabe en max: se corta con "..."
static void fit(const char *in, int scale, int max, char *out, size_t room) {
  snprintf(out, room, "%s", in);
  size_t n = strlen(out);
  while (n > 3 && text_w(out, scale) > max) {
    n--;
    out[n] = 0;
    if (n >= 3) memcpy(out + n - 3, "...", 3);
  }
}

static unsigned shade(unsigned rgb, float k) {
  unsigned r = (rgb >> 16) & 255, g = (rgb >> 8) & 255, b = rgb & 255;
  r = (unsigned)(r * k > 255 ? 255 : r * k);
  g = (unsigned)(g * k > 255 ? 255 : g * k);
  b = (unsigned)(b * k > 255 ? 255 : b * k);
  return (r << 16) | (g << 8) | b;
}

int toast_render(const Toast *t, int px, Surface *out) {
  // px: el tamano de letra del titulo que pide el backend; en escalas enteras
  int big = px / 7 < 2 ? 2 : px / 7;          // el titulo
  int small = big - 1 < 1 ? 1 : big - 1;      // la cabecera y el texto
  int u = small;                              // la unidad de los bordes
  int pad = 4 * u;    // el hueco de arriba y abajo
  int pad_x = 8 * u;  // el de los lados: la letra no va pegada al borde
  unsigned color = t->color ? t->color : 0x7ab8ff;
  int max_text = px * 18;   // en una ventana de 624 de alto, unos 400 px

  // un titulo largo baja un tamano antes de cortarse
  int title_scale = big;
  if (text_w(t->title, big) > max_text) title_scale = small;
  char lines[3][192];
  fit(t->header, small, max_text, lines[0], sizeof lines[0]);
  fit(t->title, title_scale, max_text, lines[1], sizeof lines[1]);
  fit(t->text, small, max_text, lines[2], sizeof lines[2]);
  int scales[3] = { small, title_scale, small };
  int heights[3] = { 12 * small, 12 * title_scale, t->text[0] ? 12 * small : 0 };
  int text_wd = 0, text_h = 0;
  for (int i = 0; i < 3; i++) {
    int w = text_w(lines[i], scales[i]);
    if (w > text_wd) text_wd = w;
    text_h += heights[i];
  }

  int badge = text_h - 2 * u;
  int w = pad_x + badge + pad + text_wd + pad_x;
  int h = pad + text_h + pad - 2 * u;
  if (surface_init(out, w, h) != 0) return -1;

  // el panel: un borde duro del color y dentro el fondo oscuro
  fill(out, 0, 0, w, h, color);
  fill(out, u, u, w - 2 * u, h - 2 * u, 0x101014);
  // el distintivo: un cuadrado del color, con borde, y la inicial
  int bx = pad_x, by = (h - badge) / 2;
  fill(out, bx, by, badge, badge, shade(color, 0.5f));
  fill(out, bx + u, by + u, badge - 2 * u, badge - 2 * u, color);
  int is = badge / 10 < 1 ? 1 : badge / 10;
  if (t->badge[0]) {
    text(out, bx + (badge - text_w(t->badge, is)) / 2, by + (badge - 7 * is) / 2, t->badge, is, 0x101014);
  }
  // las lineas: cabecera del color, titulo blanco, texto gris
  unsigned rgb[3] = { shade(color, 1.2f), 0xffffff, 0xb4b6c2 };
  int tx = bx + badge + pad, ty = (h - text_h) / 2 + u;
  for (int i = 0; i < 3; i++) {
    if (!heights[i]) continue;
    text(out, tx, ty, lines[i], scales[i], rgb[i]);
    ty += heights[i];
  }
  return 0;
}

// ---------- la cola de avisos ----------

#define TOAST_QUEUE 16

static Toast    queue[TOAST_QUEUE];
static int      queued = 0;
static unsigned serial = 0;
static double   busy_until = 0;

void toast_push(const Toast *t) {
  if (queued == TOAST_QUEUE) return;   // demasiados a la vez: el resto no sale
  queue[queued++] = *t;
}

void toast_tick(Backend *backend, double now_ms) {
  if (busy_until && now_ms < busy_until) return;
  busy_until = 0;
  if (!queued || !backend) return;
  backend->toast = queue[0];
  backend->toast.serial = ++serial;
  memmove(queue, queue + 1, sizeof queue[0] * (size_t)--queued);
  busy_until = now_ms + TOAST_MS + 300;   // un respiro entre uno y otro
}

void toast_clear(void) {
  queued = 0;
  busy_until = 0;
}
