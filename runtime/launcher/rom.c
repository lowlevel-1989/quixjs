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

// La rom ([game] rom, runtime/zip): montarla al empezar el juego. Si el zip
// viene comprimido se descomprime una vez (zip_convert_store) en otro thread,
// y mientras tanto una pantalla del launcher dice cuanto lleva.

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include "rom/rom.h"
#include "launcher.h"
#include "ui.h"

typedef struct {
  const char *rom, *root, *mount, *script;
  volatile uint64_t done, total;
  volatile int finished, result;
} Job;

static void progress(void *user, uint64_t done, uint64_t total) {
  Job *j = user;
  j->done = done;
  j->total = total;
}

static void *run(void *user) {
  Job *j = user;
  j->result = rom_mount(j->rom, j->root, j->mount, j->script, progress, j);
  j->finished = 1;
  return NULL;
}

static int tick(Screen *s, void *context) {
  Job *j = context;
  int pct = j->total ? (int)(j->done * 100 / j->total) : 0;
  snprintf(s->lines[s->line_count - 1], sizeof s->lines[0], "%d%%", pct);
  return j->finished;
}

int launcher_mount_rom(Backend *backend, const Config *cfg, const char *first_script) {
  const char *rom = cfg->game.rom, *root = cfg->game.rom_root, *mount = cfg->game.game_dir;
  // sin comprimir (lo normal despues de la primera vez) o sin pantalla: directo
  if (!rom_needs_uncompress(rom) || !backend) return rom_mount(rom, root, mount, first_script, NULL, NULL);
  Job j = { rom, root, mount, first_script, 0, 0, 0, -1 };
  pthread_t t;
  if (pthread_create(&t, NULL, run, &j) != 0) return rom_mount(rom, root, mount, first_script, NULL, NULL);
  launcher_open_screens(backend, cfg);
  Screen s;
  ui_reset(&s, cfg->launcher.title[0] ? cfg->launcher.title : cfg->engine.name, "preparing the game");
  ui_line(&s, ui_colors()->text, "The game comes compressed: it is");
  ui_line(&s, ui_colors()->text, "uncompressed once, then it starts fast.");
  ui_line(&s, ui_colors()->highlight, "0%%");
  s.can_back = 0;
  // hasta que termine (cerrar la ventana no corta la conversion a medias)
  while (!j.finished) {
    if (ui_run(&s, tick, &j) == SCREEN_QUIT) usleep(16000);
  }
  pthread_join(t, NULL);
  ui_close();
  return j.result;
}
