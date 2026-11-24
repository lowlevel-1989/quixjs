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

// runtime/game/cheats/playthrough.c: quix.playthrough, la partida en curso.
// Los hooks del juego la dan: start al empezar una, load al cargarla, save
// para guardarla con la partida (firmada, como los logros). apply.c la mira
// en cada frame para marcarla y aplicar los trucos.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game/achievements/achievements.h"
#include "common.h"

// un dato de la partida en curso (hay que liberarlo), undefined si no hay
JSValue cheats_pt_get(JSContext *ctx, const char *key) {
  return cheats_st.pt_has_data ? JS_GetPropertyStr(ctx, cheats_st.pt_data, key) : JS_UNDEFINED;
}

void cheats_pt_set(JSContext *ctx, const char *key, JSValueConst value) {
  if (cheats_st.pt_has_data) JS_SetPropertyStr(ctx, cheats_st.pt_data, key, JS_DupValue(ctx, value));
}

// otra partida: data (o {} si no es un objeto) pasa a ser la de ahora
static void pt_replace(JSContext *ctx, JSValueConst data) {
  if (cheats_st.pt_has_data) JS_FreeValue(ctx, cheats_st.pt_data);
  cheats_st.pt_data = JS_IsObject(data) ? JS_DupValue(ctx, data) : JS_NewObject(ctx);
  cheats_st.pt_has_data = 1;
  cheats_st.pt_gen++;
}

// quix.playthrough.start(): una partida nueva, limpia
// --trace-saves: cada llamada de la partida al log, para ver si los hooks del
// juego estan bien puestos (runtime/binding/quix/quix.c traza quien guarda)
static void pt_trace(JSContext *ctx, const char *what) {
  if (!getenv("QUIXJS_TRACE_SAVES")) return;
  int marked = 0;
  if (cheats_st.pt_has_data) {
    JSValue c = JS_GetPropertyStr(ctx, cheats_st.pt_data, "cheats");
    marked = JS_ToBool(ctx, c);
    JS_FreeValue(ctx, c);
  }
  printf("[trace-saves] quix.playthrough.%s: %s\n", what,
         cheats_st.pt_has_data ? (marked ? "a playthrough with cheats" : "a clean playthrough") : "no playthrough");
  fflush(stdout);
}

static JSValue js_pt_start(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val, (void)argc, (void)argv;
  JSValue o = JS_NewObject(ctx);
  // empezada en hardcore: sus logros cuentan como hardcore (va firmado con la
  // partida: cargarla otra vez lo conserva)
  if (cheats_st.hardcore_mode) JS_SetPropertyStr(ctx, o, "hardcore", JS_TRUE);
  pt_replace(ctx, o);
  JS_FreeValue(ctx, o);
  pt_trace(ctx, "start()");
  return JS_UNDEFINED;
}

// Los datos de la partida se guardan firmados, como los logros
// (ach_sign: el juego y el texto): { data: "<json>", sig: "<hex>" }. Al
// cargar, un guardado sin datos o con la firma mal es una partida marcada:
// borrar la marca no devuelve los logros (los guardados de antes de quixjs
// 0.6 no los dan).

// quix.playthrough.save(): lo que se guarda con la partida, firmado; null si
// no hay partida
static JSValue js_pt_save(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val, (void)argc, (void)argv;
  pt_trace(ctx, "save()");
  if (!cheats_st.pt_has_data) return JS_NULL;
  JSValue json = JS_JSONStringify(ctx, cheats_st.pt_data, JS_UNDEFINED, JS_UNDEFINED);
  if (JS_IsException(json)) return JS_NULL;
  const char *s = JS_ToCString(ctx, json);
  JSValue out = JS_NULL;
  if (s) {
    char sig[32];
    snprintf(sig, sizeof sig, "%016llx", achievements_sign(s));
    out = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, out, "data", JS_NewString(ctx, s));
    JS_SetPropertyStr(ctx, out, "sig", JS_NewString(ctx, sig));
    JS_FreeCString(ctx, s);
  }
  JS_FreeValue(ctx, json);
  return out;
}

