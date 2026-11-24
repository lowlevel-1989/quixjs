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

// runtime/launcher/ui/ui.c: las pantallas propias de quixjs (el launcher,
// Controls, Transfer Save, "preparing the game"). Heredado de tamatama
// (src/shared/screen.c): la misma lista que se desliza, las mismas flechas y
// la misma barra, la misma letra (stb_easy_font) y las mismas medidas; aqui
// con el mando del backend y los colores de [launcher].
//
// Este archivo abre y cierra las pantallas, las monta (ui_reset, ui_line,
// ui_item) y las corre (ui_run). Las demas partes: draw.c (dibujar) e
// input.c (lo que se pulsa); lo que comparten, en common.h.

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

UiState ui_st = { .vw = 640, .k = 1.0 };

void ui_open(Backend *backend, int w, int h, const UiColors *c) {
  ui_st.backend = backend;
  if (w <= 0) w = 640;
  if (h <= 0) h = 480;
  surface_init(&ui_st.screen, w, h);
  ui_st.colors = *c;
  ui_st.k = h / (double)VH;
  ui_st.vw = (int)(w / ui_st.k);
}

void ui_close(void) {
  surface_free(&ui_st.screen);
  ui_st.backend = NULL;
}

const UiColors *ui_colors(void) { return &ui_st.colors; }
const Surface *ui_surface(void) { return &ui_st.screen; }
long ui_frames(void) { return ui_st.frames; }

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
  ui_line(s, ui_st.colors.highlight, "%s", signature);
  s->line_scale[s->line_count - 1] = 4;
}

void ui_item(Screen *s, const char *text) {
  if (s->count == UI_ITEMS) return;
  s->item_rgb[s->count] = 0;
  s->item_badge[s->count] = 0;
  s->item_hollow[s->count] = 0;
  snprintf(s->items[s->count++], sizeof s->items[0], "%s", text);
}

int ui_run(Screen *s, ScreenTick tick, void *context) {
  ui_input_clear();   // lo pulsado antes no cuenta
  for (;;) {
    backend_process_events(ui_st.backend);
    ui_read_input();
    if (backend_should_close(ui_st.backend) || ui_quit_code() >= 0) return SCREEN_QUIT;
    if (tick) {
      int event = tick(s, context);
      if (event) return SCREEN_EVENT + event;
    }
    // tick puede haber cambiado las opciones
    if (s->selected >= s->count) s->selected = s->count ? s->count - 1 : 0;
    s->target = ui_clampf(s->target, 0, ui_max_scroll(s));

    for (int in; (in = ui_next_input()) != IN_NONE;) {
      if (in == IN_BACK && s->can_back) return SCREEN_BACK;
      if (!s->count) continue;
      if (in == IN_LEFT || in == IN_UP) s->selected = (s->selected + s->count - 1) % s->count;
      if (in == IN_RIGHT || in == IN_DOWN) s->selected = (s->selected + 1) % s->count;
      // la lista sigue a la elegida
      if (s->selected < ui_sliding(s) && s->selected < s->target) s->target = (float)s->selected;
      if (s->selected < ui_sliding(s) && s->selected >= s->target + ui_rows_of(s)) s->target = (float)(s->selected - ui_rows_of(s) + 1);
      if (in == IN_ACCEPT) return s->selected;
    }
    // scroll va hacia target poco a poco, para que se vea deslizarse
    s->scroll += (s->target - s->scroll) * 0.25f;
    if (fabsf(s->target - s->scroll) < 0.01f) s->scroll = s->target;

    ui_draw(s);
    backend_present(ui_st.backend, ui_st.screen.px, ui_st.screen.w, ui_st.screen.h);
    ui_st.frames++;
    usleep(16000);
  }
}

int ui_message(const char *logo, const char *title, const char *line1, const char *line2,
               ScreenTick tick, void *context) {
  Screen s;
  ui_reset(&s, logo, title);
  if (line1) ui_line(&s, ui_st.colors.text, "%s", line1);
  if (line2) ui_line(&s, ui_st.colors.text, "%s", line2);
  ui_item(&s, "OK");
  // un evento de tick no cierra el aviso
  int r;
  while ((r = ui_run(&s, tick, context)) >= SCREEN_EVENT) {}
  return r;
}
