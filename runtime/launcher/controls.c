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

// runtime/launcher/controls.c: la pantalla Controls del launcher:
// asignar el mando entrada a entrada (el plan de lo que se pide y la captura
// de cada una) y guardarlo en controller.map.

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "common.h"


// Controls: asignar el mando, entrada a entrada, como en tamatama. Se piden
// todas las del mando estandar (las que lee el juego con la Gamepad API),
// tambien cada palanca en sus cuatro direcciones. Start va el primero: desde
// L1, lo que el mando no tenga se salta con el. Se guarda en controller.map
// (backend_save_controller) y se aplica ya.
static const struct {
  const char *sdl, *label;
  int optional;
} wanted[] = {
  {"start", "Start", 0},            {"back", "Select", 0},
  {"dpup", "D-pad up", 0},          {"dpdown", "D-pad down", 0},
  {"dpleft", "D-pad left", 0},      {"dpright", "D-pad right", 0},
  {"a", "A", 0},                    {"b", "B", 0},
  {"x", "X", 0},                    {"y", "Y", 0},
  {"leftshoulder", "L1", 1},        {"rightshoulder", "R1", 1},
  {"lefttrigger", "L2", 1},         {"righttrigger", "R2", 1},
  {"-lefty", "Left stick up", 1},   {"+lefty", "Left stick down", 1},
  {"-leftx", "Left stick left", 1}, {"+leftx", "Left stick right", 1},
  {"leftstick", "Left stick press (L3)", 1},
  {"-righty", "Right stick up", 1},   {"+righty", "Right stick down", 1},
  {"-rightx", "Right stick left", 1}, {"+rightx", "Right stick right", 1},
  {"rightstick", "Right stick press (R3)", 1},
};
#define WANTED (int)(sizeof wanted / sizeof wanted[0])
#define START_STEP 0

// lo que se pide: las entradas de wanted y, detras, las acciones de
// [controls.keys] (indice WANTED + k)
#define PLAN_MAX (WANTED + 8)
typedef struct {
  Backend *backend;
  const ControlsConfig *cc;
  int      step, count;
  int      idx[PLAN_MAX];          // lo que se pide, en orden
  char     label[PLAN_MAX][72];    // como se pide: "Confirm (A)", "Torch"
  char     raw[PLAN_MAX][24];      // "" = saltada; "k:Return": una tecla
} Capture;

// las acciones de [controls.keys] siempre se pueden saltar
static int plan_optional(const Capture *c, int step) {
  return c->idx[step] >= WANTED || wanted[c->idx[step]].optional;
}

// en controller.map ("a") o, con @ delante, en controller.keys ("@Torch")
static void plan_target(const Capture *c, int step, char *out, size_t room) {
  int i = c->idx[step];
  if (i >= WANTED) snprintf(out, room, "@%s", c->cc->key_button[i - WANTED]);
  else snprintf(out, room, "%s", wanted[i].sdl);
}

// Un boton de [controls.buttons] en wanted: su nombre de SDL o el de la
// etiqueta (select, l1, r2, l3...); -1 si no existe
static int wanted_index(const char *button) {
  static const char *alias[][2] = {
    {"select", "back"},         {"l1", "leftshoulder"}, {"r1", "rightshoulder"},
    {"l2", "lefttrigger"},      {"r2", "righttrigger"}, {"l3", "leftstick"},
    {"r3", "rightstick"},
  };
  for (size_t i = 0; i < sizeof alias / sizeof alias[0]; i++) {
    if (strcasecmp(button, alias[i][0]) == 0) button = alias[i][1];
  }
  for (int i = 0; i < WANTED; i++) {
    if (strcasecmp(button, wanted[i].sdl) == 0) return i;
  }
  return -1;
}

static int is_stick(int i) {
  return strstr(wanted[i].sdl, "stick") || strstr(wanted[i].sdl, "leftx") || strstr(wanted[i].sdl, "lefty") ||
         strstr(wanted[i].sdl, "rightx") || strstr(wanted[i].sdl, "righty");
}

static void plan_add(Capture *c, int i, const char *name) {
  for (int k = 0; k < c->count; k++) {
    if (c->idx[k] == i) return;
  }
  c->idx[c->count] = i;
  if (i >= WANTED) snprintf(c->label[c->count], sizeof c->label[0], "%s", name);
  else if (name && name[0]) snprintf(c->label[c->count], sizeof c->label[0], "%s (%s)", name, wanted[i].label);
  else snprintf(c->label[c->count], sizeof c->label[0], "%s", wanted[i].label);
  c->count++;
}

