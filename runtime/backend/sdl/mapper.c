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

// controller.map al final de SDL_GAMECONTROLLERCONFIG. PortMaster (el
// script del port) exporta esa variable con su asignacion del mando, y SDL
// le da mas prioridad que a la que agrega el programa
// (SDL_GameControllerAddMapping): con el mismo mando ganaba la de PortMaster
// y lo asignado en Controls no se usaba. En la misma variable tienen la
// misma prioridad y gana la ultima linea de cada mando: la de controller.map.
// Se llama antes de iniciar los mandos de SDL, que es cuando la lee.
static void keys_load(const char *map);   // controller.keys (abajo)

#define MAX_FILE_GUIDS 8
static char file_guids[MAX_FILE_GUIDS][33];   // los mandos de controller.map
static int  file_guid_n = 0;

int mapper_from_file(SDL_JoystickGUID guid) {
  char g[33];
  SDL_JoystickGetGUIDString(guid, g, sizeof g);
  for (int i = 0; i < file_guid_n; i++) {
    if (strcmp(file_guids[i], g) == 0) return 1;
  }
  return 0;
}

void mapper_load(const char *path) {
  file_guid_n = 0;
  keys_load(path);
  if (!path || !path[0]) return;
  FILE *f = fopen(path, "r");
  if (!f) return;
  const char *env = getenv("SDL_GAMECONTROLLERCONFIG");
  size_t room = (env ? strlen(env) : 0) + 2;
  char line[2048];
  long start = ftell(f);
  while (fgets(line, sizeof line, f)) room += strlen(line) + 1;
  fseek(f, start, SEEK_SET);
  char *all = malloc(room);
  if (!all) {
    fclose(f);
    return;
  }
  all[0] = 0;
  if (env && env[0]) {
    strcat(all, env);
    if (all[strlen(all) - 1] != '\n') strcat(all, "\n");
  }
  int n = 0;
  while (fgets(line, sizeof line, f)) {
    if (line[0] == '#' || strlen(line) < 34) continue;   // una linea: GUID,nombre,...
    strcat(all, line);
    if (all[strlen(all) - 1] != '\n') strcat(all, "\n");
    if (file_guid_n < MAX_FILE_GUIDS) {
      memcpy(file_guids[file_guid_n], line, 32);
      file_guids[file_guid_n++][32] = 0;
    }
    n++;
  }
  fclose(f);
  setenv("SDL_GAMECONTROLLERCONFIG", all, 1);
  free(all);
  if (n > 0) printf("[controls] %s: %d controller(s)\n", path, n);
}

// controller.keys, junto a controller.map: el boton crudo de cada accion de
// [controls.keys], por mando ("GUID,Torch:b11,Phase:+a5"). Se lee del mando
// crudo (SDL_Joystick), como al asignarlo: vale tambien un boton que el
// mapeo estandar no tiene
#define MAX_KEY_BINDS 32
static struct {
  char guid[33];
  char name[48];
  char raw[16];
  int  down;
} binds[MAX_KEY_BINDS];
static int binds_n = 0;

// controller.map -> controller.keys
static void keys_path(const char *map, char *out, size_t room) {
  const char *slash = strrchr(map, '/');
  if (slash) snprintf(out, room, "%.*s/controller.keys", (int)(slash - map), map);
  else snprintf(out, room, "controller.keys");
}

static void keys_load(const char *map) {
  binds_n = 0;
  if (!map || !map[0]) return;
  char path[1100], line[1024];
  keys_path(map, path, sizeof path);
  FILE *f = fopen(path, "r");
  if (!f) return;
  while (fgets(line, sizeof line, f)) {
    line[strcspn(line, "\r\n")] = 0;
    if (line[0] == '#' || strlen(line) < 33 || line[32] != ',') continue;
    char *p = line + 33;
    while (*p && binds_n < MAX_KEY_BINDS) {
      char *end = strchr(p, ',');
      if (end) *end = 0;
      char *colon = strrchr(p, ':');
      if (colon && colon > p) {
        memcpy(binds[binds_n].guid, line, 32);
        binds[binds_n].guid[32] = 0;
        snprintf(binds[binds_n].name, sizeof binds[0].name, "%.*s", (int)(colon - p), p);
        snprintf(binds[binds_n].raw, sizeof binds[0].raw, "%s", colon + 1);
        binds[binds_n].down = 0;
        binds_n++;
      }
      if (!end) break;
      p = end + 1;
    }
  }
  fclose(f);
  if (binds_n > 0) printf("[controls] %s: %d key(s)\n", path, binds_n);
}

void mapper_key_event(const SDL_Event *e, void (*cb)(void *ctx, const char *name, int down), void *ctx) {
  if (joy || binds_n == 0) return;   // mientras se asigna, nada
  SDL_JoystickID which;
  switch (e->type) {
    case SDL_JOYBUTTONDOWN: case SDL_JOYBUTTONUP: which = e->jbutton.which; break;
    case SDL_JOYHATMOTION: which = e->jhat.which; break;
    case SDL_JOYAXISMOTION: which = e->jaxis.which; break;
    default: return;
  }
  SDL_Joystick *j = SDL_JoystickFromInstanceID(which);
  if (!j) return;
  char g[33];
  SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(j), g, sizeof g);
  for (int i = 0; i < binds_n; i++) {
    if (strcmp(binds[i].guid, g) != 0) continue;
    const char *r = binds[i].raw;
    int down = binds[i].down;
    if (r[0] == 'b' && (e->type == SDL_JOYBUTTONDOWN || e->type == SDL_JOYBUTTONUP)) {
      if (atoi(r + 1) != e->jbutton.button) continue;
      down = e->type == SDL_JOYBUTTONDOWN;
    } else if (r[0] == 'h' && e->type == SDL_JOYHATMOTION) {
      int hat = atoi(r + 1);
      const char *dot = strchr(r, '.');
      if (hat != e->jhat.hat || !dot) continue;
      down = (e->jhat.value & atoi(dot + 1)) != 0;
    } else if ((r[0] == '+' || r[0] == '-') && r[1] == 'a' && e->type == SDL_JOYAXISMOTION) {
      if (atoi(r + 2) != e->jaxis.axis) continue;
      int v = r[0] == '-' ? -e->jaxis.value : e->jaxis.value;
      if (v > AXIS_PRESSED) down = 1;
      else if (v < AXIS_RELEASED) down = 0;
    } else {
      continue;
    }
    if (down == binds[i].down) continue;
    binds[i].down = down;
    cb(ctx, binds[i].name, down);
  }
}

