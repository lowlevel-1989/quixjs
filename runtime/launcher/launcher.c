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

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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
  // a la izquierda; los que faltan, en gris; los conseguidos, en el color del
  // texto; delante, el cuadrado del color de su tipo
  s.align_left = 1;
  for (int i = 0; i < ach_count(); i++) {
    ui_item(&s, ach_title(i));
    if (!ach_unlocked(i)) s.item_rgb[s.count - 1] = ui_colors()->muted;
    s.item_badge[s.count - 1] = ach_color(i);
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
  enum { OPT_FPS_SHOW, OPT_PROFILE, OPT_FULLSCREEN, OPT_RESET, OPT_BACK };
  Screen s;
  ui_reset(&s, logo, "options");
  ui_line(&s, ui_colors()->muted, "the profiler writes quixjs-profile.txt");
  s.list_top = 150;
  s.rows = 6;
  for (;;) {
    int actions[8], n = 0;
    char a[64], p[64], f[64];
    snprintf(a, sizeof(a), "Show FPS: %s", st->show_fps ? "On" : "Off");
    snprintf(p, sizeof(p), "Profiler: %s", st->profile ? "On" : "Off");
    snprintf(f, sizeof(f), "Fullscreen: %s", backend->fullscreen ? "On" : "Off");
    s.count = 0;
    ui_item(&s, a);
    actions[n++] = OPT_FPS_SHOW;
    ui_item(&s, p);
    actions[n++] = OPT_PROFILE;
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
static int settings_has_cheat(const Settings *st, const char *id) {
  for (int i = 0; i < st->cheats_count; i++) {
    if (strcmp(st->cheats[i], id) == 0) return i;
  }
  return -1;
}

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
      snprintf(items[i], sizeof items[i], "%s: %s", cheats_name(i), settings_has_cheat(st, cheats_id(i)) >= 0 ? "On" : "Off");
      ui_item(&s, items[i]);
    }
    ui_item(&s, "Back");
    s.fixed = 1;
    int chosen = ui_run(&s, NULL, NULL);
    if (chosen == SCREEN_QUIT) return SCREEN_QUIT;
    if (chosen < 0 || chosen >= n) return 0;
    int k = settings_has_cheat(st, cheats_id(chosen));
    if (k >= 0) {
      memmove(st->cheats[k], st->cheats[k + 1], sizeof st->cheats[0] * (size_t)(st->cheats_count - k - 1));
      st->cheats_count--;
    } else if (st->cheats_count < MAX_SETTING_CHEATS) {
      snprintf(st->cheats[st->cheats_count++], sizeof st->cheats[0], "%s", cheats_id(chosen));
    }
    settings_save(st, config_path);
    settings_apply_cheats(st);
  }
}

// Mods: las carpetas de mods/ (junto al toml del juego), cada una On u Off.
// Los activos llevan su numero: si dos traen el mismo archivo, gana el de
// numero mas alto (el ultimo de la lista). Se guarda en quixjs-settings.toml
// (mods) y vale al pulsar Play; hasta que se toca, los de [game] mods.
#define MAX_MOD_DIRS 32

static int mod_cmp(const void *a, const void *b) { return strcmp((const char *)a, (const char *)b); }

static int mod_dirs(const char *root, char names[][64]) {
  int n = 0;
  DIR *d = opendir(root);
  if (!d) return 0;
  struct dirent *e;
  while ((e = readdir(d)) != NULL && n < MAX_MOD_DIRS) {
    if (e->d_name[0] == '.' || strlen(e->d_name) >= 64) continue;
    char path[1024];
    struct stat st;
    snprintf(path, sizeof path, "%s/%s", root, e->d_name);
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) snprintf(names[n++], 64, "%s", e->d_name);
  }
  closedir(d);
  qsort(names, (size_t)n, 64, mod_cmp);
  return n;
}

static int settings_mod_index(const Settings *st, const char *name) {
  for (int i = 0; i < st->mods_count; i++) {
    if (strcmp(st->mods[i], name) == 0) return i;
  }
  return -1;
}

