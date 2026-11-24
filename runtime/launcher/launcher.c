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

// runtime/launcher/launcher.c: el menu de --launcher (launcher_run): Play,
// Transfer Save, las opciones del juego y Exit, con los colores de
// [launcher]. La pantalla y como se elige son las de ui.c; las pantallas del
// menu, en screens.c y controls.c.

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "game/achievements/achievements.h"
#include "game/cheats/cheats.h"
#include "binding/quix/quix.h"
#include "gfx/soft/color.h"
#include "transfer/saves/saves.h"
#include "transfer/transfer.h"
#include "common.h"

static unsigned rgb_of(const char *css, unsigned fallback) {
  Color c;
  if (!css || !css[0] || color_parse(css, &c) != 0) return fallback;
  return (unsigned)c.r << 16 | (unsigned)c.g << 8 | c.b;
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
  launcher_controls_screen(backend, title, &cfg->controls);
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
  if (achievements_count() > 0) {
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
  // el menos usado: al final de las que se deslizan. En hardcore no hay
  // trucos: ni la opcion
  if (cheats_count() > 0 && !cheats_hardcore_mode()) {
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
            : action == OPTIONS ? launcher_options_screen(backend, title, cfg, config_path, settings)
            : action == CHEATS  ? launcher_cheats_screen(title, config_path, settings)
            : action == MODS    ? launcher_mods_screen(title, cfg, config_path, settings)
            : action == CONTROLS ? launcher_controls_screen(backend, title, &cfg->controls)
                                : launcher_achievements_screen(title);
    // Receive Save y Restore Backup cambian el archivo de los logros
    if (action == TRANSFER) achievements_reload();
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
