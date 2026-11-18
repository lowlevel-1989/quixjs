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

// Asignar el mando (Controls del launcher). Heredado de tamatama
// (src/backend/sdl/mapper.c): se lee el mando crudo (SDL_Joystick, sin
// mapeo), una entrada cada vez, y lo pedido se guarda como una linea de
// SDL_GAMECONTROLLERCONFIG en controller.map, el formato de PortMaster:
//
//   GUID,nombre,dpup:h0.1,a:b0,-leftx:-a0,+leftx:+a0,...,platform:Linux
//
// Un eje se pide en sus dos direcciones (cada una es "-aN" o "+aN"): asi se
// sabe cual es y hacia donde va, aunque este invertido. Los ejes se miden
// desde donde estaban al empezar (un gatillo en reposo puede estar en -32768).

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "backend/backend.h"
#include "mapper.h"

#define AXIS_PRESSED  16000   // de 32767: mas alla, el eje cuenta como pulsado
#define AXIS_RELEASED 8000
#define MAX_AXES      16

static SDL_Joystick *joy = NULL;
static SDL_JoystickID joy_id = -1;
static Sint16 rest[MAX_AXES];   // los ejes al empezar
static char   held[16];         // la entrada pulsada, a la espera de que se suelte
static char   done[16];         // pulsada y soltada
static int    cancelled = 0;

void mapper_load(const char *path) {
  if (!path || !path[0]) return;
  FILE *f = fopen(path, "r");
  if (!f) return;
  fclose(f);
  int n = SDL_GameControllerAddMappingsFromFile(path);
  if (n > 0) printf("[controls] %s: %d controller(s)\n", path, n);
}

static void reset(void) {
  held[0] = done[0] = 0;
  cancelled = 0;
  if (!joy) return;
  int axes = SDL_JoystickNumAxes(joy);
  for (int i = 0; i < axes && i < MAX_AXES; i++) rest[i] = SDL_JoystickGetAxis(joy, i);
}

void mapper_event(const SDL_Event *e) {
  if (!joy) return;
  if (e->type == SDL_KEYDOWN && (e->key.keysym.sym == SDLK_ESCAPE || e->key.keysym.sym == SDLK_BACKSPACE)) {
    cancelled = 1;
    return;
  }
  if (e->type == SDL_JOYDEVICEREMOVED && e->jdevice.which == joy_id) {
    cancelled = 1;
    return;
  }
  char text[16] = "";
  int released = 0;
  switch (e->type) {
    case SDL_JOYBUTTONDOWN:
      if (e->jbutton.which == joy_id) snprintf(text, sizeof text, "b%d", e->jbutton.button);
      break;
    case SDL_JOYBUTTONUP:
      if (e->jbutton.which == joy_id && held[0] == 'b' && atoi(held + 1) == e->jbutton.button) released = 1;
      break;
    case SDL_JOYHATMOTION:
      if (e->jhat.which != joy_id) break;
      if (e->jhat.value == SDL_HAT_CENTERED) released = held[0] == 'h';
      else snprintf(text, sizeof text, "h%d.%d", e->jhat.hat, e->jhat.value);
      break;
    case SDL_JOYAXISMOTION: {
      if (e->jaxis.which != joy_id || e->jaxis.axis >= MAX_AXES) break;
      int d = e->jaxis.value - rest[e->jaxis.axis];
      if (abs(d) > AXIS_PRESSED) snprintf(text, sizeof text, "%ca%d", d < 0 ? '-' : '+', e->jaxis.axis);
      else if (abs(d) < AXIS_RELEASED && held[1] == 'a' && atoi(held + 2) == e->jaxis.axis) released = 1;
      break;
    }
  }
  if (held[0]) {
    if (released) {
      memcpy(done, held, sizeof done);
      held[0] = 0;
    }
  } else if (text[0] && !done[0]) {
    memcpy(held, text, sizeof held);
  }
}

int mapper_capture(int step, char *out, size_t room) {
  if (step == CAPTURE_START) {
    if (!joy) {
      if (SDL_NumJoysticks() < 1) return -1;
      joy = SDL_JoystickOpen(0);
      if (!joy) return -1;
      joy_id = SDL_JoystickInstanceID(joy);
      SDL_JoystickEventState(SDL_ENABLE);
    }
    reset();
    return 0;
  }
  if (step == CAPTURE_STOP) {
    if (joy) SDL_JoystickClose(joy);
    joy = NULL;
    joy_id = -1;
    return 0;
  }
  if (!joy || cancelled) return -1;
  if (!done[0]) return 0;
  snprintf(out, room, "%s", done);
  done[0] = 0;
  return 1;
}

// La linea de este mando en controller.map: las de los demas se conservan y
// la suya (el mismo GUID, el principio de la linea) se sustituye
int mapper_save(const char *path, const char *pairs) {
  if (!joy || !path || !path[0]) return -1;
  char guid[33], line[2048];
  SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(joy), guid, sizeof guid);
  const char *name = SDL_JoystickName(joy);
  snprintf(line, sizeof line, "%s,%s,%s,platform:%s", guid, name ? name : "Controller", pairs, SDL_GetPlatform());
  char tmp[1100];
  snprintf(tmp, sizeof tmp, "%s.tmp", path);
  FILE *out = fopen(tmp, "w");
  if (!out) return -1;
  FILE *in = fopen(path, "r");
  if (in) {
    char old[2048];
    while (fgets(old, sizeof old, in)) {
      if (strncmp(old, guid, strlen(guid)) != 0) fputs(old, out);
    }
    fclose(in);
  }
  fprintf(out, "%s\n", line);
  if (fclose(out) != 0 || rename(tmp, path) != 0) return -1;
  printf("[controls] %s: %s\n", path, line);
  return SDL_GameControllerAddMapping(line) < 0 ? -1 : 0;
}
