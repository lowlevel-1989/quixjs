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

// runtime/game/cheats/define.c: leer los trucos del toml del juego ([[cheat]]):
// su id, nombre y descripcion, los simbolos que fijan (set) y su script.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tomlc17.h"
#include "game/symbols/symbols.h"
#include "common.h"

int cheats_load(const char *config_path) {
  cheats_free();
  FILE *f = fopen(config_path, "r");
  if (!f) return 0;
  toml_result_t result = toml_parse_file(f);
  fclose(f);
  if (!result.ok) return 0;
  int bad = 0;
  toml_datum_t arr = toml_get(result.toptab, "cheat");
  if (arr.type == TOML_ARRAY) {
    for (int i = 0; i < arr.u.arr.size && cheats_total < MAX_CHEATS; i++) {
      toml_datum_t t = arr.u.arr.elem[i];
      if (t.type != TOML_TABLE) continue;
      Cheat *c = &cheats_list[cheats_total];
      memset(c, 0, sizeof *c);
      toml_datum_t id = toml_get(t, "id"), name = toml_get(t, "name");
      toml_datum_t desc = toml_get(t, "description"), set = toml_get(t, "set");
      toml_datum_t script = toml_get(t, "script");
      if (id.type != TOML_STRING || (set.type != TOML_TABLE && script.type != TOML_STRING)) {
        fprintf(stderr, "[cheats] cheat %d: it needs id and set = { symbol = value } or script\n", i + 1);
        bad = 1;
        continue;
      }
      if (script.type == TOML_STRING) c->script = strdup(script.u.s);
      snprintf(c->id, sizeof c->id, "%s", id.u.s);
      snprintf(c->name, sizeof c->name, "%s", name.type == TOML_STRING ? name.u.s : id.u.s);
      if (desc.type == TOML_STRING) snprintf(c->description, sizeof c->description, "%s", desc.u.s);
      int ok = 1;
      for (int k = 0; set.type == TOML_TABLE && k < set.u.tab.size && ok; k++) {
        int sym = symbols_find(set.u.tab.key[k]);
        toml_datum_t v = set.u.tab.value[k];
        if (sym < 0 || c->set_count == MAX_CHEAT_SETS) {
          fprintf(stderr, "[cheats] %s: unknown symbol %s (it has to be in [symbols])\n", c->id, set.u.tab.key[k]);
          ok = 0;
          break;
        }
        CheatSet *cs = &c->sets[c->set_count];
        cs->symbol = sym;
        if (v.type == TOML_BOOLEAN) {
          cs->kind = 0;
          cs->num = v.u.boolean;
        } else if (v.type == TOML_INT64) {
          cs->kind = 1;
          cs->num = (double)v.u.int64;
        } else if (v.type == TOML_FP64) {
          cs->kind = 1;
          cs->num = v.u.fp64;
        } else if (v.type == TOML_STRING) {
          cs->kind = 2;
          snprintf(cs->str, sizeof cs->str, "%s", v.u.s);
        } else {
          fprintf(stderr, "[cheats] %s: %s has to be a boolean, a number or a text\n", c->id, set.u.tab.key[k]);
          ok = 0;
          break;
        }
        c->set_count++;
      }
      if (!ok) {
        free(c->script);
        c->script = NULL;
        bad = 1;
        continue;
      }
      cheats_total++;
    }
  }
  toml_free(result);
  if (cheats_total) printf("Cheats: %d in the config\n", cheats_total);
  return bad ? -1 : cheats_total;
}
