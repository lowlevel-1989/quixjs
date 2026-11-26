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

// runtime/game/achievements/define.c: leer los logros del toml del juego
// ([[achievements]] y [achievements_menu]) y entender sus condiciones
// ("symbol > 3", "prev(x) == 0 || y", "debug == false"). Al terminar lee los
// ya conseguidos (store.c).

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tomlc17.h"
#include "core/sandbox.h"
#include "common.h"

static const char *skip(const char *p) {
  while (isspace((unsigned char)*p)) p++;
  return p;
}

static const char *parse_operand(const char *p, Operand *o) {
  p = skip(p);
  memset(o, 0, sizeof *o);
  if (*p == '"') {
    const char *end = strchr(p + 1, '"');
    if (!end || end - p - 1 >= (long)sizeof o->value.str) return NULL;
    o->kind = OPD_CONST;
    o->value.type = SV_STR;
    memcpy(o->value.str, p + 1, (size_t)(end - p - 1));
    return end + 1;
  }
  if (isdigit((unsigned char)*p) || ((*p == '-' || *p == '.') && (isdigit((unsigned char)p[1]) || p[1] == '.'))) {
    char *end;
    o->kind = OPD_CONST;
    o->value.type = SV_NUM;
    o->value.num = strtod(p, &end);
    return end;
  }
  char word[64];
  int n = 0;
  while ((isalnum((unsigned char)*p) || *p == '_' || *p == '$') && n < 63) word[n++] = *p++;
  word[n] = 0;
  if (!n) return NULL;
  if (strcmp(word, "true") == 0 || strcmp(word, "false") == 0) {
    o->kind = OPD_CONST;
    o->value.type = SV_NUM;
    o->value.num = word[0] == 't';
    return p;
  }
  int prev = strcmp(word, "prev") == 0 && *skip(p) == '(';
  if (prev) {
    p = skip(skip(p) + 1);
    n = 0;
    while ((isalnum((unsigned char)*p) || *p == '_' || *p == '$') && n < 63) word[n++] = *p++;
    word[n] = 0;
    p = skip(p);
    if (*p != ')') return NULL;
    p++;
  }
  o->symbol = symbols_find(word);
  if (o->symbol < 0) {
    fprintf(stderr, "[achievements] unknown symbol '%s' (add it to [symbols])\n", word);
    return NULL;
  }
  o->kind = prev ? OPD_PREV : OPD_SYMBOL;
  return p;
}

static int parse_clause(const char *text, Clause *c) {
  const char *p = parse_operand(text, &c->a);
  if (!p) return -1;
  p = skip(p);
  if (!*p) {
    c->op = OP_TRUTHY;
    return 0;
  }
  static const struct { const char *s; CondOp op; } ops[] = {
    {"==", OP_EQ}, {"!=", OP_NE}, {"<=", OP_LE}, {">=", OP_GE}, {"<", OP_LT}, {">", OP_GT},
  };
  size_t i;
  for (i = 0; i < sizeof ops / sizeof ops[0]; i++) {
    size_t n = strlen(ops[i].s);
    if (strncmp(p, ops[i].s, n) == 0) {
      c->op = ops[i].op;
      p += n;
      break;
    }
  }
  if (i == sizeof ops / sizeof ops[0]) return -1;
  p = parse_operand(p, &c->b);
  if (!p || *skip(p)) return -1;
  return 0;
}

// "a == 1 || b == 2": cada alternativa por separado
static int parse_condition(const char *text, Condition *c) {
  c->count = 0;
  char buf[512];
  snprintf(buf, sizeof buf, "%s", text);
  char *p = buf;
  for (;;) {
    char *bar = strstr(p, "||");
    if (bar) *bar = 0;
    if (c->count == MAX_ALTERNATIVES || parse_clause(p, &c->alt[c->count]) != 0) return -1;
    c->count++;
    if (!bar) return 0;
    p = bar + 2;
  }
}

