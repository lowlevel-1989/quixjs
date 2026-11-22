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

// El menu de --launcher. La pantalla y como se elige son las de ui.c.

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "achievements/achievements.h"
#include "binding/quix.h"
#include "gfx/color.h"
#include "transfer/saves.h"
#include "transfer/transfer.h"
#include "cheats/cheats.h"
#include "launcher.h"
#include "ui.h"

static unsigned rgb_of(const char *css, unsigned fallback) {
  Color c;
  if (!css || !css[0] || color_parse(css, &c) != 0) return fallback;
  return (unsigned)c.r << 16 | (unsigned)c.g << 8 | c.b;
}

static int achievements_screen(const char *logo) {
  int got = 0;
  for (int i = 0; i < ach_count(); i++) got += ach_unlocked(i) != 0;
  Screen s;
  ui_reset(&s, logo, "achievements");
  ui_line(&s, ui_colors()->text, "%d of %d unlocked", got, ach_count());
  // la leyenda, como en RetroAchievements: cada tipo con su color y cuantos
  // van (en el orden en que aparecen en el config)
  for (int i = 0; i < ach_count(); i++) {
    unsigned rgb = ach_color(i);
    int k = 0;
    while (k < s.legend_count && s.legend_rgb[k] != rgb) k++;
    if (k < s.legend_count || k == 4) continue;
    int n = 0, done = 0;
    for (int j = 0; j < ach_count(); j++) {
      if (ach_color(j) != rgb) continue;
      n++;
      done += ach_unlocked(j) != 0;
    }
    char name[40];
    snprintf(name, sizeof name, "%s", ach_type(i)[0] ? ach_type(i) : "other");
    for (char *p = name; *p; p++) if (*p == '_') *p = ' ';
    if (name[0] >= 'a' && name[0] <= 'z') name[0] = (char)(name[0] - 32);
    snprintf(s.legend[k], sizeof s.legend[0], "%s %d/%d", name, done, n);
    s.legend_rgb[k] = rgb;
    s.legend_count++;
  }
  // a la izquierda; delante, el cuadrado del color de su tipo: lleno los
  // conseguidos; los que faltan, solo su borde y el texto en gris (tambien
  // con la barra encima)
  s.align_left = 1;
  for (int i = 0; i < ach_count(); i++) {
    ui_item(&s, ach_title(i));
    if (!ach_unlocked(i)) s.item_rgb[s.count - 1] = ui_colors()->muted;
    s.item_badge[s.count - 1] = ach_color(i);
    s.item_hollow[s.count - 1] = !ach_unlocked(i);
  }
  ui_item(&s, "Back");
  s.fixed = 1;
  s.list_top = 150;
  s.rows = 6;
  for (;;) {
    int chosen = ui_run(&s, NULL, NULL);
    if (chosen == SCREEN_QUIT) return SCREEN_QUIT;
    if (chosen < 0 || chosen >= ach_count()) return 0;
    char when[64] = "locked";
    if (ach_unlocked(chosen)) {
      char date[32];
      saves_date_text((time_t)ach_unlocked(chosen), date, sizeof date);
      snprintf(when, sizeof when, "unlocked %s", date);
    }
    // el tipo, legible: win_condition -> win condition
    char type[40];
    snprintf(type, sizeof type, "%s", ach_type(chosen));
    for (char *p = type; *p; p++) if (*p == '_') *p = ' ';
    Screen d;
    ui_reset(&d, logo, ach_title(chosen));
    if (ach_description(chosen)[0]) ui_line(&d, ui_colors()->text, "%s", ach_description(chosen));
    if (type[0]) ui_line(&d, ui_colors()->muted, "%s", type);
    ui_line(&d, ach_unlocked(chosen) ? ui_colors()->highlight : ui_colors()->notice, "%s", when);
    ui_item(&d, "OK");
    while (ui_run(&d, NULL, NULL) >= SCREEN_EVENT) {}
    if (ui_quit_code() >= 0) return SCREEN_QUIT;
  }
}

