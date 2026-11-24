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

// runtime/launcher/ui/input.c: lo que se pulsa en las pantallas de ui, de una
// en una: el teclado, la cruceta, el stick y los botones del mando, y
// --script (que tambien maneja estas pantallas). Y el codigo de salir que
// pida el script.

#include <string.h>
#include "core/debug/input_script.h"
#include "common.h"

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
  return &ui_st.screen;
}

void ui_read_input(void) {
  // --script tambien maneja las pantallas propias: sus ui_st.frames cuentan aqui
  if (input_script_active()) {
    int code = 0;
    if (input_script_run(ui_st.frames, ui_st.backend, script_shot, NULL, &code)) quit_code = code;
  }
  InputEvent ev;
  while (backend_pop_input(ui_st.backend, &ev)) {
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
  int n = backend_get_gamepads(ui_st.backend, pads, GAMEPAD_MAX);
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

int ui_next_input(void) {
  if (!in_len) return IN_NONE;
  int in = in_queue[0];
  memmove(in_queue, in_queue + 1, sizeof(int) * (size_t)--in_len);
  return in;
}

int ui_quit_code(void) { return quit_code; }

void ui_input_clear(void) { in_len = 0; }
