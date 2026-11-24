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

// runtime/game/achievements/achievements.c: los logros (ver achievements.h).
// Este archivo tiene la lista y lo que se consulta de ella, y el comando ach
// de la consola de debug. Las demas partes: define.c (leer el toml),
// store.c (guardar los conseguidos) y check.c (cada frame); lo que comparten,
// en common.h.

#include <stdio.h>
#include <string.h>
#include "game/cheats/cheats.h"
#include "common.h"

Achievement ach_list[MAX_ACHIEVEMENTS];
int         ach_total = 0;
int         ach_icon = 0;
char        ach_store_path[1024];
char        ach_game_id[128];

int achievements_menu_icon(void) {
  return ach_icon;
}

void achievements_free(void) {
  ach_total = 0;
}

int achievements_count(void) { return ach_total; }
const char *achievements_id(int i) { return ach_list[i].id; }
const char *achievements_title(int i) { return ach_list[i].title; }
const char *achievements_description(int i) { return ach_list[i].description; }
const char *achievements_type(int i) { return ach_list[i].type; }
long long achievements_unlocked(int i) { return ach_list[i].unlocked; }
int achievements_hardcore(int i) { return ach_list[i].unlocked && ach_list[i].hardcore; }

int achievements_unlocked_count(void) {
  int n = 0;
  for (int i = 0; i < ach_total; i++) n += ach_list[i].unlocked != 0;
  return n;
}

int achievements_hardcore_count(void) {
  int n = 0;
  for (int i = 0; i < ach_total; i++) n += achievements_hardcore(i);
  return n;
}

int achievements_unlocked_by_id(const char *id) {
  for (int i = 0; i < ach_total; i++) {
    if (strcmp(ach_list[i].id, id) == 0) return ach_list[i].unlocked != 0;
  }
  return 0;
}

// ---------- la consola ----------

void achievements_describe(char *out, size_t room) {
  size_t n = 0;
  out[0] = 0;
  for (int i = 0; i < ach_total && n < room; i++) {
    n += (size_t)snprintf(out + n, room - n, "%-8s %s  %s\n", ach_list[i].unlocked ? "unlocked" : "locked",
                          ach_list[i].id, ach_list[i].title);
  }
  if (n < room) n += symbols_describe(out + n, room - n);
  if (n < room) n += cheats_describe(out + n, room - n);
  if (n == 0) snprintf(out, room, "no achievements in the config\n");
}
