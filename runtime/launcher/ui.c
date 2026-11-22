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

// Dibujar las pantallas propias y elegir en ellas. Heredado de tamatama
// (src/shared/screen.c): la misma lista que se desliza, las mismas flechas y
// la misma barra, la misma letra (stb_easy_font) y las mismas medidas; aqui
// con el mando del backend y los colores de [launcher].

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "core/input_script.h"
#include "stb/stb_easy_font.h"
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

static Backend  *ui_backend = NULL;
static Surface   screen;
static UiColors  colors;
static double    k = 1.0;   // pixeles por unidad virtual
static int       vw = 640;  // ancho virtual
static long      frames = 0;

void ui_open(Backend *backend, int w, int h, const UiColors *c) {
  ui_backend = backend;
  if (w <= 0) w = 640;
  if (h <= 0) h = 480;
  surface_init(&screen, w, h);
  colors = *c;
  k = h / (double)VH;
  vw = (int)(w / k);
}

void ui_close(void) {
  surface_free(&screen);
  ui_backend = NULL;
}

const UiColors *ui_colors(void) { return &colors; }
const Surface *ui_surface(void) { return &screen; }
long ui_frames(void) { return frames; }

static void fill(int x, int y, int w, int h, unsigned rgb) {
  int x0 = (int)lround(x * k), y0 = (int)lround(y * k);
  int x1 = (int)lround((x + w) * k), y1 = (int)lround((y + h) * k);
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > screen.w) x1 = screen.w;
  if (y1 > screen.h) y1 = screen.h;
  uint8_t r = (uint8_t)(rgb >> 16), g = (uint8_t)(rgb >> 8), b = (uint8_t)rgb;
  for (int yy = y0; yy < y1; yy++) {
    uint8_t *p = screen.px + ((size_t)yy * screen.w + x0) * 4;
    for (int xx = x0; xx < x1; xx++, p += 4) {
      p[0] = r; p[1] = g; p[2] = b; p[3] = 255;
    }
  }
}