// Reset achievements: olvida los conseguidos, despues de confirmarlo (No va
// elegido: no se puede deshacer). 1 si se borraron
static int reset_achievements(const char *logo) {
  int got = 0;
  for (int i = 0; i < ach_count(); i++) got += ach_unlocked(i) != 0;
  Screen s;
  ui_reset(&s, logo, "reset achievements");
  ui_line(&s, ui_colors()->text, "Forget %d unlocked achievements?", got);
  ui_line(&s, ui_colors()->notice, "This cannot be undone.");
  ui_item(&s, "No");
  ui_item(&s, "Yes, reset them");
  int r = ui_run(&s, NULL, NULL);
  if (r == SCREEN_QUIT) return SCREEN_QUIT;
  if (r != 1) return 0;
  ach_reset();
  printf("[launcher] achievements reset\n");
  r = ui_message(logo, "reset achievements", "Done: no achievements unlocked.", NULL, NULL, NULL);
  return r == SCREEN_QUIT ? SCREEN_QUIT : 1;
}

// Options: cada opcion cambia al elegirla y se guarda enseguida
static int options_screen(Backend *backend, const char *logo, const Config *cfg, const char *config_path,
                          Settings *st) {
  enum { OPT_FPS_SHOW, OPT_PROFILE, OPT_VIBRATION, OPT_QUALITY, OPT_PIXEL, OPT_FPS, OPT_FULLSCREEN, OPT_RESET, OPT_BACK };
  static const char *quality_names[] = { "", "Low", "Medium", "High" };
  Screen s;
  ui_reset(&s, logo, "options");
  ui_line(&s, ui_colors()->muted, "the profiler writes quixjs-profile.txt");
  s.list_top = 150;
  s.rows = 6;
  for (;;) {
    int actions[10], n = 0;
    char a[64], p[64], v[64], f[64], x[64], q[64], r[64];
    int quality = st->quality ? st->quality : cfg->engine.quality;
    snprintf(q, sizeof(q), "Quality: %s", quality_names[quality]);
    snprintf(a, sizeof(a), "Show FPS: %s", st->show_fps ? "On" : "Off");
    snprintf(p, sizeof(p), "Profiler: %s", st->profile ? "On" : "Off");
    snprintf(v, sizeof(v), "Vibration: %s", st->no_vibration ? "Off" : "On");
    snprintf(f, sizeof(f), "Fullscreen: %s", backend->fullscreen ? "On" : "Off");
    int fps = st->fps ? st->fps : cfg->engine.fps;
    snprintf(r, sizeof(r), "FPS: %d", fps == 30 ? 30 : 60);
    snprintf(x, sizeof(x), "Scaling: %s", backend->scaling == SCALING_SMOOTH ? "Smooth"
                                         : backend->scaling == SCALING_PIXEL ? "Pixel perfect" : "Sharp");
    s.count = 0;
    ui_item(&s, a);
    actions[n++] = OPT_FPS_SHOW;
    ui_item(&s, p);
    actions[n++] = OPT_PROFILE;
    // solo si el juego vibra ([controls] vibration)
    if (cfg->controls.vibration) {
      ui_item(&s, v);
      actions[n++] = OPT_VIBRATION;
    }
    // solo si el port la trae ([engine] quality): un juego 3D, cuantos
    // pixeles dibuja
    if (cfg->engine.quality) {
      ui_item(&s, q);
      actions[n++] = OPT_QUALITY;
    }
    // como se amplia el juego: Sharp (nitido), Smooth o Pixel perfect (1x, 2x...)
    ui_item(&s, x);
    actions[n++] = OPT_PIXEL;
    // 30 o 60 (sin elegir, el del port.toml)
    ui_item(&s, r);
    actions[n++] = OPT_FPS;
#ifndef QUIX_HANDHELD
    // en el PC (en una portatil el juego ya ocupa la pantalla)
    ui_item(&s, f);
    actions[n++] = OPT_FULLSCREEN;
#endif
    if (ach_count() > 0) {
      ui_item(&s, "Reset achievements");
      actions[n++] = OPT_RESET;
    }
    ui_item(&s, "Back");
    actions[n++] = OPT_BACK;
    s.fixed = 1;
    int chosen = ui_run(&s, NULL, NULL);
    if (chosen == SCREEN_QUIT) return SCREEN_QUIT;
    int action = chosen >= 0 && chosen < n ? actions[chosen] : OPT_BACK;
    if (action == OPT_FPS_SHOW) st->show_fps = !st->show_fps;
    else if (action == OPT_PROFILE) st->profile = !st->profile;
    else if (action == OPT_VIBRATION) {
      st->no_vibration = !st->no_vibration;
      // al encenderla, un golpe corto: se nota que funciona
      if (!st->no_vibration) backend_vibrate(backend, 0.8f, 0.8f, 300);
    }
    else if (action == OPT_QUALITY) st->quality = quality % 3 + 1;   // Low, Medium, High, Low...
    else if (action == OPT_PIXEL) {
      // Sharp, Smooth, Pixel perfect, Sharp... Se ve en el momento: el
      // launcher se escala igual que el juego
      backend->scaling = (backend->scaling + 1) % 3;
      st->scaling = backend->scaling + 1;
    }
    else if (action == OPT_FPS) st->fps = fps == 30 ? 60 : 30;
    else if (action == OPT_FULLSCREEN) {
      st->fullscreen = !backend->fullscreen;
      backend_set_fullscreen(backend, st->fullscreen);
    } else if (action == OPT_RESET) {
      if (reset_achievements(logo) == SCREEN_QUIT) return SCREEN_QUIT;
      continue;
    } else return 0;
    settings_save(st, config_path);
  }
}

