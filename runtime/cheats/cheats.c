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

// Los trucos. Ver cheats.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tomlc17.h"
#include "backend/toast.h"
#include "hooks/hooks.h"
#include "achievements/achievements.h"
#include "symbols/symbols.h"
#include "cheats.h"

#define MAX_CHEATS     32
#define MAX_CHEAT_SETS 8

typedef struct {
  int    symbol;
  int    kind;           // 0 booleano, 1 numero, 2 texto
  double num;            // el booleano (0/1) o el numero
  char   str[64];
} CheatSet;

typedef struct {
  char     id[64];
  char     name[128];
  char     description[256];
  CheatSet sets[MAX_CHEAT_SETS];
  int      set_count;
  char    *script;       // JS que cambia el juego mientras el truco esta activo (o NULL)
  int      active;
} Cheat;

static Cheat cheats[MAX_CHEATS];
static int   count = 0;

// la partida en curso segun el motor (runtime/engines/<motor>/playthrough.js)
static JSValue  pt_obj;           // el objeto de playthrough.js
static int      pt_state = 0;     // 0 sin mirar, 1 motor conocido, -1 ninguno,
                                  // 2 juego sin logros (no hace falta la partida)
static int      pt_tries = 0;
static JSValue  pt_current;       // la partida en curso (para ver cuando cambia)
static int      pt_has_current = 0;
static int      cheated = 0;      // la partida en curso uso trucos: sin logros
static int      scripts_run = 0;  // los script de los trucos activos ya se ejecutaron
static unsigned frame = 0;

// ---------- la definicion ----------

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
    for (int i = 0; i < arr.u.arr.size && count < MAX_CHEATS; i++) {
      toml_datum_t t = arr.u.arr.elem[i];
      if (t.type != TOML_TABLE) continue;
      Cheat *c = &cheats[count];
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
      count++;
    }
  }
  toml_free(result);
  if (count) printf("Cheats: %d in the config\n", count);
  return bad ? -1 : count;
}

void cheats_free(void) {
  for (int i = 0; i < count; i++) {
    free(cheats[i].script);
    cheats[i].script = NULL;
  }
  count = 0;
  scripts_run = 0;
  pt_state = pt_tries = 0;
  pt_has_current = 0;
  cheated = 0;
  frame = 0;
}

int cheats_count(void) { return count; }
const char *cheats_id(int i) { return cheats[i].id; }
const char *cheats_name(int i) { return cheats[i].name; }
const char *cheats_description(int i) { return cheats[i].description; }
int cheats_active(int i) { return cheats[i].active; }
int cheats_playthrough_cheated(void) { return cheated; }

void cheats_set_active(const char *const *ids, int n) {
  for (int i = 0; i < count; i++) {
    cheats[i].active = 0;
    for (int k = 0; k < n; k++) {
      if (strcmp(cheats[i].id, ids[k]) == 0) cheats[i].active = 1;
    }
  }
}

static int any_active(void) {
  for (int i = 0; i < count; i++) {
    if (cheats[i].active) return 1;
  }
  return 0;
}

// ---------- la partida en curso ----------

// pt_obj.name(args...): el resultado (hay que liberarlo), undefined si falla
static JSValue pt_call(JSContext *ctx, const char *name, int argc, JSValueConst *argv) {
  JSValue fn = JS_GetPropertyStr(ctx, pt_obj, name);
  JSValue r = JS_IsFunction(ctx, fn) ? JS_Call(ctx, fn, pt_obj, argc, argv) : JS_UNDEFINED;
  JS_FreeValue(ctx, fn);
  if (JS_IsException(r)) {
    JS_FreeValue(ctx, JS_GetException(ctx));
    return JS_UNDEFINED;
  }
  return r;
}

static JSValue pt_get(JSContext *ctx, const char *key) {
  JSValue k = JS_NewString(ctx, key);
  JSValue r = pt_call(ctx, "get", 1, &k);
  JS_FreeValue(ctx, k);
  return r;
}

static void pt_set(JSContext *ctx, const char *key, JSValueConst value) {
  JSValue args[2] = { JS_NewString(ctx, key), JS_DupValue(ctx, value) };
  JS_FreeValue(ctx, pt_call(ctx, "set", 2, args));
  JS_FreeValue(ctx, args[0]);
  JS_FreeValue(ctx, args[1]);
}

