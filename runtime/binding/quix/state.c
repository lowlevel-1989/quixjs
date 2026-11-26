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

// runtime/binding/quix/state.c: quix.state, los savestates. El motor (un
// hook por motor, como runtime/engines/rpgmaker-mv/hooks/port/savestate.js)
// dice como guardar y cargar con register({can, save, load}); aqui se guardan
// las ranuras, en la carpeta de las partidas (.quixjs/states/<n>.json), por el
// canal save y dentro del sandbox. Los combos del mando (Select + X / Y)
// llegan por runtime/core/main/state.c, entre dos frames.
//
// En modo hardcore se puede guardar, pero no cargar; cargar uno (fuera de
// hardcore) quita el hardcore a la partida (runtime/game/cheats/playthrough.c,
// cheats_state_loaded).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "backend/toast.h"
#include "binding/events.h"
#include "core/app_context.h"
#include "core/save.h"
#include "core/sandbox.h"
#include "core/url/url.h"
#include "game/cheats/cheats.h"
#include "quix.h"
#include "common.h"

#define STATE_VERSION 1

// lo que registro el motor: sus tres funciones (undefined si no hay)
static JSValue fn_can, fn_save, fn_load;
static int registered = 0;
static int current = 1;   // la ranura elegida (1 a QUIX_STATE_SLOTS)
// la peticion del mando que espera a un momento seguro (solo una), y si el
// motor esta guardando o cargando en segundo plano (una promesa: Construct 3)
static struct { int active, load, slot; } request;
static int inflight = 0;

// La ruta de una ranura: <partidas>/.quixjs/states/<n>.json. 0 si la ranura
// no vale (solo un entero de 1 a QUIX_STATE_SLOTS: desde JS no se puede
// formar otra ruta) o si el sandbox no deja escribir ahi
static int slot_path(JSContext *ctx, int slot, char *out, size_t size) {
  if (slot < 1 || slot > QUIX_STATE_SLOTS) return 0;
  AppContext *app = JS_GetContextOpaque(ctx);
  const char *dir = app->cfg->saves.dir[0] ? app->cfg->saves.dir : app->cfg->game.game_dir;
  snprintf(out, size, "%s/.quixjs/states/%d.json", dir, slot);
  return sandbox_check_write(out, "savestate");
}

// las carpetas .quixjs/states, si faltan (dentro de las partidas: sandbox)
static void make_dirs(const char *path) {
  char dir[MAX_PATH_LEN * 2 + 2];
  snprintf(dir, sizeof dir, "%s", path);
  char *states = strrchr(dir, '/');
  if (!states) return;
  *states = 0;                       // .../.quixjs/states
  char *quixjs = strrchr(dir, '/');
  if (quixjs) {
    *quixjs = 0;                     // .../.quixjs
    sandbox_mkdir(dir, 0755, "savestate");
    *quixjs = '/';
  }
  sandbox_mkdir(dir, 0755, "savestate");
}

// llama a una funcion del motor; el resultado hay que liberarlo
static JSValue call(JSContext *ctx, JSValueConst fn, int argc, JSValueConst *argv) {
  JSValue r = JS_Call(ctx, fn, JS_UNDEFINED, argc, argv);
  if (JS_IsException(r)) js_std_dump_error(ctx);
  return r;
}

// quixstatesave / quixstateload en window, con detail {slot, mod}: un mod
// guarda lo suyo en detail.mod al guardar y lo recibe al cargar
static void dispatch(JSContext *ctx, const char *type, int slot, JSValueConst mod) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue ev = events_new(ctx, type);
  JSValue detail = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, detail, "slot", JS_NewInt32(ctx, slot));
  JS_SetPropertyStr(ctx, detail, "mod", JS_DupValue(ctx, mod));
  JS_SetPropertyStr(ctx, ev, "detail", detail);
  events_dispatch(ctx, global, ev);
  JS_FreeValue(ctx, ev);
  JS_FreeValue(ctx, global);
}

int quix_state_registered(void) { return registered; }
int quix_state_slot(void) { return current; }

// la ranura elegida; una peticion que espera pasa a ella (se guarda o se
// carga en la ultima que se elija)
void quix_state_set_slot(int slot) {
  if (slot < 1 || slot > QUIX_STATE_SLOTS) return;
  current = slot;
  if (request.active) request.slot = slot;
}

// si el motor dice que ahora se puede guardar (load 0) o cargar (load 1):
// can(load). Un juego puede guardar solo en ciertos momentos (L'Abbaye: al
// cambiar de sala) y cargar en cualquiera
int quix_state_can(JSContext *ctx, int load) {
  if (!registered) return 0;
  JSValue arg = JS_NewBool(ctx, load);
  JSValue r = call(ctx, fn_can, 1, &arg);
  int ok = !JS_IsException(r) && JS_ToBool(ctx, r);
  JS_FreeValue(ctx, r);
  return ok;
}