// Un savestate cargado: la partida pierde el hardcore para siempre (hardcore:
// false y state: true en sus datos, que se guardan firmados con la partida).
// El estado trae su propia partida (el hook la carga con extractSaveContents):
// se marca esa. apply.c lo lee al ver la partida nueva (pt_gen)
void cheats_state_loaded(JSContext *ctx) {
  if (!cheats_st.pt_has_data) return;
  cheats_pt_set(ctx, "hardcore", JS_FALSE);
  cheats_pt_set(ctx, "state", JS_TRUE);
  cheats_st.pt_hardcore = 0;
  printf("[cheats] a savestate was loaded: no hardcore achievements for this playthrough\n");
}

// quix.playthrough.data(): los datos de la partida en curso (una copia, sin
// firmar), o null: para mirarlos (las pruebas, la consola)
static JSValue js_pt_data(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val, (void)argc, (void)argv;
  if (!cheats_st.pt_has_data) return JS_NULL;
  JSValue json = JS_JSONStringify(ctx, cheats_st.pt_data, JS_UNDEFINED, JS_UNDEFINED);
  const char *s = JS_IsException(json) ? NULL : JS_ToCString(ctx, json);
  JSValue copy = s ? JS_ParseJSON(ctx, s, strlen(s), "playthrough") : JS_NULL;
  if (s) JS_FreeCString(ctx, s);
  JS_FreeValue(ctx, json);
  return copy;
}

// quix.playthrough.load(saved): la partida que se acaba de cargar, con lo
// que save dio al guardarla. Sin eso o con la firma mal: marcada
static JSValue js_pt_load(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  JSValue data = JS_NULL;
  // sin datos (un guardado de antes de quixjs 0.6, o de fuera de quixjs, o
  // con la marca borrada): cuenta como con trucos, como la firma mal
  int ok = 0, none = argc < 1 || !JS_IsObject(argv[0]);
  if (!none) {
    JSValue d = JS_GetPropertyStr(ctx, argv[0], "data"), g = JS_GetPropertyStr(ctx, argv[0], "sig");
    const char *ds = JS_IsString(d) ? JS_ToCString(ctx, d) : NULL;
    const char *gs = JS_IsString(g) ? JS_ToCString(ctx, g) : NULL;
    if (ds && gs) {
      char sig[32];
      snprintf(sig, sizeof sig, "%016llx", achievements_sign(ds));
      if (strcmp(sig, gs) == 0) {
        data = JS_ParseJSON(ctx, ds, strlen(ds), "playthrough");
        ok = JS_IsObject(data);
      }
    }
    if (ds) JS_FreeCString(ctx, ds);
    if (gs) JS_FreeCString(ctx, gs);
    JS_FreeValue(ctx, d);
    JS_FreeValue(ctx, g);
  }
  if (!ok) {
    JS_FreeValue(ctx, data);
    data = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, data, "cheats", JS_TRUE);
    printf("[cheats] the loaded playthrough has %s: it counts as cheated\n", none ? "no quixjs data" : "a bad signature");
  }
  pt_replace(ctx, data);
  JS_FreeValue(ctx, data);
  pt_trace(ctx, ok ? "load(): signed data" : none ? "load(): no data" : "load(): bad signature");
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_pt_funcs[] = {
  JS_CFUNC_DEF("start", 0, js_pt_start),
  JS_CFUNC_DEF("save", 0, js_pt_save),
  JS_CFUNC_DEF("load", 1, js_pt_load),
  JS_CFUNC_DEF("data", 0, js_pt_data),
};

void cheats_add_api(JSContext *ctx, JSValueConst quix) {
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, o, js_pt_funcs, sizeof js_pt_funcs / sizeof js_pt_funcs[0]);
  JS_SetPropertyStr(ctx, quix, "playthrough", o);
}