static void fill_triangle(float ax, float ay, float bx, float by, float cx, float cy, unsigned rgb) {
  // pequeno: se recorre su caja y se mira el lado de cada arista
  float minx = fminf(ax, fminf(bx, cx)) * k, maxx = fmaxf(ax, fmaxf(bx, cx)) * k;
  float miny = fminf(ay, fminf(by, cy)) * k, maxy = fmaxf(ay, fmaxf(by, cy)) * k;
  ax *= k; ay *= k; bx *= k; by *= k; cx *= k; cy *= k;
  float area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
  if (area == 0) return;
  for (int y = (int)miny; y <= (int)maxy; y++) {
    for (int x = (int)minx; x <= (int)maxx; x++) {
      if (x < 0 || y < 0 || x >= screen.w || y >= screen.h) continue;
      float px = x + 0.5f, py = y + 0.5f;
      float w0 = (bx - ax) * (py - ay) - (by - ay) * (px - ax);
      float w1 = (cx - bx) * (py - by) - (cy - by) * (px - bx);
      float w2 = (ax - cx) * (py - cy) - (ay - cy) * (px - cx);
      if ((area > 0 && w0 >= 0 && w1 >= 0 && w2 >= 0) || (area < 0 && w0 <= 0 && w1 <= 0 && w2 <= 0)) {
        uint8_t *p = screen.px + ((size_t)y * screen.w + x) * 4;
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
    int x0 = (int)lround((x + a[0] * scale) * k), y0 = (int)lround((y + a[1] * scale) * k);
    int x1 = (int)lround((x + c[0] * scale) * k), y1 = (int)lround((y + c[1] * scale) * k);
    if (x1 <= x0) x1 = x0 + 1;
    if (y1 <= y0) y1 = y0 + 1;
    for (int yy = y0 < 0 ? 0 : y0; yy < y1 && yy < screen.h; yy++) {
      for (int xx = x0 < 0 ? 0 : x0; xx < x1 && xx < screen.w; xx++) {
        uint8_t *p = screen.px + ((size_t)yy * screen.w + xx) * 4;
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
  while (scale > 1 && text_width(text, scale) > vw - 24) {
    scale--;
    y += 4;
  }
  draw_text((vw - text_width(text, scale)) / 2, y, text, scale, rgb);
}

// La lista: una opcion cada ROW desde list_top. De las que se deslizan se
// ven rows a la vez; si hay mas, unas flechas y una barra lo dicen. Las fijas
// van debajo, 16 mas separadas.
static int sliding(const Screen *s) { return s->count - s->fixed; }
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
    while (j < s->legend_count && legend_width(s, i, j + 1) <= vw - 80) j++;
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
static int rows_of(const Screen *s) {
  int fixed = s->fixed ? s->fixed * ROW + 20 : 0;
  int room = (VH - (s->notice ? 92 : 66) - top_of(s) - fixed) / ROW;
  if (room < 1) room = 1;
  return s->rows < room ? s->rows : room;
}

static int visible(const Screen *s) { return sliding(s) < rows_of(s) ? sliding(s) : rows_of(s); }
static float max_scroll(const Screen *s) { return (float)(sliding(s) - visible(s)); }
static int window_top(const Screen *s) { return top_of(s) - 8; }
static int window_bottom(const Screen *s) { return window_top(s) + visible(s) * ROW; }
static float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }

static int item_y(const Screen *s, int i) {
  int n = sliding(s);
  if (i < n) return top_of(s) + (int)((i - s->scroll) * ROW + (i >= s->scroll ? 0.5f : -0.5f));
  return top_of(s) + (visible(s) + i - n) * ROW + (n ? 20 : 0);
}

void ui_reset(Screen *s, const char *logo, const char *title) {
  memset(s, 0, sizeof *s);
  s->logo = logo;
  s->logo_scale = 4;
  s->logo_y = 24;
  s->sub_y = 64;
  s->title = title;
  s->title_y = 64;
  s->lines_top = 104;
  s->list_top = 250;
  s->rows = 4;
  s->can_back = 1;
}

void ui_line(Screen *s, unsigned rgb, const char *format, ...) {
  if (s->line_count == UI_LINES) return;
  va_list args;
  va_start(args, format);
  vsnprintf(s->lines[s->line_count], sizeof s->lines[0], format, args);
  va_end(args);
  s->line_scale[s->line_count] = 3;
  s->line_rgb[s->line_count++] = rgb;
}

void ui_signature(Screen *s, const char *signature) {
  ui_line(s, colors.highlight, "%s", signature);
  s->line_scale[s->line_count - 1] = 4;
}

void ui_item(Screen *s, const char *text) {
  if (s->count == UI_ITEMS) return;
  s->item_rgb[s->count] = 0;
  s->item_badge[s->count] = 0;
  s->item_hollow[s->count] = 0;
  snprintf(s->items[s->count++], sizeof s->items[0], "%s", text);
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
  if (i == s->selected) fill((vw - box) / 2, y - 8, box, BAR_H, colors.accent);
  // la elegida, siempre en el color del texto (se lee sobre la barra)
  // (un logro sin conseguir sigue en gris: asi se distingue con la barra encima)
  unsigned rgb = s->item_rgb[i] && (i != s->selected || s->item_hollow[i]) ? s->item_rgb[i] : colors.text;
  int tx = (vw - text_width(s->items[i], scale)) / 2;
  // a la izquierda, solo las que se deslizan (las fijas, como Back, centradas)
  if (s->align_left && i < sliding(s)) tx = (vw - box) / 2 + 16 + (s->item_badge[i] ? 4 * scale + 12 : 0);
  draw_text(tx, y + dy, s->items[i], scale, rgb);
  // el tipo: un cuadrado de su color delante del texto
  // hueco (solo el borde): un logro sin conseguir; se ve igual con la barra
  if (s->item_badge[i]) {
    int bx = tx - 4 * scale - 10, size = 4 * scale + 2, b = scale > 1 ? 2 : 1;
    fill(bx, y + dy, size, size, s->item_badge[i]);
    if (s->item_hollow[i])
      fill(bx + b, y + dy + b, size - 2 * b, size - 2 * b, i == s->selected ? colors.accent : colors.background);
  }
}

static void draw(const Screen *s) {
  fill(0, 0, vw, VH, colors.background);

  // las que se deslizan; lo que se sale de la ventana lo tapa el fondo de despues
  for (int i = 0; i < sliding(s); i++) {
    int y = item_y(s, i);
    if (y >= window_top(s) - ROW && y <= window_bottom(s) + ROW) draw_item(s, i);
  }
  fill(0, 0, vw, window_top(s), colors.background);
  fill(0, window_bottom(s), vw, VH - window_bottom(s), colors.background);

  if (s->logo) centered(s->logo_y, s->logo, s->logo_scale, colors.logo);
  if (s->sub) centered(s->sub_y, s->sub, 2, colors.muted);
  if (s->title) centered(s->title_y, s->title, 3, colors.highlight);
  for (int i = 0, y = s->lines_top; i < s->line_count; y += s->line_scale[i] >= 4 ? LINE + 10 : LINE, i++) {
    centered(y, s->lines[i], s->line_scale[i], s->line_rgb[i]);
  }

  // la leyenda: cada parte con su cuadrado de color, todas en una linea
  if (s->legend_count) {
    int y = s->lines_top;
    for (int i = 0; i < s->line_count; i++) y += s->line_scale[i] >= 4 ? LINE + 10 : LINE;
    for (int i = 0; i < s->legend_count; y += 22) {
      int j = i + 1;
      while (j < s->legend_count && legend_width(s, i, j + 1) <= vw - 80) j++;
      int x = (vw - legend_width(s, i, j)) / 2;
      for (int k = i; k < j; k++) {
        if (k > i) x += 24;
        fill(x, y + 1, 10, 10, s->legend_rgb[k]);
        x += 18;
        draw_text(x, y, s->legend[k], 2, colors.muted);
        x += text_width(s->legend[k], 2);
      }
      i = j;
    }
  }

  // si hay mas de las que caben: una flecha hacia donde quedan y, a la
  // derecha, la barra con la parte que se ve
  if (sliding(s) > rows_of(s)) {
    float c = vw / 2.0f;
    if (s->scroll > 0.01f) {
      float y = (float)window_top(s) - 6;
      fill_triangle(c - 9, y, c + 9, y, c, y - 9, colors.highlight);
    }
    if (s->scroll < max_scroll(s) - 0.01f) {
      float y = (float)window_bottom(s) + 2;
      fill_triangle(c - 9, y, c + 9, y, c, y + 9, colors.highlight);
    }
    int track = window_bottom(s) - window_top(s);
    int thumb = track * rows_of(s) / sliding(s);
    int at = window_top(s) + (int)((track - thumb) * s->scroll / max_scroll(s) + 0.5f);
    int bar_x = (vw + BAR_W) / 2 + 6;
    fill(bar_x, window_top(s), 3, track, colors.accent);
    fill(bar_x, at, 3, thumb, colors.highlight);
  }

  for (int i = sliding(s); i < s->count; i++) draw_item(s, i);

  if (s->notice) centered(VH - 76, s->notice, 2, colors.notice);
  if (!s->no_help) centered(VH - 48, "arrows, d-pad: move   enter, A: choose", 2, colors.muted);
  if (s->can_back && !s->no_help) centered(VH - 26, "escape, B: go back", 2, colors.muted);
}

// Lo que se pulsa, de una en una
enum { IN_NONE, IN_UP, IN_DOWN, IN_LEFT, IN_RIGHT, IN_ACCEPT, IN_BACK };

#define IN_QUEUE 16
static int  in_queue[IN_QUEUE];
static int  in_len = 0;
static int  pad_prev[GAMEPAD_MAX][GAMEPAD_BUTTONS];
static int  axis_prev[GAMEPAD_MAX][2];
static int  quit_code = -1;

static void push(int in) {
  if (in_len < IN_QUEUE) in_queue[in_len++] = in;
}

static const Surface *script_shot(void *user) {
  (void)user;
  return &screen;
}

static void read_input(void) {
  // --script tambien maneja las pantallas propias: sus frames cuentan aqui
  if (input_script_active()) {
    int code = 0;
    if (input_script_run(frames, ui_backend, script_shot, NULL, &code)) quit_code = code;
  }
  InputEvent ev;
  while (backend_pop_input(ui_backend, &ev)) {
    if (ev.type != INPUT_KEY_DOWN) continue;
    const char *c = ev.code;
    if (!strcmp(c, "ArrowUp")) push(IN_UP);
    else if (!strcmp(c, "ArrowDown")) push(IN_DOWN);
    else if (!strcmp(c, "ArrowLeft")) push(IN_LEFT);
    else if (!strcmp(c, "ArrowRight")) push(IN_RIGHT);
    else if (!strcmp(c, "Enter") || !strcmp(c, "NumpadEnter") || !strcmp(c, "Space") || !strcmp(c, "KeyZ")) push(IN_ACCEPT);
    else if (!strcmp(c, "Escape") || !strcmp(c, "Backspace") || !strcmp(c, "KeyX")) push(IN_BACK);
  }
  // El mando: la cruceta y el stick; A, X, Y y Start aceptan; B vuelve
  GamepadState pads[GAMEPAD_MAX];
  int n = backend_get_gamepads(ui_backend, pads, GAMEPAD_MAX);
  for (int p = 0; p < n && p < GAMEPAD_MAX; p++) {
    if (!pads[p].connected) continue;
    static const int map[][2] = { {12, IN_UP}, {13, IN_DOWN}, {14, IN_LEFT}, {15, IN_RIGHT},
                                  {0, IN_ACCEPT}, {2, IN_ACCEPT}, {3, IN_ACCEPT}, {9, IN_ACCEPT}, {1, IN_BACK} };
    for (size_t i = 0; i < sizeof map / sizeof map[0]; i++) {
      int b = map[i][0];
      if (pads[p].pressed[b] && !pad_prev[p][b]) push(map[i][1]);
    }
    for (int b = 0; b < GAMEPAD_BUTTONS; b++) pad_prev[p][b] = pads[p].pressed[b];
    int ay = pads[p].axes[1] < -0.6f ? -1 : pads[p].axes[1] > 0.6f ? 1 : 0;
    int ax = pads[p].axes[0] < -0.6f ? -1 : pads[p].axes[0] > 0.6f ? 1 : 0;
    if (ay && ay != axis_prev[p][1]) push(ay < 0 ? IN_UP : IN_DOWN);
    if (ax && ax != axis_prev[p][0]) push(ax < 0 ? IN_LEFT : IN_RIGHT);
    axis_prev[p][0] = ax;
    axis_prev[p][1] = ay;
  }
}

static int next_input(void) {
  if (!in_len) return IN_NONE;
  int in = in_queue[0];
  memmove(in_queue, in_queue + 1, sizeof(int) * (size_t)--in_len);
  return in;
}

int ui_quit_code(void) { return quit_code; }

int ui_run(Screen *s, ScreenTick tick, void *context) {
  in_len = 0;   // lo pulsado antes no cuenta
  for (;;) {
    backend_process_events(ui_backend);
    read_input();
    if (backend_should_close(ui_backend) || quit_code >= 0) return SCREEN_QUIT;
    if (tick) {
      int event = tick(s, context);
      if (event) return SCREEN_EVENT + event;
    }
    // tick puede haber cambiado las opciones
    if (s->selected >= s->count) s->selected = s->count ? s->count - 1 : 0;
    s->target = clampf(s->target, 0, max_scroll(s));

    for (int in; (in = next_input()) != IN_NONE;) {
      if (in == IN_BACK && s->can_back) return SCREEN_BACK;
      if (!s->count) continue;
      if (in == IN_LEFT || in == IN_UP) s->selected = (s->selected + s->count - 1) % s->count;
      if (in == IN_RIGHT || in == IN_DOWN) s->selected = (s->selected + 1) % s->count;
      // la lista sigue a la elegida
      if (s->selected < sliding(s) && s->selected < s->target) s->target = (float)s->selected;
      if (s->selected < sliding(s) && s->selected >= s->target + rows_of(s)) s->target = (float)(s->selected - rows_of(s) + 1);
      if (in == IN_ACCEPT) return s->selected;
    }
    // scroll va hacia target poco a poco, para que se vea deslizarse
    s->scroll += (s->target - s->scroll) * 0.25f;
    if (fabsf(s->target - s->scroll) < 0.01f) s->scroll = s->target;

    draw(s);
    backend_present(ui_backend, screen.px, screen.w, screen.h);
    frames++;
    usleep(16000);
  }
}

int ui_message(const char *logo, const char *title, const char *line1, const char *line2,
               ScreenTick tick, void *context) {
  Screen s;
  ui_reset(&s, logo, title);
  if (line1) ui_line(&s, colors.text, "%s", line1);
  if (line2) ui_line(&s, colors.text, "%s", line2);
  ui_item(&s, "OK");
  // un evento de tick no cierra el aviso
  int r;
  while ((r = ui_run(&s, tick, context)) >= SCREEN_EVENT) {}
  return r;
}