// Cheats: cada truco se enciende o se apaga al elegirlo y se guarda enseguida
// (cheats en quixjs-settings.toml). Una partida jugada con uno activo pierde
// los logros para siempre: lo dice arriba.
static int cheats_screen(const char *logo, const char *config_path, Settings *st) {
  Screen s;
  ui_reset(&s, logo, "cheats");
  ui_line(&s, ui_colors()->notice, "A playthrough that uses a cheat");
  ui_line(&s, ui_colors()->notice, "can't unlock achievements.");
  ui_line(&s, ui_colors()->notice, "Other playthroughs still can.");
  s.list_top = 220;   // debajo de las tres lineas del aviso
  s.rows = 5;
  for (;;) {
    char items[32][160];
    int n = cheats_count() < 32 ? cheats_count() : 32;
    s.count = 0;
    for (int i = 0; i < n; i++) {
      snprintf(items[i], sizeof items[i], "%s: %s", cheats_name(i), settings_cheat_index(st, cheats_id(i)) >= 0 ? "On" : "Off");
      ui_item(&s, items[i]);
    }
    ui_item(&s, "Back");
    s.fixed = 1;
    int chosen = ui_run(&s, NULL, NULL);
    if (chosen == SCREEN_QUIT) return SCREEN_QUIT;
    if (chosen < 0 || chosen >= n) return 0;
    settings_toggle_cheat(st, cheats_id(chosen));
    settings_save(st, config_path);
    settings_apply_cheats(st);
  }
}