// El playthrough.js del motor del juego: el primero que lo reconoce
static void find_engine(JSContext *ctx) {
  const char *name, *src;
  for (int i = 0; (src = hooks_engine_file(i, &name)) != NULL; i++) {
    const char *slash = strrchr(name, '/');
    if (!slash || strcmp(slash + 1, "playthrough") != 0) continue;
    JSValue obj = JS_Eval(ctx, src, strlen(src), name, JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(obj)) {
      JS_FreeValue(ctx, JS_GetException(ctx));
      continue;
    }
    JSValue fn = JS_GetPropertyStr(ctx, obj, "detect");
    JSValue r = JS_IsFunction(ctx, fn) ? JS_Call(ctx, fn, obj, 0, NULL) : JS_FALSE;
    JS_FreeValue(ctx, fn);
    int yes = !JS_IsException(r) && JS_ToBool(ctx, r);
    if (JS_IsException(r)) JS_FreeValue(ctx, JS_GetException(ctx));
    JS_FreeValue(ctx, r);
    if (yes) {
      pt_obj = obj;
      pt_state = 1;
      printf("[cheats] engine: %.*s\n", (int)(slash - name), name);
      return;
    }
    JS_FreeValue(ctx, obj);
  }
}

// Un solo aviso por partida: cuantos trucos hay activos o, si no hay ninguno,
// que la partida ya los uso
static int noticed = 0;

static void notice(void) {
  if (noticed) return;
  noticed = 1;
  int on = 0;
  for (int i = 0; i < count; i++) on += cheats[i].active != 0;
  Toast t;
  memset(&t, 0, sizeof t);
  snprintf(t.header, sizeof t.header, "Achievements disabled");
  if (on) snprintf(t.title, sizeof t.title, "%d cheat%s on", on, on == 1 ? "" : "s");
  else snprintf(t.title, sizeof t.title, "Cheats were used in this playthrough");
  snprintf(t.text, sizeof t.text, "%s", "No achievements in this playthrough. Other playthroughs still can.");
  snprintf(t.badge, sizeof t.badge, "!");
  t.color = 0xc04040;
  toast_push(&t);
}

// El valor que fija un truco, como valor de JS
static JSValue set_value(JSContext *ctx, const CheatSet *cs) {
  if (cs->kind == 0) return JS_NewBool(ctx, cs->num != 0);
  if (cs->kind == 1) return JS_NewFloat64(ctx, cs->num);
  return JS_NewString(ctx, cs->str);
}

// Los script de los trucos activos, una vez: con la primera partida, cuando el
// juego y sus plugins ya estan cargados (si no, un plugin podria sustituir
// despues la funcion que cambia el truco)
static void run_scripts(JSContext *ctx) {
  if (scripts_run) return;
  scripts_run = 1;
  for (int i = 0; i < count; i++) {
    if (!cheats[i].active || !cheats[i].script) continue;
    char name[96];
    snprintf(name, sizeof name, "cheat:%s", cheats[i].id);
    JSValue r = JS_Eval(ctx, cheats[i].script, strlen(cheats[i].script), name, JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(r)) {
      JSValue e = JS_GetException(ctx);
      const char *msg = JS_ToCString(ctx, e);
      fprintf(stderr, "[cheats] %s: the script failed: %s\n", cheats[i].id, msg ? msg : "?");
      if (msg) JS_FreeCString(ctx, msg);
      JS_FreeValue(ctx, e);
    } else {
      printf("[cheats] %s: on\n", cheats[i].id);
    }
    JS_FreeValue(ctx, r);
  }
}

// Otra partida (nueva o cargada): ¿esta marcada? Y los simbolos de los trucos
// que ya no estan activos vuelven al valor que tenian antes del truco
static void on_new_playthrough(JSContext *ctx) {
  run_scripts(ctx);
  JSValue mark = pt_get(ctx, "cheats");
  cheated = JS_ToBool(ctx, mark);
  JS_FreeValue(ctx, mark);
  if (cheated) {
    printf("[cheats] this playthrough used cheats: no achievements for it\n");
    notice();
  }
  JSValue orig = pt_get(ctx, "orig");
  if (JS_IsObject(orig)) {
    for (int i = 0; i < count; i++) {
      if (cheats[i].active) continue;
      for (int k = 0; k < cheats[i].set_count; k++) {
        const char *sym = symbols_name(cheats[i].sets[k].symbol);
        if (!JS_HasPropertyStr(ctx, orig, sym)) continue;
        JSValue was = JS_GetPropertyStr(ctx, orig, sym);
        if (symbols_set(ctx, cheats[i].sets[k].symbol, was) == 0) {
          JS_DeletePropertyStr(ctx, orig, sym);
          printf("[cheats] %s is off: %s back to its value\n", cheats[i].id, sym);
        }
        JS_FreeValue(ctx, was);
      }
    }
  }
  JS_FreeValue(ctx, orig);
}