// la fecha de una ranura en ms desde 1970 (0: vacia)
long long quix_state_time(JSContext *ctx, int slot) {
  char path[MAX_PATH_LEN * 2 + 2];
  struct stat st;
  if (!slot_path(ctx, slot, path, sizeof path)) return 0;
  if (sandbox_stat(path, &st, "savestate") != 0) return 0;
  return (long long)st.st_mtime * 1000;
}

// ---- el aviso (Select + X / Y): "Slot 2 saved", o por que no

static const char *why(const char *r) {
  if (!strcmp(r, "off")) return "Loading is off in hardcore mode";
  if (!strcmp(r, "none")) return "Savestates are not supported in this game";
  if (!strcmp(r, "busy")) return "Not now: try again in a moment";
  if (!strcmp(r, "empty")) return "This slot is empty";
  return "Something went wrong (see the log)";
}

// El distintivo dice donde y que, de un vistazo: el numero de la ranura, en
// azul al guardar, en verde al cargar y en gris al cambiar de ranura
#define STATE_SAVE_RGB 0x6d9dc5
#define STATE_LOAD_RGB 0x81b29a
#define STATE_SLOT_RGB 0x9aa3ad

void quix_state_toast(int kind, int slot, const char *title, const char *text) {
  Toast t;
  memset(&t, 0, sizeof t);
  snprintf(t.badge, sizeof t.badge, "%d", slot);
  t.color = kind == QUIX_STATE_SAVE ? STATE_SAVE_RGB : kind == QUIX_STATE_LOAD ? STATE_LOAD_RGB : STATE_SLOT_RGB;
  snprintf(t.header, sizeof t.header, "Savestate");
  snprintf(t.group, sizeof t.group, "savestate");   // uno nuevo sustituye al anterior
  snprintf(t.title, sizeof t.title, "%s", title);
  snprintf(t.text, sizeof t.text, "%s", text ? text : "");
  printf("[savestate] %s%s%s\n", title, text && text[0] ? ": " : "", text ? text : "");
  toast_push(&t);
}

// el aviso de guardar (load 0) o cargar (load 1) con su resultado; "saving" y
// "loading" no: el motor termina despues y lo avisa entonces
void quix_state_notice(int slot, int load, const char *r) {
  if (!strcmp(r, "saving") || !strcmp(r, "loading")) return;
  const char *good = load ? "loaded" : "saved";
  char title[64];
  snprintf(title, sizeof title, "Slot %d %s%s", slot, strcmp(r, good) ? "not " : "", load ? "loaded" : "saved");
  quix_state_toast(load ? QUIX_STATE_LOAD : QUIX_STATE_SAVE, slot, title, strcmp(r, good) ? why(r) : NULL);
}

// un valor con then (una promesa del motor: Construct 3 guarda y carga asi)
static int thenable(JSContext *ctx, JSValueConst v) {
  if (!JS_IsObject(v)) return 0;
  JSValue then = JS_GetPropertyStr(ctx, v, "then");
  int ok = JS_IsFunction(ctx, then);
  JS_FreeValue(ctx, then);
  return ok;
}

// p.then(ok, fail), con data (la ranura y lo que haga falta) en los dos
static void when_done(JSContext *ctx, JSValueConst p, JSCFunctionData *ok, JSCFunctionData *fail, int n, JSValueConst *data) {
  JSValue then = JS_GetPropertyStr(ctx, p, "then");
  JSValue f[2] = { JS_NewCFunctionData(ctx, ok, 1, 0, n, (JSValue *)data),
                   JS_NewCFunctionData(ctx, fail, 1, 0, n, (JSValue *)data) };
  JSValue r = JS_Call(ctx, then, p, 2, f);
  JS_FreeValue(ctx, r);
  JS_FreeValue(ctx, f[0]);
  JS_FreeValue(ctx, f[1]);
  JS_FreeValue(ctx, then);
}

