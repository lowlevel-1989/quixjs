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

// La captura de pantalla (Select + R1 o su boton de Controls): el backend
// pide una (want_screenshot) y el loop la guarda aqui.
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "run.h"

// Select + R1: lo que se ve, en screenshots/ junto al toml del juego
// (<juego>-<fecha>.png), con un aviso como el de los logros. En Android,
// ademas, a la galeria (Pictures/quixjs)
static void save_screenshot(Run *r, Loop *l) {
  l->backend->want_screenshot = 0;
  const Surface *s = script_screen(&r->shot_ctx);
  if (!s || !s->px) return;
  char dir[MAX_PATH_LEN], path[MAX_PATH_LEN + 128], name[160], stamp[32];
  const char *cfg = r->args.config_path, *slash = strrchr(cfg, '/');
  if (slash) snprintf(dir, sizeof dir, "%.*s/screenshots", (int)(slash - cfg), cfg);
  else snprintf(dir, sizeof dir, "screenshots");
  mkdir(dir, 0755);
  time_t now = time(NULL);
  strftime(stamp, sizeof stamp, "%Y%m%d-%H%M%S", localtime(&now));
  snprintf(name, sizeof name, "%s-%s.png", config_game_id(&r->cfg), stamp);
  snprintf(path, sizeof path, "%s/%s", dir, name);
  Toast t;
  memset(&t, 0, sizeof t);
  snprintf(t.badge, sizeof t.badge, "S");
  t.color = 0x81b29a;
  if (image_save_png(s->px, s->w, s->h, path) != 0) {
    fprintf(stderr, "[screenshot] cannot write %s\n", path);
    snprintf(t.header, sizeof t.header, "Screenshot");
    snprintf(t.title, sizeof t.title, "Could not save it");
  } else {
    int gallery = platform_add_picture(path);
    printf("[screenshot] %s%s\n", path, gallery ? " (and in Pictures/quixjs)" : "");
    snprintf(t.header, sizeof t.header, "Screenshot saved");
    snprintf(t.title, sizeof t.title, "%s", name);
    snprintf(t.text, sizeof t.text, "%s", gallery ? "In the gallery: Pictures/quixjs" : "In the screenshots folder of the port");
  }
  toast_push(&t);
}
