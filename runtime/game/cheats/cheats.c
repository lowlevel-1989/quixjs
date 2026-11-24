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

// runtime/game/cheats/cheats.c: los trucos (ver cheats.h). Este archivo tiene
// la lista, lo que se consulta de ella, encenderlos y lo que dice el comando
// ach de la consola. Las demas partes: define.c (leer el toml),
// playthrough.c (quix.playthrough) y apply.c (cada frame); lo que comparten,
// en common.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

Cheat       cheats_list[MAX_CHEATS];
int         cheats_total = 0;
CheatsState cheats_st;

void cheats_free(void) {
  for (int i = 0; i < cheats_total; i++) {
    free(cheats_list[i].script);
    cheats_list[i].script = NULL;
  }
  cheats_total = 0;
  cheats_st.scripts_run = 0;
  cheats_st.pt_state = 0;
  cheats_st.pt_warned = 0;
  cheats_st.cheated = 0;
  cheats_st.pt_hardcore = 0;
  cheats_st.frame = 0;
}

int cheats_count(void) { return cheats_total; }
const char *cheats_id(int i) { return cheats_list[i].id; }
const char *cheats_name(int i) { return cheats_list[i].name; }
const char *cheats_description(int i) { return cheats_list[i].description; }
int cheats_active(int i) { return cheats_list[i].active; }
int cheats_playthrough_cheated(void) { return cheats_st.cheated; }

void cheats_set_hardcore(int on) {
  cheats_st.hardcore_mode = on != 0;
  if (on) {
    for (int i = 0; i < cheats_total; i++) cheats_list[i].active = 0;
  }
}

int cheats_hardcore_mode(void) { return cheats_st.hardcore_mode; }

int cheats_playthrough_hardcore(void) {
  if (!cheats_st.hardcore_mode || cheats_st.cheated) return 0;
  return cheats_st.pt_has_data ? cheats_st.pt_hardcore : 1;
}

void cheats_set_active(const char *const *ids, int n) {
  if (cheats_st.hardcore_mode) n = 0;   // en hardcore, ninguno
  for (int i = 0; i < cheats_total; i++) {
    cheats_list[i].active = 0;
    for (int k = 0; k < n; k++) {
      if (strcmp(cheats_list[i].id, ids[k]) == 0) cheats_list[i].active = 1;
    }
  }
}

int cheats_any_active(void) {
  for (int i = 0; i < cheats_total; i++) {
    if (cheats_list[i].active) return 1;
  }
  return 0;
}

// ---------- la consola ----------

size_t cheats_describe(char *out, size_t room) {
  size_t n = 0;
  if (room) out[0] = 0;
  for (int i = 0; i < cheats_total && n < room; i++) {
    n += (size_t)snprintf(out + n, room - n, "cheat %s %s  %s\n", cheats_list[i].active ? "on " : "off", cheats_list[i].id,
                          cheats_list[i].name);
  }
  if (cheats_total && n < room) {
    n += (size_t)snprintf(out + n, room - n, "playthrough: %s\n", cheats_st.pt_state == 2 ? "not needed (no achievements)"
                                                                : !cheats_st.pt_has_data ? "none yet (quix.playthrough)"
                                                                  : cheats_st.cheated ? "used cheats (no achievements)"
                                                                            : "clean");
  }
  return n < room ? n : room;
}
