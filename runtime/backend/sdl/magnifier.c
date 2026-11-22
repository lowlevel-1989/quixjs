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

// La lupa, en todos los juegos: Select + L1 (o su boton de Controls) abre un
// cuadro que muestra ampliado lo que tiene debajo. Aqui todo lo suyo: abrirla
// y cerrarla, sus botones, moverla, el mando que el juego deja de ver, el
// cuadro y su icono. input.c, gamepads.c y draw.c solo la llaman.
// Parte de backend/sdl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

// la lupa: el cuadro en el centro, x2; o cerrarla
static void magnifier_toggle(Backend* backend) {
  backend->magnify = backend->magnify > 1 ? 0 : 2;
  backend->mag_x = backend->mag_y = 0.5f;
  backend->mag_size = 0.5f;
  printf("[magnifier] %s\n", backend->magnify > 1 ? "on" : "off");
}

// Acercar (+1) o alejar (-1) la lupa, de x2 a x6
static void magnifier_zoom(Backend* backend, int d) {
  int m = backend->magnify + d;
  if (m >= 2 && m <= 6) backend->magnify = m;
}

// Con la lupa abierta: R1 y R2 acercan, L1 y L2 alejan (R2 y L2 son ejes:
// input.c llama a magnifier_zoom), A la vuelve al
// centro, Y agranda el cuadro y X lo achica (un tercio, la mitad, dos
// tercios o todo el juego), B la cierra
static void magnifier_button(Backend* backend, int b) {
  static const float sizes[] = { 1.0f / 3, 0.5f, 2.0f / 3, 1.0f };
  int k = 0;
  while (k < 3 && sizes[k] < backend->mag_size - 0.01f) k++;
  if (b == SDL_CONTROLLER_BUTTON_A) backend->mag_x = backend->mag_y = 0.5f;
  else if (b == SDL_CONTROLLER_BUTTON_Y && k < 3) backend->mag_size = sizes[k + 1];
  else if (b == SDL_CONTROLLER_BUTTON_X && k > 0) backend->mag_size = sizes[k - 1];
  else if (b == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER) magnifier_zoom(backend, 1);
  else if (b == SDL_CONTROLLER_BUTTON_LEFTSHOULDER) magnifier_zoom(backend, -1);
  else if (b == SDL_CONTROLLER_BUTTON_B) magnifier_toggle(backend);
}

// Con la lupa: las dos palancas y la cruceta mueven el punto que se ve (en
// cada imagen; el juego no recibe el mando mientras tanto, gamepads.c)
static float stick(SDL_GameController *gc, SDL_GameControllerAxis a) {
  float v = SDL_GameControllerGetAxis(gc, a) / 32767.0f;
  return v > -0.2f && v < 0.2f ? 0 : v;
}
static void magnifier_pan(Backend* backend, SDLPlatformData* data) {
  if (backend->magnify <= 1) return;
  for (int i = 0; i < GAMEPAD_MAX; i++) {
    SDL_GameController *gc = data->pads[i];
    if (!gc) continue;
    float ax = stick(gc, SDL_CONTROLLER_AXIS_LEFTX) + stick(gc, SDL_CONTROLLER_AXIS_RIGHTX);
    float ay = stick(gc, SDL_CONTROLLER_AXIS_LEFTY) + stick(gc, SDL_CONTROLLER_AXIS_RIGHTY);
    ax += SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) - SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    ay += SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_DOWN) - SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_UP);
    backend->mag_x += ax * 0.012f;
    backend->mag_y += ay * 0.012f;
  }
  // el centro del cuadro, dentro del juego (el cuadro puede salirse)
  if (backend->mag_x < 0) backend->mag_x = 0;
  if (backend->mag_x > 1) backend->mag_x = 1;
  if (backend->mag_y < 0) backend->mag_y = 0;
  if (backend->mag_y > 1) backend->mag_y = 1;
}