// escribe la ranura: {version, time, state, mod}, con lo de los mods
// (quixstatesave). 1 si se escribio
static int write_slot(JSContext *ctx, int slot, JSValueConst state) {
  char path[MAX_PATH_LEN * 2 + 2];
  if (!JS_IsString(state)) {
    fprintf(stderr, "[savestate] the save of the engine gave no text\n");
    return 0;
  }
  if (!slot_path(ctx, slot, path, sizeof path)) return 0;
  JSValue mod = JS_NewObject(ctx);
  dispatch(ctx, "quixstatesave", slot, mod);
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, o, "version", JS_NewInt32(ctx, STATE_VERSION));
  JS_SetPropertyStr(ctx, o, "time", JS_NewFloat64(ctx, (double)time(NULL) * 1000));
  JS_SetPropertyStr(ctx, o, "state", JS_DupValue(ctx, state));
  JS_SetPropertyStr(ctx, o, "mod", mod);
  JSValue json = JS_JSONStringify(ctx, o, JS_UNDEFINED, JS_UNDEFINED);
  JS_FreeValue(ctx, o);
  size_t len = 0;
  const char *text = JS_IsException(json) ? NULL : JS_ToCStringLen(ctx, &len, json);
  int ok = 0;
  if (text) {
    make_dirs(path);
    ok = save_write(path, text, len, 0);   // a un temporal y rename (core/save.h)
    JS_FreeCString(ctx, text);
  }
  JS_FreeValue(ctx, json);
  if (!ok) fprintf(stderr, "[savestate] cannot write %s\n", path);
  return ok;
}

// el aviso de carga ("Loading...") mientras se carga un estado: al empezar,
// ya; al terminar se suelta y se quita cuando las imagenes vuelven a ir
// normales (backend_set_loading). Sin el, la pantalla se quedaba quieta un
// rato y parecia que la consola se habia colgado
static void loading_notice(JSContext *ctx, int on) {
  AppContext *app = JS_GetContextOpaque(ctx);
  if (!app || !app->backend) return;
  if (on) backend_loading_now(app->backend);
  backend_set_loading(app->backend, on, 0);
}

// despues de cargar: la partida pierde el hardcore y los mods reciben lo suyo
static void loaded(JSContext *ctx, int slot, JSValueConst o) {
  cheats_state_loaded(ctx);   // sus datos (firmados): ya no cuenta como hardcore
  JSValue mod = JS_GetPropertyStr(ctx, o, "mod");
  if (!JS_IsObject(mod)) {
    JS_FreeValue(ctx, mod);
    mod = JS_NewObject(ctx);
  }
  dispatch(ctx, "quixstateload", slot, mod);
  JS_FreeValue(ctx, mod);
}

// la promesa del motor: termino de guardar (argv[0], el texto) o de cargar
static JSValue on_saved(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *data) {
  (void)this_val, (void)magic;
  int32_t slot = 0;
  JS_ToInt32(ctx, &slot, data[0]);
  inflight = 0;
  quix_state_notice(slot, 0, argc > 0 && write_slot(ctx, slot, argv[0]) ? "saved" : "error");
  return JS_UNDEFINED;
}

static JSValue on_loaded(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *data) {
  (void)this_val, (void)argc, (void)argv, (void)magic;
  int32_t slot = 0;
  JS_ToInt32(ctx, &slot, data[0]);
  inflight = 0;
  loading_notice(ctx, 0);
  loaded(ctx, slot, data[1]);
  quix_state_notice(slot, 1, "loaded");
  return JS_UNDEFINED;
}

static JSValue on_failed(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *data) {
  (void)this_val, (void)magic;
  int32_t slot = 0;
  JS_ToInt32(ctx, &slot, data[0]);
  inflight = 0;
  if (JS_IsObject(data[1])) loading_notice(ctx, 0);   // fallo al cargar
  if (argc > 0) {
    const char *s = JS_ToCString(ctx, argv[0]);
    fprintf(stderr, "[savestate] slot %d: %s\n", slot, s ? s : "failed");
    if (s) JS_FreeCString(ctx, s);
  }
  quix_state_notice(slot, JS_IsObject(data[1]), "error");   // data[1]: lo cargado (al cargar)
  return JS_UNDEFINED;
}

// guarda en la ranura: "saved", o "saving" si el motor guarda en segundo
// plano (una promesa: el aviso sale al terminar), o por que no
const char *quix_state_save(JSContext *ctx, int slot) {
  if (!registered) return "none";
  char path[MAX_PATH_LEN * 2 + 2];
  if (!slot_path(ctx, slot, path, sizeof path)) return "error";
  if (!quix_state_can(ctx, 0)) return "busy";
  JSValue state = call(ctx, fn_save, 0, NULL);
  if (JS_IsException(state)) return "error";
  if (thenable(ctx, state)) {
    JSValue data[2] = { JS_NewInt32(ctx, slot), JS_NewInt32(ctx, 0) };
    inflight = 1;
    when_done(ctx, state, on_saved, on_failed, 2, data);
    JS_FreeValue(ctx, state);
    return "saving";
  }
  int ok = write_slot(ctx, slot, state);
  JS_FreeValue(ctx, state);
  if (!ok) return "error";
  return "saved";
}

