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

// runtime/launcher/screens.c: las pantallas del menu del launcher:
// Achievements (la lista, por tipo), Options (cada opcion cambia al elegirla
// y se guarda enseguida; con Reset achievements), Cheats y Mods.

#include <stdio.h>
#include <string.h>
#include "game/achievements/achievements.h"
#include "game/cheats/cheats.h"
#include "transfer/saves/saves.h"
#include "common.h"
#include "backend/star.h"

int launcher_achievements_screen(const char *logo) {
  int got = 0;
  for (int i = 0; i < achievements_count(); i++) got += achievements_unlocked(i) != 0;
  Screen s;
  ui_reset(&s, logo, "achievements");
  int hard = achievements_hardcore_count();
  if (hard) ui_line(&s, ui_colors()->text, "%d of %d unlocked, %d hardcore", got, achievements_count(), hard);
  else ui_line(&s, ui_colors()->text, "%d of %d unlocked", got, achievements_count());
  // la leyenda, como en RetroAchievements: cada tipo con su color y cuantos
  // van (en el orden en que aparecen en el config)
  for (int i = 0; i < achievements_count(); i++) {
    unsigned rgb = achievements_color(i);
    int k = 0;
    while (k < s.legend_count && s.legend_rgb[k] != rgb) k++;
    if (k < s.legend_count || k == 4) continue;
    int n = 0, done = 0;
    for (int j = 0; j < achievements_count(); j++) {
      if (achievements_color(j) != rgb) continue;
      n++;
      done += achievements_unlocked(j) != 0;
    }
    char name[40];
    snprintf(name, sizeof name, "%s", achievements_type(i)[0] ? achievements_type(i) : "other");
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
  for (int i = 0; i < achievements_count(); i++) {
    ui_item(&s, achievements_title(i));
    if (!achievements_unlocked(i)) s.item_rgb[s.count - 1] = ui_colors()->muted;
    s.item_badge[s.count - 1] = achievements_color(i);
    s.item_hollow[s.count - 1] = !achievements_unlocked(i);
    // conseguido en hardcore: la estrella dorada despues del nombre
    if (achievements_hardcore(i)) s.item_star[s.count - 1] = STAR_RGB;
  }
  ui_item(&s, "Back");
  s.fixed = 1;
  s.list_top = 150;
  s.rows = 6;
  for (;;) {
    int chosen = ui_run(&s, NULL, NULL);
    if (chosen == SCREEN_QUIT) return SCREEN_QUIT;
    if (chosen < 0 || chosen >= achievements_count()) return 0;
    char when[64] = "locked";
    if (achievements_unlocked(chosen)) {
      char date[32];
      saves_date_text((time_t)achievements_unlocked(chosen), date, sizeof date);
      snprintf(when, sizeof when, "unlocked %s", date);
    }
    // el tipo, legible: win_condition -> win condition
    char type[40];
    snprintf(type, sizeof type, "%s", achievements_type(chosen));
    for (char *p = type; *p; p++) if (*p == '_') *p = ' ';
    Screen d;
    ui_reset(&d, logo, achievements_title(chosen));
    if (achievements_description(chosen)[0]) ui_line(&d, ui_colors()->text, "%s", achievements_description(chosen));
    if (type[0]) ui_line(&d, ui_colors()->muted, "%s", type);
    ui_line(&d, achievements_unlocked(chosen) ? ui_colors()->highlight : ui_colors()->notice, "%s", when);
    ui_item(&d, "OK");
    while (ui_run(&d, NULL, NULL) >= SCREEN_EVENT) {}
    if (ui_quit_code() >= 0) return SCREEN_QUIT;
  }
}

// Reset achievements: olvida los conseguidos, despues de confirmarlo (No va
// elegido: no se puede deshacer). 1 si se borraron
static int reset_achievements(const char *logo) {
  int got = 0;
  for (int i = 0; i < achievements_count(); i++) got += achievements_unlocked(i) != 0;
  Screen s;
  ui_reset(&s, logo, "reset achievements");
  ui_line(&s, ui_colors()->text, "Forget %d unlocked achievements?", got);
  ui_line(&s, ui_colors()->notice, "This cannot be undone.");
  ui_item(&s, "No");
  ui_item(&s, "Yes, reset them");
  int r = ui_run(&s, NULL, NULL);
  if (r == SCREEN_QUIT) return SCREEN_QUIT;
  if (r != 1) return 0;
  achievements_reset();
  printf("[launcher] achievements reset\n");
  r = ui_message(logo, "reset achievements", "Done: no achievements unlocked.", NULL, NULL, NULL);
  return r == SCREEN_QUIT ? SCREEN_QUIT : 1;
}

// Options: cada opcion cambia al elegirla y se guarda enseguida
int launcher_options_screen(Backend *backend, const char *logo, const Config *cfg, const char *config_path,
                          Settings *st) {
  enum { OPT_FPS_SHOW, OPT_PROFILE, OPT_VIBRATION, OPT_QUALITY, OPT_PIXEL, OPT_FPS, OPT_FULLSCREEN, OPT_HARDCORE, OPT_RESET,
         OPT_BACK };
  static const char *quality_names[] = { "", "Low", "Medium", "High" };
  Screen s;
  ui_reset(&s, logo, "options");
  ui_line(&s, ui_colors()->muted, "the profiler writes quixjs-profile.txt");
  s.list_top = 150;
  s.rows = 6;
  for (;;) {
    int actions[10], n = 0;
    char a[64], p[64], v[64], f[64], x[64], q[64], r[64], h[64];
    snprintf(h, sizeof(h), "Achievements: %s", st->hardcore ? "Hardcore" : "Normal");
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
    if (achievements_count() > 0) {
      // Hardcore: sin trucos; lo que se consigue asi lleva la estrella
      ui_item(&s, h);
      actions[n++] = OPT_HARDCORE;
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
    else if (action == OPT_HARDCORE) {
      st->hardcore = !st->hardcore;
      settings_apply_cheats(st);   // en hardcore, ningun truco
    }
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
// los logros para siempre: lo dice arriba. En hardcore el menu no la ofrece
// (launcher.c).
int launcher_cheats_screen(const char *logo, const char *config_path, Settings *st) {
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
int launcher_mods_screen(const char *logo, const Config *cfg, const char *config_path, Settings *st) {
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
