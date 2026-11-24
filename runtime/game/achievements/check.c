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

// runtime/game/achievements/check.c: lo de cada frame (ach_tick): evaluar las
// condiciones de los logros que faltan, el progreso de los que cuentan
// (progress / goal) y el aviso al conseguir uno, con el color de su tipo y
// su inicial. Una partida con trucos no consigue logros (cheats).

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "backend/toast.h"
#include "game/cheats/cheats.h"
#include "common.h"

static const SymValue *operand_value(const Operand *o) {
  if (o->kind == OPD_CONST) return &o->value;
  return o->kind == OPD_PREV ? symbols_prev(o->symbol) : symbols_now(o->symbol);
}

static int clause_holds(const Clause *c) {
  const SymValue *a = operand_value(&c->a);
  if (a->type == SV_MISSING) return 0;
  if (c->op == OP_TRUTHY) return a->type == SV_NUM ? a->num != 0 : a->str[0] != 0;
  const SymValue *b = operand_value(&c->b);
  if (b->type == SV_MISSING) return 0;
  int cmp;
  if (a->type == SV_NUM && b->type == SV_NUM) cmp = a->num < b->num ? -1 : a->num > b->num;
  else if (a->type == SV_STR && b->type == SV_STR) cmp = strcmp(a->str, b->str);
  else return c->op == OP_NE;   // un numero y un texto nunca son iguales
  switch (c->op) {
    case OP_EQ: return cmp == 0;
    case OP_NE: return cmp != 0;
    case OP_LT: return cmp < 0;
    case OP_LE: return cmp <= 0;
    case OP_GT: return cmp > 0;
    case OP_GE: return cmp >= 0;
    default:    return 0;
  }
}

static int condition_holds(const Condition *c) {
  for (int i = 0; i < c->count; i++) {
    if (clause_holds(&c->alt[i])) return 1;
  }
  return 0;
}

// El color del distintivo segun el tipo: dorado los finales, azul el
// progreso, plateado lo secundario; otro tipo, un color fijo sacado del nombre
static unsigned type_color(const char *type) {
  if (!type[0]) return 0x7ab8ff;
  if (strstr(type, "win") || strstr(type, "ending")) return 0xe8b94a;
  if (strstr(type, "progress")) return 0x5aa0f0;
  if (strstr(type, "secondary") || strstr(type, "missable")) return 0xb8bcc8;
  static const unsigned palette[] = { 0xe07a5f, 0x81b29a, 0xc08ad8, 0x6cc4c4 };
  unsigned h = 0;
  for (const char *p = type; *p; p++) h = h * 31 + (unsigned char)*p;
  return palette[h % 4];
}

// La primera letra del titulo (o el primer caracter UTF-8)
static void initial(const char *title, char *out, size_t room) {
  const unsigned char *p = (const unsigned char *)title;
  while (*p && !(isalnum(*p) || *p >= 0x80)) p++;
  size_t n = !*p ? 0 : *p < 0x80 ? 1 : *p >= 0xF0 ? 4 : *p >= 0xE0 ? 3 : 2;
  if (n >= room) n = 0;
  memcpy(out, p, n);
  out[n] = 0;
  if (n == 1) out[0] = (char)toupper((unsigned char)out[0]);
}

unsigned achievements_color(int i) { return type_color(ach_list[i].type); }

static void push_unlocked(const Achievement *a) {
  int got = 0;
  for (int i = 0; i < ach_total; i++) got += ach_list[i].unlocked != 0;
  Toast t;
  memset(&t, 0, sizeof t);
  t.star = a->hardcore;   // la estrella dorada despues del nombre
  snprintf(t.header, sizeof t.header, "Achievement unlocked   %d/%d", got, ach_total);
  snprintf(t.title, sizeof t.title, "%s", a->title);
  snprintf(t.text, sizeof t.text, "%s", a->description);
  initial(a->title, t.badge, sizeof t.badge);
  t.color = type_color(a->type);
  toast_push(&t);
}

// una condicion que lee el simbolo s (en alguna alternativa)
static int condition_uses(const Condition *c, int s) {
  for (int i = 0; i < c->count; i++) {
    const Clause *k = &c->alt[i];
    if ((k->a.kind != OPD_CONST && k->a.symbol == s) || (k->b.kind != OPD_CONST && k->b.symbol == s)) return 1;
  }
  return 0;
}

// progress: el contador subio (y aun no llega a goal) y se cumplen las demas
// condiciones (las que no lo leen: "debug == false"...): "3/12"
static void check_progress(const Achievement *a) {
  const SymValue *now = symbols_now(a->progress), *prev = symbols_prev(a->progress);
  if (now->type != SV_NUM || prev->type != SV_NUM || now->num <= prev->num || now->num >= a->goal) return;
  for (int k = 0; k < a->condition_count; k++) {
    if (!condition_uses(&a->conditions[k], a->progress) && !condition_holds(&a->conditions[k])) return;
  }
  printf("Achievement progress: %s %d/%d\n", a->id, (int)now->num, a->goal);
  Toast t;
  memset(&t, 0, sizeof t);
  snprintf(t.header, sizeof t.header, "Achievement progress   %d/%d", (int)now->num, a->goal);
  snprintf(t.title, sizeof t.title, "%s", a->title);
  snprintf(t.text, sizeof t.text, "%s", a->description);
  initial(a->title, t.badge, sizeof t.badge);
  t.color = type_color(a->type);
  t.locked = 1;
  t.done = (int)now->num;
  t.goal = a->goal;
  toast_push(&t);
}

void achievements_tick(JSContext *ctx, Backend *backend, double now_ms) {
  (void)ctx;
  (void)backend;
  (void)now_ms;
  if (!ach_total) return;
  // una partida en la que se usaron trucos no consigue logros, nunca
  if (cheats_playthrough_cheated()) return;
  // en hardcore (sin trucos, la partida empezada asi) tambien se miran los ya
  // conseguidos en normal: si se cumplen otra vez, pasan a hardcore
  int hardcore = cheats_playthrough_hardcore();
  int changed = 0;
  for (int i = 0; i < ach_total; i++) {
    Achievement *a = &ach_list[i];
    if ((a->unlocked && (a->hardcore || !hardcore)) || a->condition_count == 0) continue;
    int ok = 1;
    for (int k = 0; k < a->condition_count && ok; k++) ok = condition_holds(&a->conditions[k]);
    if (!ok) continue;
    a->unlocked = (long long)time(NULL);
    a->hardcore = hardcore;
    changed = 1;
    printf("Achievement unlocked: %s (%s)%s\n", a->id, a->title, hardcore ? " in hardcore" : "");
    push_unlocked(a);
  }
  if (changed) {
    achievements_store_save();
    return;   // con un logro nuevo, sin el aviso del contador (serian dos)
  }
  for (int i = 0; i < ach_total; i++) {
    if (!ach_list[i].unlocked && ach_list[i].progress >= 0) check_progress(&ach_list[i]);
  }
}