// Lo que se pide: siempre todo el mando estandar, en su orden (Start, con el
// se saltan los que no tenga el mando; Select; la cruceta; A, B, X, Y; L1,
// R1, L2, R2...): los combos de quixjs (Select + Start, + L1, + R1, + R2,
// + X, + Y, + izquierda / derecha) usan esos botones, y en algunas consolas
// estan en otro orden. Los que nombra [controls.buttons] salen con su
// nombre ("Dash (X)"). Al final, las acciones de [controls.keys] y las dos
// palancas
static void capture_plan(Capture *c, const ControlsConfig *cc) {
  c->count = 0;
  c->cc = cc;
  const char *named[WANTED] = {0};
  for (int k = 0; k < cc->names_count; k++) {
    int i = wanted_index(cc->button[k]);
    if (i < 0) printf("[controls] unknown button \"%s\" in [controls.buttons]\n", cc->button[k]);
    else if (!named[i]) named[i] = cc->name[k];
  }
  for (int i = 0; i < WANTED; i++) {
    if (!is_stick(i)) plan_add(c, i, named[i]);
  }
  // las acciones de [controls.keys], por su nombre, y al final las palancas
  // (se pueden saltar: el resto ya esta asignado)
  for (int k = 0; k < cc->keys_count && k < 8; k++) plan_add(c, WANTED + k, cc->key_button[k]);
  for (int i = 0; i < WANTED; i++) {
    if (is_stick(i)) plan_add(c, i, NULL);
  }
}

static void capture_lines(Screen *s, const Capture *c) {
  s->line_count = 0;
  ui_line(s, ui_colors()->highlight, "Press %s", c->label[c->step]);
  ui_line(s, ui_colors()->muted, "%d of %d", c->step + 1, c->count);
#ifdef __ANDROID__
  // en Android se cancela con Atras (GameActivity cierra la pantalla)
#define CANCEL_KEY "back"
#else
#define CANCEL_KEY "escape"
#endif
  s->notice = plan_optional(c, c->step) ? "not on your controller? Start: skip   " CANCEL_KEY ": cancel"
                                       : CANCEL_KEY ": cancel";
}

// 1 al terminar, 2 si se cancela
static int capture_tick(Screen *s, void *context) {
  Capture *c = context;
  char raw[24];
  int r = backend_capture_input(c->backend, CAPTURE_POLL, raw, sizeof raw);
  if (r < 0) return 2;
  if (r == 0) return 0;
  int taken = -1;
  for (int i = 0; i < c->step; i++) {
    if (strcmp(c->raw[i], raw) == 0) taken = i;
  }
  if (taken == 0 && plan_optional(c, c->step)) {
    c->raw[c->step][0] = 0;   // saltada
  } else if (taken >= 0) {
    return 0;                 // ya es de otra: se espera otra
  } else {
    snprintf(c->raw[c->step], sizeof c->raw[0], "%s", raw);
  }
  char target[64];
  plan_target(c, c->step, target, sizeof target);
  printf("[controls] %s = %s\n", target, c->raw[c->step][0] ? c->raw[c->step] : "(skipped)");
  if (++c->step == c->count) return 1;
  capture_lines(s, c);
  return 0;
}

int launcher_controls_screen(Backend *backend, const char *logo, const ControlsConfig *cc) {
  if (backend_capture_input(backend, CAPTURE_START, NULL, 0) < 0) {
    int r = ui_message(logo, "controls", "no controller found", "connect one and try again", NULL, NULL);
    return r == SCREEN_QUIT ? SCREEN_QUIT : 0;
  }
  Capture c = { .backend = backend };
  capture_plan(&c, cc);
  Screen s;
  ui_reset(&s, logo, "controls");
  s.can_back = 0;    // B tambien se asigna: se cancela con Escape
  s.no_help = 1;
  s.lines_top = 180;
  capture_lines(&s, &c);
  int r = ui_run(&s, capture_tick, &c);
  int saved = 0;
  if (r == SCREEN_EVENT + 1) {
    char pairs[1024];
    size_t n = 0;
    for (int i = 0; i < c.count && n < sizeof pairs; i++) {
      if (!c.raw[i][0]) continue;
      char target[64];
      plan_target(&c, i, target, sizeof target);
      n += (size_t)snprintf(pairs + n, sizeof pairs - n, "%s%s:%s", n ? "," : "", target, c.raw[i]);
    }
    saved = backend_save_controller(backend, pairs) == 0;
  }
  backend_capture_input(backend, CAPTURE_STOP, NULL, 0);
  if (r == SCREEN_QUIT) return SCREEN_QUIT;
  const char *what = r == SCREEN_EVENT + 1 ? (saved ? "controller saved" : "cannot save controller.map") : "nothing changed";
  r = ui_message(logo, "controls", what, NULL, NULL, NULL);
  return r == SCREEN_QUIT ? SCREEN_QUIT : 0;
}
