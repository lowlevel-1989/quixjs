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

// El teclado como un mando mas, como en tamatama
// (src/backend/sdl/mapper.c, keyboard.map). En Controls se asigna el teclado
// en vez del mando (la primera entrada decide) y se guarda en keyboard.map,
// junto a controller.map, una linea por entrada del mando estandar con el
// nombre de SDL de la tecla:
//
//   dpup=Up
//   a=Return
//   -leftx=A
//
// Con keyboard.map, las teclas asignadas entran por el mismo canal que los
// botones: el mando estandar de getGamepads (el primero). Como en tamatama,
// una tecla asignada es solo del mando: el juego ya no la recibe
// como tecla (Super Smiling Dungeon movia dos veces con la flecha: la suya y
// la del mando). Las demas siguen llegando como teclas.

#include <stdio.h>
#include <string.h>
#include "keypad.h"

// el mando estandar: el indice de la Gamepad API de cada boton (gatillos
// incluidos) y los ejes como dos mitades
static const char *buttons[GAMEPAD_BUTTONS] = {
  "a", "b", "x", "y", "leftshoulder", "rightshoulder", "lefttrigger", "righttrigger",
  "back", "start", "leftstick", "rightstick", "dpup", "dpdown", "dpleft", "dpright", "guide",
};
static const char *halves[GAMEPAD_AXES * 2] = {
  "-leftx", "+leftx", "-lefty", "+lefty", "-rightx", "+rightx", "-righty", "+righty",
};

#define TARGETS (GAMEPAD_BUTTONS + GAMEPAD_AXES * 2)
static SDL_Keycode keys[TARGETS];   // SDLK_UNKNOWN: sin tecla
static int held[TARGETS];
static int active = 0;

static int target_index(const char *name, size_t len) {
  for (int i = 0; i < GAMEPAD_BUTTONS; i++) {
    if (strlen(buttons[i]) == len && strncmp(buttons[i], name, len) == 0) return i;
  }
  for (int i = 0; i < GAMEPAD_AXES * 2; i++) {
    if (strlen(halves[i]) == len && strncmp(halves[i], name, len) == 0) return GAMEPAD_BUTTONS + i;
  }
  return -1;
}

static const char *target_name(int i) {
  return i < GAMEPAD_BUTTONS ? buttons[i] : halves[i - GAMEPAD_BUTTONS];
}

// keyboard.map en la carpeta de controller.map
static void map_path(const char *map, char *out, size_t room) {
  const char *slash = strrchr(map, '/');
  int dir = slash ? (int)(slash - map + 1) : 0;
  snprintf(out, room, "%.*skeyboard.map", dir, map);
}

void keypad_load(const char *map) {
  memset(keys, 0, sizeof keys);
  memset(held, 0, sizeof held);
  active = 0;
  if (!map || !map[0]) return;
  char path[1100], line[128];
  map_path(map, path, sizeof path);
  FILE *f = fopen(path, "r");
  if (!f) return;
  while (fgets(line, sizeof line, f)) {
    line[strcspn(line, "\r\n")] = 0;
    char *eq = strchr(line, '=');
    if (!eq) continue;
    int i = target_index(line, (size_t)(eq - line));
    SDL_Keycode k = SDL_GetKeyFromName(eq + 1);
    if (i < 0 || k == SDLK_UNKNOWN) continue;
    keys[i] = k;
    active = 1;
  }
  fclose(f);
  if (active) printf("[controls] %s: the keyboard is a controller\n", path);
}

// pairs como los de controller.map, con la tecla cruda ("a:k:Return"); las
// acciones de [controls.keys] (@) no: ya son teclas
int keypad_save(const char *map, const char *pairs) {
  char path[1100], tmp[1110];
  map_path(map, path, sizeof path);
  snprintf(tmp, sizeof tmp, "%s.tmp", path);
  FILE *out = fopen(tmp, "w");
  if (!out) return -1;
  for (const char *p = pairs; *p;) {
    const char *end = strchr(p, ',');
    size_t len = end ? (size_t)(end - p) : strlen(p);
    const char *raw = memchr(p, ':', len);
    if (p[0] != '@' && raw && (size_t)(raw - p) + 3 < len && raw[1] == 'k' && raw[2] == ':') {
      int i = target_index(p, (size_t)(raw - p));
      if (i >= 0) fprintf(out, "%s=%.*s\n", target_name(i), (int)(len - (size_t)(raw - p) - 3), raw + 3);
    }
    if (!end) break;
    p = end + 1;
  }
  if (fclose(out) != 0 || rename(tmp, path) != 0) return -1;
  printf("[controls] %s saved\n", path);
  keypad_load(map);
  return 0;
}

void keypad_event(const SDL_Event *e) {
  if (!active || (e->type != SDL_KEYDOWN && e->type != SDL_KEYUP)) return;
  for (int i = 0; i < TARGETS; i++) {
    if (keys[i] == e->key.keysym.sym) held[i] = e->type == SDL_KEYDOWN;
  }
}

int keypad_active(void) {
  return active;
}

int keypad_has(SDL_Keycode key) {
  for (int i = 0; active && i < TARGETS; i++) {
    if (keys[i] == key) return 1;
  }
  return 0;
}

// Las teclas pulsadas, sumadas al mando: el mismo canal que los botones (un
// boton cuenta si lo pulsa el mando o su tecla; un eje, la suma). Sin mando,
// el mando es solo el teclado.
void keypad_merge(GamepadState *pad) {
  if (!pad->connected) {
    memset(pad, 0, sizeof *pad);
    pad->connected = 1;
    snprintf(pad->id, sizeof pad->id, "Keyboard");
  }
  for (int b = 0; b < GAMEPAD_BUTTONS; b++) {
    if (!held[b]) continue;
    pad->pressed[b] = 1;
    pad->value[b] = 1.0f;
  }
  for (int a = 0; a < GAMEPAD_AXES; a++) {
    float v = pad->axes[a] + (float)(held[GAMEPAD_BUTTONS + a * 2 + 1] - held[GAMEPAD_BUTTONS + a * 2]);
    pad->axes[a] = v > 1 ? 1 : v < -1 ? -1 : v;
  }
}
