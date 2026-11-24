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
// cuadro que muestra ampliado lo que tiene debajo. Start con ella abierta la
// fija: se queda donde esta y el mando vuelve al juego (menos Start, que la
// desfija). Aqui todo lo suyo:
// abrirla, fijarla y cerrarla, sus botones, moverla, el mando que el juego
// deja de ver, el cuadro y su icono. input.c, gamepads.c y draw.c solo la
// llaman.
// Parte de backend/sdl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"

// El tamano mas grande de la lupa: toda la pantalla, tambien las franjas
// negras que deja el juego (en una de 720x720, el juego es 720x405)
#define MAG_SCREEN 2.0f

// Los botones que estaban pulsados al fijar la lupa: el juego no los ve hasta que se sueltan, para que
// fijarla no le llegue como una pulsacion
static unsigned char pin_mask[GAMEPAD_MAX][GAMEPAD_BUTTONS];
static int pin_capture = 0;

// la lupa (Select + L1, o su boton de Controls): abrirla o cerrarla; fija,
// la desfija (el mando vuelve a la lupa, para moverla) y la siguiente vez la
// cierra. El zoom, el tamano y la posicion de la ultima vez se recuerdan
// mientras el juego sigue abierto (en memoria, no en disco): la primera vez,
// x2, la mitad de la pantalla y en el centro (A la vuelve al centro)
static void magnifier_toggle(Backend* backend) {
  static float last_zoom = 2, last_size = 0.5f, last_x = 0.5f, last_y = 0.5f;
  if (backend->magnify > 1 && backend->mag_pinned) {
    backend->mag_pinned = 0;
    printf("[magnifier] unpinned\n");
    return;
  }
  if (backend->magnify > 1) {
    last_zoom = backend->magnify;
    last_size = backend->mag_size > 0 ? backend->mag_size : 0.5f;
    last_x = backend->mag_x;
    last_y = backend->mag_y;
    backend->magnify = 0;
    backend->mag_pinned = 0;
  } else {
    backend->magnify = last_zoom;
    backend->mag_size = last_size;
    backend->mag_x = last_x;
    backend->mag_y = last_y;
  }
  printf("[magnifier] %s\n", backend->magnify > 1 ? "on" : "off");
}

// Start con la lupa abierta: fijarla, en su posicion, tamano y zoom; fija,
// desfijarla (el mando vuelve a la lupa)
static void magnifier_pin(Backend* backend) {
  backend->mag_pinned = !backend->mag_pinned;
  if (backend->mag_pinned) pin_capture = 1;
  printf("[magnifier] %s\n", backend->mag_pinned ? "pinned" : "unpinned");
}

// Acercar (+1) o alejar (-1) la lupa, por estos pasos: de x1.25 (casi el
// juego tal cual, para un texto que solo se lee un poco mejor) a x6
static void magnifier_zoom(Backend* backend, int d) {
  static const float steps[] = { 1.25f, 1.5f, 2, 3, 4, 5, 6 };
  int n = (int)(sizeof steps / sizeof steps[0]), k = 0;
  while (k < n - 1 && steps[k] < backend->magnify - 0.01f) k++;
  k += d;
  if (k >= 0 && k < n) backend->magnify = steps[k];
}

// Con la lupa abierta: R1 y R2 acercan, L1 y L2 alejan (R2 y L2 son ejes:
// input.c llama a magnifier_zoom), A la vuelve al
// centro, Y agranda el cuadro y X lo achica (un tercio, la mitad, dos
// tercios, todo el juego o toda la pantalla, MAG_SCREEN), Start la fija o la desfija, B la cierra. Fija,
// los demas botones son del juego
static void magnifier_button(Backend* backend, int b) {
  static const float sizes[] = { 1.0f / 3, 0.5f, 2.0f / 3, 1.0f, MAG_SCREEN };
  if (b == SDL_CONTROLLER_BUTTON_START) {
    magnifier_pin(backend);
    return;
  }
  if (backend->mag_pinned) return;
  int k = 0;
  while (k < 4 && sizes[k] < backend->mag_size - 0.01f) k++;
  if (b == SDL_CONTROLLER_BUTTON_A) backend->mag_x = backend->mag_y = 0.5f;
  else if (b == SDL_CONTROLLER_BUTTON_Y && k < 4) backend->mag_size = sizes[k + 1];
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
  if (backend->magnify <= 1 || backend->mag_pinned) return;
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

// Con la lupa abierta el mando es de la lupa: el juego lo ve quieto. Fija,
// el juego lo ve entero, menos Start (el de la lupa) y los botones que la
// fijaron mientras sigan pulsados (pin_mask)
static void magnifier_hide_pads(Backend* backend, GamepadState *pads, int n) {
  if (backend->magnify <= 1) return;
  if (backend->mag_pinned) {
    for (int i = 0; i < n; i++) {
      for (int b = 0; b < GAMEPAD_BUTTONS; b++) {
        if (pin_capture && pads[i].pressed[b]) pin_mask[i][b] = 1;
        if (b == 9) pin_mask[i][b] = 1;   // Start (9 en el mapeo standard): de la lupa
        if (!pin_mask[i][b]) continue;
        if (!pads[i].pressed[b]) pin_mask[i][b] = 0;
        pads[i].pressed[b] = 0;
        pads[i].value[b] = 0;
      }
    }
    pin_capture = 0;
    return;
  }
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
// de pixeles blancos sobre un recuadro oscuro, a la escala de la ventana;
// fija, con un candado a la derecha (el recuadro se alarga)
static void magnifier_icon(Backend* backend, int wh) {
  static const char *icon[] = {
    "..XXXX......XXX..", ".X....X....X...X.", "X......X...X...X.", "X......X..XXXXXXX",
    "X......X..XXX.XXX", "X......X..XXX.XXX", ".X....X...XXXXXXX", "..XXXXXX.........",
    "......XXX........", ".......XXX.......",
  };
  int cols = backend->mag_pinned ? 17 : 10;
  int px = wh / 160 > 2 ? wh / 160 : 2, m = 8, size = 10 * px;
  qgl.Enable(GL_SCISSOR_TEST);
  qgl.Scissor(m - px * 2, wh - (m + size + px * 2), cols * px + px * 4, size + px * 4);
  qgl.ClearColor(0.1f, 0.1f, 0.12f, 1);
  qgl.Clear(GL_COLOR_BUFFER_BIT);
  qgl.ClearColor(1, 1, 1, 1);
  for (int r = 0; r < 10; r++) {
    for (int c = 0; c < cols; c++) {
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
static void magnifier_lens(Backend* backend, int *pdx, int *pdy, int *pdw, int *pdh, int ww, int wh) {
  int dx = *pdx, dy = *pdy, dw = *pdw, dh = *pdh;
  // la lupa: un cuadro de mag_size del juego centrado en (mag_x, mag_y),
  // con lo que tiene debajo magnify veces mas grande. Puede salirse de
  // la pantalla: lo que sobra no se dibuja, y asi llega a los bordes.
  // MAG_SCREEN: el cuadro es toda la pantalla, con ese punto en el centro
  float size = backend->mag_size > 0 ? backend->mag_size : 0.5f;
  int lw = (int)(dw * size), lh = (int)(dh * size);
  int cx = dx + (int)(backend->mag_x * dw), cy = dy + (int)(backend->mag_y * dh);
  if (size >= MAG_SCREEN - 0.01f) {
    lw = ww;
    lh = wh;
    cx = ww / 2;
    cy = wh / 2;
  }
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