static int mods_screen(const char *logo, const Config *cfg, const char *config_path, Settings *st) {
  char names[MAX_MOD_DIRS][64];
  int n = mod_dirs(cfg->mods.root, names);
  // la primera vez, los del toml del juego
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
    int k = settings_mod_index(st, names[chosen]);
    if (k >= 0) {
      memmove(st->mods[k], st->mods[k + 1], sizeof st->mods[0] * (size_t)(st->mods_count - k - 1));
      st->mods_count--;
    } else if (st->mods_count < MAX_SETTING_MODS) {
      snprintf(st->mods[st->mods_count++], sizeof st->mods[0], "%s", names[chosen]);
    }
    st->mods_set = 1;
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

typedef struct {
  Backend *backend;
  int      step;
  char     raw[WANTED][16];   // "" = saltada
} Capture;

static void capture_lines(Screen *s, const Capture *c) {
  s->line_count = 0;
  ui_line(s, ui_colors()->highlight, "Press %s", wanted[c->step].label);
  ui_line(s, ui_colors()->muted, "%d of %d", c->step + 1, WANTED);
  s->notice = wanted[c->step].optional ? "not on your controller? Start: skip   escape: cancel" : "escape: cancel";
}

// 1 al terminar, 2 si se cancela
static int capture_tick(Screen *s, void *context) {
  Capture *c = context;
  char raw[16];
  int r = backend_capture_input(c->backend, CAPTURE_POLL, raw, sizeof raw);
  if (r < 0) return 2;
  if (r == 0) return 0;
  int taken = -1;
  for (int i = 0; i < c->step; i++) {
    if (strcmp(c->raw[i], raw) == 0) taken = i;
  }
  if (taken == START_STEP && wanted[c->step].optional) {
    c->raw[c->step][0] = 0;   // saltada
  } else if (taken >= 0) {
    return 0;                 // ya es de otra: se espera otra
  } else {
    snprintf(c->raw[c->step], sizeof c->raw[0], "%s", raw);
  }
  printf("[controls] %s = %s\n", wanted[c->step].sdl, c->raw[c->step][0] ? c->raw[c->step] : "(skipped)");
  if (++c->step == WANTED) return 1;
  capture_lines(s, c);
  return 0;
}

static int controls_screen(Backend *backend, const char *logo) {
  if (backend_capture_input(backend, CAPTURE_START, NULL, 0) < 0) {
    int r = ui_message(logo, "controls", "no controller found", "connect one and try again", NULL, NULL);
    return r == SCREEN_QUIT ? SCREEN_QUIT : 0;
  }
  Capture c = { .backend = backend };
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
    for (int i = 0; i < WANTED && n < sizeof pairs; i++) {
      if (!c.raw[i][0]) continue;
      n += (size_t)snprintf(pairs + n, sizeof pairs - n, "%s%s:%s", n ? "," : "", wanted[i].sdl, c.raw[i]);
    }
    saved = backend_save_controller(backend, pairs) == 0;
  }
  backend_capture_input(backend, CAPTURE_STOP, NULL, 0);
  if (r == SCREEN_QUIT) return SCREEN_QUIT;
  const char *what = r == SCREEN_EVENT + 1 ? (saved ? "controller saved" : "cannot save controller.map") : "nothing changed";
  r = ui_message(logo, "controls", what, NULL, NULL, NULL);
  return r == SCREEN_QUIT ? SCREEN_QUIT : 0;
}

int launcher_run(Backend *backend, const Config *cfg, const char *config_path, Settings *settings,
                 int *exit_code) {
  *exit_code = 0;
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
    if (mod_dirs(cfg->mods.root, mod_names) > 0) {
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
      result = LAUNCHER_PLAY;
      break;
    }
    if (action == EXIT) break;
    int r = action == TRANSFER  ? transfer_screen(cfg->saves.id[0] ? cfg->saves.id : title)
            : action == OPTIONS ? options_screen(backend, title, cfg, config_path, settings)
            : action == CHEATS  ? cheats_screen(title, config_path, settings)
            : action == MODS    ? mods_screen(title, cfg, config_path, settings)
            : action == CONTROLS ? controls_screen(backend, title)
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