// carga la ranura: "loaded", o "loading" si el motor carga en segundo plano
// (una promesa), o por que no
const char *quix_state_load(JSContext *ctx, int slot) {
  if (cheats_hardcore_mode()) return "off";
  if (!registered) return "none";
  char path[MAX_PATH_LEN * 2 + 2];
  if (!slot_path(ctx, slot, path, sizeof path)) return "error";
  if (!quix_state_time(ctx, slot)) return "empty";
  if (!quix_state_can(ctx, 1)) return "busy";
  size_t len = 0;
  char *text = (char *)url_read_file(path, &len);
  if (!text) return "empty";
  JSValue o = JS_ParseJSON(ctx, text, len, "savestate");
  free(text);
  JSValue state = JS_IsObject(o) ? JS_GetPropertyStr(ctx, o, "state") : JS_UNDEFINED;
  if (!JS_IsString(state)) {
    JS_FreeValue(ctx, state);
    JS_FreeValue(ctx, o);
    fprintf(stderr, "[savestate] slot %d is not a savestate\n", slot);
    return "error";
  }
  loading_notice(ctx, 1);
  JSValue r = call(ctx, fn_load, 1, &state);
  JS_FreeValue(ctx, state);
  const char *res = "error";
  if (JS_IsException(r) || !thenable(ctx, r)) loading_notice(ctx, 0);   // ya acabo
  if (!JS_IsException(r)) {
    if (thenable(ctx, r)) {
      JSValue data[2] = { JS_NewInt32(ctx, slot), o };
      inflight = 1;
      when_done(ctx, r, on_loaded, on_failed, 2, data);
      res = "loading";
    } else {
      loaded(ctx, slot, o);
      res = "loaded";
    }
  }
  JS_FreeValue(ctx, r);
  JS_FreeValue(ctx, o);
  return res;
}

// ---- la espera (el mando y la consola de depuracion)
// Select + X / Y piden guardar o cargar: si ahora se puede, se hace ya; si no
// (un evento, un dialogo, una batalla...), la peticion espera y en cada frame
// (quix_state_tick, entre dos frames) se mira si ya se puede. Solo una a la
// vez, la ultima: guardar mientras espera una carga (o al reves) la cancela y
// la sustituye; la misma otra vez sigue esperando. Mientras el motor termina
// algo en segundo plano, otra se descarta. Cada paso con su aviso. Da lo que paso: "saved", "loaded",
// "saving", "loading", "waiting", "discarded" o por que no

// el aviso de una peticion que espera: "Slot 2: waiting to save" y debajo
// que pasara, o que se cancelo la anterior
static void waiting_toast(int slot, int load, const char *note) {
  char title[64];
  snprintf(title, sizeof title, "Slot %d: waiting to %s", slot, load ? "load" : "save");
  quix_state_toast(load ? QUIX_STATE_LOAD : QUIX_STATE_SAVE, slot, title, note ? note : load ? "It loads when the game allows it" : "It saves when the game allows it");
}

const char *quix_state_request(JSContext *ctx, int load, int slot) {
  const char *r;
  const char *cancelled = NULL;
  if (load && cheats_hardcore_mode()) r = "off";   // en hardcore solo se guarda
  else if (!registered) r = "none";
  else if (inflight) {
    // el motor aun guarda o carga en segundo plano: esta se descarta
    quix_state_toast(load ? QUIX_STATE_LOAD : QUIX_STATE_SAVE, slot, "Not now", "A savestate is still being done");
    return "discarded";
  } else {
    if (request.active) {
      if (request.load == load) {
        // la misma otra vez: sigue esperando (en la ranura elegida)
        request.slot = slot;
        waiting_toast(slot, load, NULL);
        return "waiting";
      }
      // la otra: la nueva sustituye a la que esperaba
      cancelled = request.load ? "Load cancelled" : "Save cancelled";
      printf("[savestate] slot %d: the waiting %s is cancelled\n", request.slot, request.load ? "load" : "save");
      request.active = 0;
    }
    if (!quix_state_can(ctx, load)) {
      request.active = 1;
      request.load = load;
      request.slot = slot;
      waiting_toast(slot, load, cancelled);
      return "waiting";
    }
    r = load ? quix_state_load(ctx, slot) : quix_state_save(ctx, slot);
  }
  quix_state_notice(slot, load, r);
  return r;
}

