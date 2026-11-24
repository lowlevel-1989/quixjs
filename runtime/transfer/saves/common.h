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

// runtime/transfer/saves/common.h: lo que comparten las partes de las
// partidas, y nadie mas (el resto de quixjs usa saves.h):
//   saves.c   que es una partida ([saves] files), buscarlas y lo pequeno
//   pack.c    empaquetarlas para enviarlas y guardar las que llegan
//   backup.c  el backup: mover, Restore Backup y Delete Save
#pragma once

#include <limits.h>
#include "saves.h"

// lo que dice [saves] (saves_setup)
typedef struct {
  char dir[PATH_MAX];                                // game_dir: donde estan
  char patterns[SAVES_MAX_PATTERNS][PATH_MAX];
  int  pattern_count;
  char keep[PATH_MAX];   // saves_keep: Delete Save no lo toca
} SavesState;
extern SavesState saves_st;

// saves.c: la carpeta de las partidas (backup 0) o la de su backup (1)
void saves_root_of(int backup, char *out, size_t room);
// backup.c
void saves_make_parents(const char *path);
void saves_remove_tree(const char *path);
int  saves_move_set(const SaveSet *set, const char *from, const char *to);