// Los trucos activos: la partida queda marcada y sus simbolos fijados (lo de
// antes de cada uno, apuntado en la partida para poder devolverlo)
static void apply(JSContext *ctx) {
  if (!cheated) {
    pt_set(ctx, "cheats", JS_TRUE);
    cheated = 1;
    printf("[cheats] cheats on: no achievements in this playthrough (other playthroughs still get them)\n");
    notice();
  }
  JSValue orig = pt_get(ctx, "orig");
  if (!JS_IsObject(orig)) {
    JS_FreeValue(ctx, orig);
    JSValue o = JS_NewObject(ctx);
    pt_set(ctx, "orig", o);
    JS_FreeValue(ctx, o);
    orig = pt_get(ctx, "orig");
  }
  for (int i = 0; i < count; i++) {
    if (!cheats[i].active) continue;
    for (int k = 0; k < cheats[i].set_count; k++) {
      const CheatSet *cs = &cheats[i].sets[k];
      const char *sym = symbols_name(cs->symbol);
      JSValue now = symbols_get(ctx, cs->symbol);
      if (JS_IsObject(orig) && !JS_HasPropertyStr(ctx, orig, sym)) {
        JS_SetPropertyStr(ctx, orig, sym, JS_DupValue(ctx, now));
      }
      JSValue want = set_value(ctx, cs);
      if (!JS_StrictEq(ctx, now, want)) symbols_set(ctx, cs->symbol, want);
      JS_FreeValue(ctx, want);
      JS_FreeValue(ctx, now);
    }
  }
  JS_FreeValue(ctx, orig);
}

// Un juego sin logros (HexGL...): no hay logros que proteger, asi que no hace
// falta saber la partida. Los simbolos de los trucos activos se fijan cada
// pocas imagenes (un camino que aun no existe no se fija) y los script corren
// una vez, a los dos segundos (el juego ya cargo sus scripts)
static void apply_free(JSContext *ctx) {
  if (frame >= 120) run_scripts(ctx);
  if (frame % 10 != 0) return;
  for (int i = 0; i < count; i++) {
    if (!cheats[i].active) continue;
    for (int k = 0; k < cheats[i].set_count; k++) {
      const CheatSet *cs = &cheats[i].sets[k];
      JSValue now = symbols_get(ctx, cs->symbol);
      JSValue want = set_value(ctx, cs);
      if (!JS_StrictEq(ctx, now, want)) symbols_set(ctx, cs->symbol, want);
      JS_FreeValue(ctx, want);
      JS_FreeValue(ctx, now);
    }
  }
}

void cheats_tick(JSContext *ctx) {
  frame++;
  if (pt_state == 2) {
    apply_free(ctx);
    return;
  }
  if (pt_state == 0 && ach_count() == 0) {
    pt_state = 2;
    if (any_active()) printf("[cheats] the game has no achievements: cheats on without a playthrough\n");
    return;
  }
  if (pt_state == 0) {
    // el motor se mira cada medio segundo hasta que el juego lo ha cargado
    if (frame % 30 != 1) return;
    find_engine(ctx);
    if (pt_state != 1 && ++pt_tries == 40) {
      pt_state = -1;
      if (any_active()) fprintf(stderr, "[cheats] unknown engine: cheats are not applied\n");
    }
    if (pt_state != 1) return;
  }
  if (pt_state != 1) return;
  JSValue cur = pt_call(ctx, "current", 0, NULL);
  int same = pt_has_current ? JS_StrictEq(ctx, cur, pt_current) : JS_IsNull(cur) || JS_IsUndefined(cur);
  if (!same) {
    if (pt_has_current) JS_FreeValue(ctx, pt_current);
    pt_current = JS_DupValue(ctx, cur);
    pt_has_current = 1;
    cheated = 0;
    noticed = 0;   // otra partida: su propio aviso
    if (JS_IsObject(cur)) on_new_playthrough(ctx);
  }
  // los trucos activos, cada pocas imagenes (que no se pierdan al cambiar de
  // mapa o cargar)
  if (JS_IsObject(cur) && any_active() && (frame % 10 == 0 || !cheated)) apply(ctx);
  JS_FreeValue(ctx, cur);
}

size_t cheats_describe(char *out, size_t room) {
  size_t n = 0;
  if (room) out[0] = 0;
  for (int i = 0; i < count && n < room; i++) {
    n += (size_t)snprintf(out + n, room - n, "cheat %s %s  %s\n", cheats[i].active ? "on " : "off", cheats[i].id,
                          cheats[i].name);
  }
  if (count && n < room) {
    n += (size_t)snprintf(out + n, room - n, "playthrough: %s\n", pt_state == 2 ? "not needed (no achievements)"
                                                                : pt_state != 1 ? "unknown engine"
                                                                  : cheated ? "used cheats (no achievements)"
                                                                            : "clean");
  }
  return n < room ? n : room;
}