int achievements_load(const char *config_path, const char *game_dir, const char *file, const char *game) {
  achievements_free();
  FILE *f = fopen(config_path, "r");
  if (!f) return 0;
  toml_result_t result = toml_parse_file(f);
  fclose(f);
  if (!result.ok) return 0;   // config_load ya lo ha dicho

  int bad = 0;
  ach_icon = 0;
  ach_menu_name[0] = 0;
  toml_datum_t menu = toml_get(result.toptab, "achievements_menu");
  if (menu.type == TOML_TABLE) {
    toml_datum_t icon = toml_get(menu, "icon");
    if (icon.type == TOML_INT64 && icon.u.int64 > 0 && icon.u.int64 < 100000) ach_icon = (int)icon.u.int64;
    toml_datum_t name = toml_get(menu, "name");
    if (name.type == TOML_STRING) snprintf(ach_menu_name, sizeof ach_menu_name, "%s", name.u.s);
  }
  toml_datum_t arr = toml_get(result.toptab, "achievements");
  if (arr.type == TOML_ARRAY) {
    for (int i = 0; i < arr.u.arr.size && ach_total < MAX_ACHIEVEMENTS; i++) {
      toml_datum_t t = arr.u.arr.elem[i];
      if (t.type != TOML_TABLE) continue;
      Achievement *a = &ach_list[ach_total];
      memset(a, 0, sizeof *a);
      toml_datum_t id = toml_get(t, "id"), title = toml_get(t, "title");
      toml_datum_t desc = toml_get(t, "description"), conds = toml_get(t, "conditions");
      if (id.type != TOML_STRING || conds.type != TOML_ARRAY) {
        fprintf(stderr, "[achievements] achievement %d: it needs id and conditions\n", i + 1);
        bad = 1;
        continue;
      }
      snprintf(a->id, sizeof a->id, "%s", id.u.s);
      snprintf(a->title, sizeof a->title, "%s", title.type == TOML_STRING ? title.u.s : id.u.s);
      if (desc.type == TOML_STRING) snprintf(a->description, sizeof a->description, "%s", desc.u.s);
      toml_datum_t type = toml_get(t, "type");
      if (type.type == TOML_STRING) snprintf(a->type, sizeof a->type, "%s", type.u.s);
      int ok = 1;
      a->progress = -1;
      toml_datum_t prog = toml_get(t, "progress"), goal = toml_get(t, "goal");
      if (prog.type == TOML_STRING) {
        a->progress = symbols_find(prog.u.s);
        a->goal = goal.type == TOML_INT64 ? (int)goal.u.int64 : 0;
        if (a->progress < 0 || a->goal <= 0) {
          fprintf(stderr, "[achievements] %s: progress needs a symbol of [symbols] and goal > 0\n", a->id);
          ok = 0;
        }
      }
      if (conds.u.arr.size == 0) fprintf(stderr, "[achievements] %s: no conditions, it never unlocks\n", a->id);
      for (int k = 0; ok && k < conds.u.arr.size; k++) {
        toml_datum_t cd = conds.u.arr.elem[k];
        if (a->condition_count == MAX_CONDITIONS || cd.type != TOML_STRING ||
            parse_condition(cd.u.s, &a->conditions[a->condition_count]) != 0) {
          fprintf(stderr, "[achievements] %s: cannot read condition '%s'\n", a->id,
                  cd.type == TOML_STRING ? cd.u.s : "?");
          ok = 0;
          break;
        }
        a->condition_count++;
      }
      if (!ok) {
        bad = 1;
        continue;
      }
      ach_total++;
    }
  }
  toml_free(result);

  snprintf(ach_game_id, sizeof ach_game_id, "%s", game ? game : "");
  ach_store_path[0] = 0;
  if (file && file[0]) {
    if (file[0] == '/') snprintf(ach_store_path, sizeof ach_store_path, "%s", file);
    else snprintf(ach_store_path, sizeof ach_store_path, "%s/%s", game_dir, file);
    // [saves] achievements viene del toml: con las partidas, no fuera
    if (!sandbox_check_write(ach_store_path, "[saves] achievements")) ach_store_path[0] = 0;
  }
  achievements_store_load();
  if (ach_total) {
    int got = 0;
    for (int i = 0; i < ach_total; i++) got += ach_list[i].unlocked != 0;
    printf("Achievements: %d (%d unlocked), %d symbols\n", ach_total, got, symbols_count());
  }
  return bad ? -1 : ach_total;
}