void quix_state_tick(JSContext *ctx) {
  if (!request.active || inflight) return;
  if (request.load && cheats_hardcore_mode()) {   // paso a hardcore mientras esperaba una carga
    request.active = 0;
    quix_state_notice(request.slot, request.load, "off");
    return;
  }
  if (!quix_state_can(ctx, request.load)) return;
  request.active = 0;
  const char *r = request.load ? quix_state_load(ctx, request.slot) : quix_state_save(ctx, request.slot);
  quix_state_notice(request.slot, request.load, r);
}

// la peticion que espera: 0 si no hay; si hay, su ranura (y load)
int quix_state_waiting(int *load) {
  if (!request.active) return 0;
  if (load) *load = request.load;
  return request.slot;
}

// ---- quix.state

// quix.state.register({can, save, load}): el motor dice como guardar y cargar.
// can(load): si ahora se puede guardar (load false) o cargar (load true)
static JSValue js_state_register(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  if (argc < 1 || !JS_IsObject(argv[0])) return JS_ThrowTypeError(ctx, "quix.state.register: {can, save, load} required");
  JSValue can = JS_GetPropertyStr(ctx, argv[0], "can");
  JSValue save = JS_GetPropertyStr(ctx, argv[0], "save");
  JSValue load = JS_GetPropertyStr(ctx, argv[0], "load");
  if (!JS_IsFunction(ctx, can) || !JS_IsFunction(ctx, save) || !JS_IsFunction(ctx, load)) {
    JS_FreeValue(ctx, can);
    JS_FreeValue(ctx, save);
    JS_FreeValue(ctx, load);
    return JS_ThrowTypeError(ctx, "quix.state.register: can, save and load must be functions");
  }
  if (registered) {
    JS_FreeValue(ctx, fn_can);
    JS_FreeValue(ctx, fn_save);
    JS_FreeValue(ctx, fn_load);
  }
  fn_can = can;
  fn_save = save;
  fn_load = load;
  registered = 1;
  printf("[savestate] the engine registered its savestates\n");
  return JS_UNDEFINED;
}

// el numero de ranura de argv[0] (o la elegida)
static int arg_slot(JSContext *ctx, int argc, JSValueConst *argv) {
  int32_t slot = current;
  if (argc > 0 && !JS_IsUndefined(argv[0])) JS_ToInt32(ctx, &slot, argv[0]);
  return slot;
}

static JSValue js_state_save(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  return JS_NewString(ctx, quix_state_save(ctx, arg_slot(ctx, argc, argv)));
}

static JSValue js_state_load(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  return JS_NewString(ctx, quix_state_load(ctx, arg_slot(ctx, argc, argv)));
}

// quix.state.slots: [{slot, time}] (time 0: vacia)
static JSValue js_state_get_slots(JSContext *ctx, JSValueConst this_val) {
  (void)this_val;
  JSValue a = JS_NewArray(ctx);
  for (int i = 1; i <= QUIX_STATE_SLOTS; i++) {
    JSValue s = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, s, "slot", JS_NewInt32(ctx, i));
    JS_SetPropertyStr(ctx, s, "time", JS_NewFloat64(ctx, (double)quix_state_time(ctx, i)));
    JS_SetPropertyUint32(ctx, a, (uint32_t)(i - 1), s);
  }
  return a;
}

static JSValue js_state_get_slot(JSContext *ctx, JSValueConst this_val) {
  (void)this_val;
  return JS_NewInt32(ctx, current);
}

static JSValue js_state_set_slot(JSContext *ctx, JSValueConst this_val, JSValueConst v) {
  (void)this_val;
  int32_t slot = 0;
  JS_ToInt32(ctx, &slot, v);
  quix_state_set_slot(slot);
  return JS_UNDEFINED;
}

// quix.state.available: se puede guardar ahora (hay motor y can() dice que
// si; tambien en hardcore, donde solo se guarda)
static JSValue js_state_get_available(JSContext *ctx, JSValueConst this_val) {
  (void)this_val;
  return JS_NewBool(ctx, quix_state_can(ctx, 0));
}

static const JSCFunctionListEntry js_quix_state[] = {
  JS_CFUNC_DEF("register", 1, js_state_register),
  JS_CFUNC_DEF("save", 1, js_state_save),
  JS_CFUNC_DEF("load", 1, js_state_load),
  JS_CGETSET_DEF("slots", js_state_get_slots, NULL),
  JS_CGETSET_DEF("slot", js_state_get_slot, js_state_set_slot),
  JS_CGETSET_DEF("available", js_state_get_available, NULL),
};

void quix_add_state(JSContext *ctx, JSValueConst quix) {
  quix_add_group(ctx, quix, "state", js_quix_state, countof(js_quix_state));
}