// Despues de guardar en Controls: los mandos de SDL se reinician para que
// vuelvan a leer la variable (con la linea nueva). El mando crudo de
// mapper_capture ya tiene que estar cerrado (CAPTURE_STOP).
void mapper_reload(const char *path) {
  mapper_load(path);
  SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
  SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
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

// 1 si en pairs ("a:b0,b:b1,...") hay una entrada con ese nombre (lado 0) o
// esa entrada cruda (lado 1)
static int pairs_have(const char *pairs, const char *what, size_t len, int side) {
  const char *p = pairs;
  while (*p) {
    const char *end = strchr(p, ',');
    size_t n = end ? (size_t)(end - p) : strlen(p);
    const char *colon = memchr(p, ':', n);
    if (colon) {
      const char *s = side ? colon + 1 : p;
      size_t sn = side ? (size_t)(p + n - colon - 1) : (size_t)(colon - p);
      if (sn == len && strncmp(s, what, len) == 0) return 1;
    }
    if (!end) break;
    p = end + 1;
  }
  return 0;
}

// La linea de este mando en controller.keys (las acciones "@Torch:b11" de
// pairs), sustituyendo la que tuviera
static int keys_save(const char *map, const char *guid, const char *pairs) {
  char path[1100], tmp[1110], line[1024];
  size_t n = (size_t)snprintf(line, sizeof line, "%s", guid);
  for (const char *p = pairs; (p = strchr(p, '@')) != NULL; p++) {
    const char *end = strchr(p, ',');
    size_t len = end ? (size_t)(end - p - 1) : strlen(p + 1);
    if (n < sizeof line) n += (size_t)snprintf(line + n, sizeof line - n, ",%.*s", (int)len, p + 1);
  }
  keys_path(map, path, sizeof path);
  snprintf(tmp, sizeof tmp, "%s.tmp", path);
  FILE *out = fopen(tmp, "w");
  if (!out) return -1;
  FILE *in = fopen(path, "r");
  if (in) {
    char old[1024];
    while (fgets(old, sizeof old, in)) {
      if (strncmp(old, guid, strlen(guid)) != 0) fputs(old, out);
    }
    fclose(in);
  }
  if (strchr(line, ',')) fprintf(out, "%s\n", line);
  if (fclose(out) != 0 || rename(tmp, path) != 0) return -1;
  printf("[controls] %s: %s\n", path, line);
  return 0;
}

// La linea de este mando en controller.map: las de los demas se conservan y
// la suya (el mismo GUID, el principio de la linea) se sustituye
int mapper_save(const char *path, const char *pairs_in) {
  if (!joy || !path || !path[0]) return -1;
  char guid[33], line[2048], all[1600], pairs[1600];
  // las acciones de [controls.keys] (@) van a controller.keys
  size_t pn = 0;
  pairs[0] = 0;
  for (const char *p = pairs_in; *p;) {
    const char *end = strchr(p, ',');
    size_t len = end ? (size_t)(end - p) : strlen(p);
    if (p[0] != '@' && len > 0 && pn < sizeof pairs) pn += (size_t)snprintf(pairs + pn, sizeof pairs - pn, "%s%.*s", pn ? "," : "", (int)len, p);
    if (!end) break;
    p = end + 1;
  }
  SDL_JoystickGUID id = SDL_JoystickGetGUID(joy);
  SDL_JoystickGetGUIDString(id, guid, sizeof guid);
  const char *name = SDL_JoystickName(joy);
  // lo que no se pidio (un port con [controls.buttons] pide solo algunos)
  // sigue como estaba, si su entrada cruda no la usa ya otro
  size_t n = (size_t)snprintf(all, sizeof all, "%s", pairs);
  char *old = SDL_GameControllerMappingForGUID(id);
  if (old) {
    const char *p = strchr(old, ',');
    if (p) p = strchr(p + 1, ',');
    while (p && *p && n < sizeof all) {
      p++;
      const char *end = strchr(p, ',');
      size_t len = end ? (size_t)(end - p) : strlen(p);
      const char *colon = memchr(p, ':', len);
      if (colon && len > 0) {
        size_t kn = (size_t)(colon - p), vn = len - kn - 1;
        if (!(kn == 8 && strncmp(p, "platform", 8) == 0) && !pairs_have(pairs, p, kn, 0) &&
            !pairs_have(pairs, colon + 1, vn, 1)) {
          n += (size_t)snprintf(all + n, sizeof all - n, "%s%.*s", n ? "," : "", (int)len, p);
        }
      }
      p = end;
    }
    SDL_free(old);
  }
  snprintf(line, sizeof line, "%s,%s,%s,platform:%s", guid, name ? name : "Controller", all, SDL_GetPlatform());
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
  if (strchr(pairs_in, '@') || binds_n) return keys_save(path, guid, pairs_in);
  return 0;
}