// Con la lupa abierta el mando es de la lupa: el juego lo ve quieto
static void magnifier_hide_pads(Backend* backend, GamepadState *pads, int n) {
  if (backend->magnify <= 1) return;
  for (int i = 0; i < n; i++) {
    memset(pads[i].axes, 0, sizeof pads[i].axes);
    memset(pads[i].pressed, 0, sizeof pads[i].pressed);
    memset(pads[i].value, 0, sizeof pads[i].value);
  }
}

// El marco de la lupa (x, y, w, h en pixeles de la ventana, desde arriba):
// un borde blanco con otro oscuro por fuera, y despues el recorte (scissor)
// al interior, donde se dibuja el juego ampliado
static void magnifier_frame(int x, int y, int w, int h, int wh) {
  qgl.Enable(GL_SCISSOR_TEST);
  for (int k = 0; k < 2; k++) {
    int b = k == 0 ? 4 : 2;   // primero el oscuro (4 px), encima el blanco (2)
    float c = k == 0 ? 0.0f : 1.0f;
    qgl.Scissor(x - b, wh - (y + h + b), w + 2 * b, h + 2 * b);
    qgl.ClearColor(c, c, c, 1);
    qgl.Clear(GL_COLOR_BUFFER_BIT);
  }
  qgl.Scissor(x, wh - (y + h), w, h);
  qgl.ClearColor(0, 0, 0, 1);
  qgl.Clear(GL_COLOR_BUFFER_BIT);
}

// El icono de la lupa abierta, arriba a la izquierda de la ventana: asi se
// sabe que esta abierta aunque el cuadro este fuera de la pantalla. Una lupa
// de pixeles blancos sobre un recuadro oscuro, a la escala de la ventana
static void magnifier_icon(int wh) {
  static const char *icon[] = {
    "..XXXX....", ".X....X...", "X......X..", "X......X..", "X......X..",
    "X......X..", ".X....X...", "..XXXXXX..", "......XXX.", ".......XXX",
  };
  int px = wh / 160 > 2 ? wh / 160 : 2, m = 8, size = 10 * px;
  qgl.Enable(GL_SCISSOR_TEST);
  qgl.Scissor(m - px * 2, wh - (m + size + px * 2), size + px * 4, size + px * 4);
  qgl.ClearColor(0.1f, 0.1f, 0.12f, 1);
  qgl.Clear(GL_COLOR_BUFFER_BIT);
  qgl.ClearColor(1, 1, 1, 1);
  for (int r = 0; r < 10; r++) {
    for (int c = 0; c < 10; c++) {
      if (icon[r][c] != 'X') continue;
      qgl.Scissor(m + c * px, wh - (m + (r + 1) * px), px, px);
      qgl.Clear(GL_COLOR_BUFFER_BIT);
    }
  }
  qgl.ClearColor(0, 0, 0, 1);
  qgl.Disable(GL_SCISSOR_TEST);
}

// La segunda pasada de draw_layers: dibuja el marco del cuadro y cambia el
// rectangulo del juego (dx, dy, dw, dh) por el ampliado, que se dibuja
// recortado dentro del cuadro
static void magnifier_lens(Backend* backend, int *pdx, int *pdy, int *pdw, int *pdh, int wh) {
  int dx = *pdx, dy = *pdy, dw = *pdw, dh = *pdh;
  // la lupa: un cuadro de mag_size del juego centrado en (mag_x, mag_y),
  // con lo que tiene debajo magnify veces mas grande. Puede salirse de
  // la pantalla: lo que sobra no se dibuja, y asi llega a los bordes
  float size = backend->mag_size > 0 ? backend->mag_size : 0.5f;
  int lw = (int)(dw * size), lh = (int)(dh * size);
  int cx = dx + (int)(backend->mag_x * dw), cy = dy + (int)(backend->mag_y * dh);
  magnifier_frame(cx - lw / 2, cy - lh / 2, lw, lh, wh);
  dw = (int)(dw * backend->magnify + 0.5);
  dh = (int)(dh * backend->magnify + 0.5);
  dx = (int)(cx - backend->mag_x * dw);
  dy = (int)(cy - backend->mag_y * dh);
  *pdx = dx;
  *pdy = dy;
  *pdw = dw;
  *pdh = dh;
}