// Mods: las carpetas de mods/ (junto al toml del juego), cada una On u Off.
// Los activos llevan su numero: si dos traen el mismo archivo, gana el de
// numero mas alto (el ultimo de la lista). Se guarda en quixjs-settings.toml
// (mods) y vale al pulsar Play; hasta que se toca, los de [game] mods.
static int mods_screen(const char *logo, const Config *cfg, const char *config_path, Settings *st) {
  char names[MAX_MOD_DIRS][64];
  int n = settings_mod_dirs(cfg->mods.root, names);
  // hasta que se toca, los del toml del juego
  if (!st->mods_set) {
    st->mods_count = 0;
    for (int i = 0; i < cfg->mods.count && i < MAX_SETTING_MODS; i++) {
      snprintf(st->mods[st->mods_count++], sizeof st->mods[0], "%s", cfg->mods.names[i]);
    }
  }
  Screen s;
  ui_reset(&s, logo, "mods");
  s.align_left = 1;   // una lista de nombres: a la izquierda, como los logros
  ui_line(&s, ui_colors()->notice, "If two mods change the same file,");
  ui_line(&s, ui_colors()->notice, "the one with the higher number wins.");
  s.list_top = 200;   // debajo de las dos lineas del aviso
  s.rows = 6;
  for (;;) {
    char items[MAX_MOD_DIRS][160];
    s.count = 0;
    for (int i = 0; i < n; i++) {
      int k = settings_mod_index(st, names[i]);
      if (k >= 0) snprintf(items[i], sizeof items[i], "%s: On (%d)", names[i], k + 1);
      else snprintf(items[i], sizeof items[i], "%s: Off", names[i]);
      ui_item(&s, items[i]);
    }
    ui_item(&s, "Back");
    s.fixed = 1;
    int chosen = ui_run(&s, NULL, NULL);
    if (chosen == SCREEN_QUIT) return SCREEN_QUIT;
    if (chosen < 0 || chosen >= n) return 0;
    settings_toggle_mod(st, (const char (*)[64])cfg->mods.names, cfg->mods.count, names[chosen]);
    settings_save(st, config_path);
  }
}

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

