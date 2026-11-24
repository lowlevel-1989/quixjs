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

// runtime/game/cheats/apply.c: lo de cada frame (cheats_tick): con otra
// partida, ver si esta marcada y devolver los valores de los trucos que se
// apagaron; con los trucos activos, marcarla, fijar sus simbolos y ejecutar
// sus script; y el aviso de que esta partida no consigue logros.

#include <stdio.h>
#include <string.h>
#include "backend/toast.h"
#include "game/achievements/achievements.h"
#include "game/symbols/symbols.h"
#include "common.h"

// Un solo aviso por partida: cuantos trucos hay activos o, si no hay ninguno,
// que la partida ya los uso
static void notice(void) {
  if (cheats_st.noticed) return;
  cheats_st.noticed = 1;
  int on = 0;
  for (int i = 0; i < cheats_total; i++) on += cheats_list[i].active != 0;
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
  if (cheats_st.scripts_run) return;
  cheats_st.scripts_run = 1;
  for (int i = 0; i < cheats_total; i++) {
    if (!cheats_list[i].active || !cheats_list[i].script) continue;
    char name[96];
    snprintf(name, sizeof name, "cheat:%s", cheats_list[i].id);
    JSValue r = JS_Eval(ctx, cheats_list[i].script, strlen(cheats_list[i].script), name, JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(r)) {
      JSValue e = JS_GetException(ctx);
      const char *msg = JS_ToCString(ctx, e);
      fprintf(stderr, "[cheats] %s: the script failed: %s\n", cheats_list[i].id, msg ? msg : "?");
      if (msg) JS_FreeCString(ctx, msg);
      JS_FreeValue(ctx, e);
    } else {
      printf("[cheats] %s: on\n", cheats_list[i].id);
    }
    JS_FreeValue(ctx, r);
  }
}

// Otra partida (nueva o cargada): ¿esta marcada? Y los simbolos de los trucos
// que ya no estan activos vuelven al valor que tenian antes del truco
static void on_new_playthrough(JSContext *ctx) {
  run_scripts(ctx);
  JSValue mark = cheats_pt_get(ctx, "cheats");
  cheats_st.cheated = JS_ToBool(ctx, mark);
  JS_FreeValue(ctx, mark);
  JSValue hard = cheats_pt_get(ctx, "hardcore");
  cheats_st.pt_hardcore = JS_ToBool(ctx, hard);
  JS_FreeValue(ctx, hard);
  if (cheats_st.hardcore_mode && !cheats_st.pt_hardcore)
    printf("[cheats] hardcore mode, but this playthrough did not start in it: its achievements are normal\n");
  if (cheats_st.cheated) {
    printf("[cheats] this playthrough used cheats: no achievements for it\n");
    notice();
  }
  JSValue orig = cheats_pt_get(ctx, "orig");
  if (JS_IsObject(orig)) {
    for (int i = 0; i < cheats_total; i++) {
      if (cheats_list[i].active) continue;
      for (int k = 0; k < cheats_list[i].set_count; k++) {
        const char *sym = symbols_name(cheats_list[i].sets[k].symbol);
        if (!JS_HasPropertyStr(ctx, orig, sym)) continue;
        JSValue was = JS_GetPropertyStr(ctx, orig, sym);
        if (symbols_set(ctx, cheats_list[i].sets[k].symbol, was) == 0) {
          JS_DeletePropertyStr(ctx, orig, sym);
          printf("[cheats] %s is off: %s back to its value\n", cheats_list[i].id, sym);
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
  if (!cheats_st.cheated) {
    cheats_pt_set(ctx, "cheats", JS_TRUE);
    cheats_st.cheated = 1;
    printf("[cheats] cheats on: no achievements in this playthrough (other playthroughs still get them)\n");
    notice();
  }
  JSValue orig = cheats_pt_get(ctx, "orig");
  if (!JS_IsObject(orig)) {
    JS_FreeValue(ctx, orig);
    JSValue o = JS_NewObject(ctx);
    cheats_pt_set(ctx, "orig", o);
    JS_FreeValue(ctx, o);
    orig = cheats_pt_get(ctx, "orig");
  }
  for (int i = 0; i < cheats_total; i++) {
    if (!cheats_list[i].active) continue;
    for (int k = 0; k < cheats_list[i].set_count; k++) {
      const CheatSet *cs = &cheats_list[i].sets[k];
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
  if (cheats_st.frame >= 120) run_scripts(ctx);
  if (cheats_st.frame % 10 != 0) return;
  for (int i = 0; i < cheats_total; i++) {
    if (!cheats_list[i].active) continue;
    for (int k = 0; k < cheats_list[i].set_count; k++) {
      const CheatSet *cs = &cheats_list[i].sets[k];
      JSValue now = symbols_get(ctx, cs->symbol);
      JSValue want = set_value(ctx, cs);
      if (!JS_StrictEq(ctx, now, want)) symbols_set(ctx, cs->symbol, want);
      JS_FreeValue(ctx, want);
      JS_FreeValue(ctx, now);
    }
  }
}

void cheats_tick(JSContext *ctx) {
  cheats_st.frame++;
  if (cheats_st.pt_state == 2) {
    apply_free(ctx);
    return;
  }
  if (cheats_st.pt_state == 0 && achievements_count() == 0) {
    cheats_st.pt_state = 2;
    if (cheats_any_active()) printf("[cheats] the game has no achievements: cheats on without a playthrough\n");
    return;
  }
  // sin partida: el juego no se la dio a quixjs (hooks de start y load);
  // sin ella no hay donde marcar los trucos y no se aplican
  if (!cheats_st.pt_has_data) {
    if (cheats_any_active() && !cheats_st.pt_warned && cheats_st.frame == 1200) {
      cheats_st.pt_warned = 1;
      fprintf(stderr, "[cheats] no playthrough (quix.playthrough): cheats are not applied\n");
    }
    return;
  }
  if (cheats_st.pt_seen != cheats_st.pt_gen) {
    cheats_st.pt_seen = cheats_st.pt_gen;
    cheats_st.cheated = 0;
    cheats_st.noticed = 0;   // otra partida: su propio aviso
    on_new_playthrough(ctx);
  }
  // los trucos activos, cada pocas imagenes (que no se pierdan al cambiar de
  // mapa o cargar)
  if (cheats_any_active() && (cheats_st.frame % 10 == 0 || !cheats_st.cheated)) apply(ctx);
}
