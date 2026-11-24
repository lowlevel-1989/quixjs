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

// El port: leer el toml del juego, crear las carpetas de las partidas,
// cargar los hooks, los simbolos, los logros y los trucos (load_port), el
// launcher antes del juego (launcher) y montar la rom (mount_rom).
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "run.h"

static void make_dirs(const char *path) {
  char tmp[PATH_MAX];
  snprintf(tmp, sizeof tmp, "%s", path);
  for (char *s = tmp + 1; *s; s++) {
    if (*s != '/') continue;
    *s = 0;
    mkdir(tmp, 0755);
    *s = '/';
  }
  mkdir(tmp, 0755);
}

// ajustes del jugador. 0 si falla (ya cerrado)
STEP int load_port(Run *r, Loop *l) {
  Config *cfg = &r->cfg;
  // [saves] y los logros: los usan el launcher y el juego
  const char *patterns[MAX_SAVE_PATTERNS];
  for (int i = 0; i < cfg->saves.files_count; i++) patterns[i] = cfg->saves.files[i];
  // dir: la carpeta de las partidas, como game_dir (sin dir, game_dir)
  const char *saves_dir = cfg->saves.dir[0] ? cfg->saves.dir : cfg->game.game_dir;
  // el sandbox: leer dentro de la carpeta del toml, escribir dentro de la de
  // las partidas; las rutas del toml tampoco pueden salir de ella
  if (sandbox_init(r->args.config_path, saves_dir) != 0 ||
      (cfg->game.game_dir[0] && !sandbox_check(cfg->game.game_dir, "[game] game_dir")) ||
      (cfg->game.rom[0] && !sandbox_check(cfg->game.rom, "[game] rom"))) {
    quit_early(l);
    return 0;
  }
  // la carpeta de las partidas, creada si aun no existe: con la rom
  // (www.zip) va fuera del juego, en gamedata/, y en un port recien
  // instalado no esta. Fear & Hunger escribe su copia .bak antes de crearla
  // y no guardaba
  struct stat st;
  r->saves_new = stat(saves_dir, &st) != 0;
  make_dirs(saves_dir);
  int npatterns = cfg->saves.files_count;
  // los logros van con las partidas: en su carpeta, y Transfer Save los manda
  // con ellas (el que llega pisa al que habia, como una partida)
  const char *ach_file = cfg->saves.achievements;
  if (ach_file[0] && ach_file[0] != '/') {
    // tambien sin partidas (Abbaye: solo logros)
    if (npatterns < MAX_SAVE_PATTERNS) patterns[npatterns++] = ach_file;
    // antes se guardaban en game_dir: se mueven la primera vez
    char old_path[MAX_PATH_LEN * 2], new_path[MAX_PATH_LEN * 2];
    snprintf(old_path, sizeof old_path, "%s/%s", cfg->game.game_dir, ach_file);
    snprintf(new_path, sizeof new_path, "%s/%s", saves_dir, ach_file);
    if (strcmp(old_path, new_path) != 0 && access(new_path, F_OK) != 0 && access(old_path, F_OK) == 0 &&
        rename(old_path, new_path) == 0) {
      printf("[achievements] moved to %s (they go with the saves)\n", new_path);
    }
  }
  saves_setup(saves_dir, patterns, npatterns);
  saves_keep(ach_file[0] && ach_file[0] != '/' ? ach_file : NULL);
  // [symbols] antes que los logros y los trucos, que los usan
  const char *config_path = r->args.config_path;
  int bad_state = symbols_load(config_path) < 0;
  const char *game_id = config_game_id(cfg);
  bad_state |= achievements_load(config_path, saves_dir, cfg->saves.achievements, game_id) < 0;
  bad_state |= cheats_load(config_path) < 0;
  if ((bad_state || hooks_load(config_path) < 0) && r->args.test) { quit_early(l); return 0; }

  // --script tambien maneja el launcher: sus frames van antes que los del juego
  if (r->args.script && input_script_load(r->args.script) != 0) { quit_early(l); return 0; }

  // los ajustes del jugador (Options del launcher)
  settings_load(&r->settings, config_path);
  settings_apply_cheats(&r->settings);
  // Options > Scaling, si se toco; si no, [engine] scaling
  if (r->settings.scaling) l->backend->scaling = r->settings.scaling - 1;
#ifndef QUIX_HANDHELD
  // Options: a pantalla completa (--fullscreen ya la pone al crear la ventana)
  if (r->settings.fullscreen && !l->backend->fullscreen) backend_set_fullscreen(l->backend, 1);
#endif
  return 1;
}

// --launcher: el menu antes del juego. 1 para jugar; si no, ya cerrado y
// *code dice con que salir
STEP int launcher(Run *r, Loop *l, int *code) {
  *code = 0;
  // --controls: solo la pantalla Controls y salir (la app de Android tiene
  // su menu, pero el mando se asigna con el mismo codigo que aqui)
  if (r->args.controls) {
    launcher_controls(l->backend, &r->cfg);
    input_script_free();
    font_quit();
    quit_early(l);
    return 0;
  }
  if (!r->args.launcher) return 1;
  int play = launcher_run(l->backend, &r->cfg, r->args.config_path, &r->settings, code) == LAUNCHER_PLAY;
  l->script_offset = ui_frames();
  // el juego tarda en arrancar (leer y correr sus scripts): el aviso de
  // carga desde que se pulsa Play
  if (play) backend_loading_now(l->backend);
  if (play) return 1;
  input_script_free();
  font_quit();
  quit_early(l);
  return 0;
}

// Los mods, el motor de JS, las APIs, las fuentes y los scripts del juego.
// 0 si no se pudo crear el motor (ya cerrado); un fallo despues deja
// l->failed y el programa sale por el final, como siempre
// [game] rom (runtime/rom): el juego (un .zip o una carpeta, junto al toml),
// montado en gamedata/ antes de leer nada del juego. El primer script del port
// ([scripts]: libs, includes o entry) solo sirve para avisar si el juego no
// esta donde dice game_dir
static void mount_rom(Run *r, Loop *l) {
  Config *cfg = &r->cfg;
  if (!cfg->game.rom[0]) return;
  const char *first = "";
  const ScriptConfig *sc = &cfg->scripts;
  for (int i = 0; !first[0] && i < sc->libs_count; i++) if (strncmp(sc->libs[i], "../", 3) != 0) first = sc->libs[i];
  for (int i = 0; !first[0] && i < sc->includes_count; i++) if (strncmp(sc->includes[i], "../", 3) != 0) first = sc->includes[i];
  if (!first[0] && sc->entry && strncmp(sc->entry, "../", 3) != 0) first = sc->entry;
  launcher_mount_rom(r->args.test ? NULL : l->backend, cfg, first);
  // [saves] dir: solo la del disco (runtime/rom/rom.h); la primera vez, con
  // las partidas que traiga la rom. Sin dir, las partidas van en gamedata/
  // misma y no se toca
  const char *saves = cfg->saves.dir;
  if (saves[0] && strcmp(saves, cfg->game.game_dir) != 0 && rom_mounted()) {
    if (r->saves_new) rom_copy_dir(saves);
    rom_hide(saves);
  }
}