// Lo que se pide: sin [controls.buttons], todo el mando estandar; con ella,
// Start (con el se saltan las que no tenga el mando), Select, los botones con
// nombre y lo que usa el menu, en el orden del mando estandar (la cruceta
// junta), y siempre las dos palancas
static void capture_plan(Capture *c, const ControlsConfig *cc) {
  c->count = 0;
  c->cc = cc;
  if (cc->names_count == 0) {
    for (int i = 0; i < WANTED; i++) {
      if (!is_stick(i)) plan_add(c, i, NULL);
    }
  } else {
    // Start (con el se saltan las que no tenga el mando) y Select, siempre:
    // Select + Start cierra el juego
    const char *start_name = NULL, *select_name = NULL;
    int select = wanted_index("back");
    for (int k = 0; k < cc->names_count; k++) {
      if (wanted_index(cc->button[k]) == START_STEP) start_name = cc->name[k];
      if (wanted_index(cc->button[k]) == select) select_name = cc->name[k];
    }
    plan_add(c, START_STEP, start_name);
    plan_add(c, select, select_name);
    const char *named[WANTED] = {0};
    for (int k = 0; k < cc->names_count; k++) {
      int i = wanted_index(cc->button[k]);
      if (i < 0) printf("[controls] unknown button \"%s\" in [controls.buttons]\n", cc->button[k]);
      else if (!named[i]) named[i] = cc->name[k];
    }
    // y siempre lo que usa este menu: la cruceta, A y B (aunque el juego no
    // los nombre: un juego que solo usa la palanca no podria moverse por el menu)
    static const char *menu[] = { "dpup", "dpdown", "dpleft", "dpright", "a", "b" };
    int in_menu[WANTED] = {0};
    for (size_t m = 0; m < sizeof menu / sizeof menu[0]; m++) in_menu[wanted_index(menu[m])] = 1;
    for (int i = 0; i < WANTED; i++) {
      if (named[i] || in_menu[i]) plan_add(c, i, named[i]);
    }
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

static int controls_screen(Backend *backend, const char *logo, const ControlsConfig *cc) {
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

// Las pantallas, con los colores de [launcher]
void launcher_open_screens(Backend *backend, const Config *cfg) {
  UiColors colors = {
    .background = rgb_of(cfg->launcher.background, 0x101014),
    .text       = rgb_of(cfg->launcher.text, 0xe8e8e8),
    .logo       = rgb_of(cfg->launcher.text, 0xffffff),
    .accent     = rgb_of(cfg->launcher.accent, 0x2c4a7a),
    .muted      = 0x8a8a96,
    .notice     = 0xf3a060,
    .highlight  = 0x7ab8ff,
  };
  ui_open(backend, cfg->game.width, cfg->game.height, &colors);
}

void launcher_controls(Backend *backend, const Config *cfg) {
  launcher_open_screens(backend, cfg);
  const char *title = cfg->launcher.title[0] ? cfg->launcher.title : cfg->engine.name;
  printf("[launcher] controls: %s\n", title);
  controls_screen(backend, title, &cfg->controls);
  ui_close();
}

int launcher_run(Backend *backend, const Config *cfg, const char *config_path, Settings *settings,
                 int *exit_code) {
  *exit_code = 0;
  launcher_open_screens(backend, cfg);

  const char *title = cfg->launcher.title[0] ? cfg->launcher.title : cfg->engine.name;
  enum { PLAY, TRANSFER, ACHIEVEMENTS, CONTROLS, MODS, CHEATS, OPTIONS, EXIT };
  int actions[8], n = 0;
  Screen s;
  ui_reset(&s, title, NULL);
  s.logo_scale = 6;   // el menu principal, como el de tamatama
  s.item_scale = 3;   // pocas opciones: mas grandes
  s.logo_y = 40;
  s.sub = "quixjs " QUIX_VERSION;   // la firma, con la version
  s.sub_y = 100;
  s.list_top = 152;   // con la flecha de arriba debajo de la firma
  // como en tamatama: las opciones del juego se deslizan en su hueco (rows a
  // la vez, con flechas y barra si hay mas) y debajo, fijas y siempre a la
  // vista, las de siempre: Transfer Save, Controls y Exit
  s.rows = 4;
  s.can_back = 0;   // para salir esta Exit
  ui_item(&s, "Play");
  actions[n++] = PLAY;
  if (ach_count() > 0) {
    ui_item(&s, "Achievements");
    actions[n++] = ACHIEVEMENTS;
  }
  {
    char mod_names[MAX_MOD_DIRS][64];
    if (settings_mod_dirs(cfg->mods.root, mod_names) > 0) {
      ui_item(&s, "Mods");
      actions[n++] = MODS;
    }
  }
  ui_item(&s, "Options");
  actions[n++] = OPTIONS;
  // el menos usado: al final de las que se deslizan
  if (cheats_count() > 0) {
    ui_item(&s, "Cheats");
    actions[n++] = CHEATS;
  }
  int fixed = 0;
  if (saves_configured()) {
    ui_item(&s, "Transfer Save");
    actions[n++] = TRANSFER;
    fixed++;
  }
  if (cfg->controls.gamepad) {
    ui_item(&s, "Controls");
    actions[n++] = CONTROLS;
    fixed++;
  }
  ui_item(&s, "Exit");
  actions[n++] = EXIT;
  s.fixed = fixed + 1;
  printf("[launcher] %s\n", title);

  int result = LAUNCHER_EXIT;
  for (;;) {
    int chosen = ui_run(&s, NULL, NULL);
    int action = chosen >= 0 && chosen < n ? actions[chosen] : EXIT;
    if (chosen == SCREEN_QUIT) action = EXIT;
    if (action == PLAY) {
      // el juego tarda en arrancar: el aviso de carga del motor (abajo a la
      // derecha) sale desde aqui (core/main/start.c)
      result = LAUNCHER_PLAY;
      break;
    }
    if (action == EXIT) break;
    int r = action == TRANSFER  ? transfer_screen(cfg->saves.id[0] ? cfg->saves.id : title)
            : action == OPTIONS ? options_screen(backend, title, cfg, config_path, settings)
            : action == CHEATS  ? cheats_screen(title, config_path, settings)
            : action == MODS    ? mods_screen(title, cfg, config_path, settings)
            : action == CONTROLS ? controls_screen(backend, title, &cfg->controls)
                                : achievements_screen(title);
    // Receive Save y Restore Backup cambian el archivo de los logros
    if (action == TRANSFER) ach_reload();
    if (r == SCREEN_QUIT) break;
  }
  if (ui_quit_code() >= 0) *exit_code = ui_quit_code();
  printf("[launcher] %s\n", result == LAUNCHER_PLAY ? "play" : "exit");
  // lo pulsado en el menu no le llega al juego
  InputEvent ev;
  while (backend_pop_input(backend, &ev)) {}
  ui_close();
  return result;
}
