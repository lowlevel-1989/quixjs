/*
 * QuixJS - A lightweight JS runtime using SDL2 and JavaScriptCore
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

// [symbols]: leer y escribir el estado del juego. Ver symbols.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tomlc17.h"
#include "symbols.h"

#define MAX_SYMBOLS 64

typedef enum { SYM_PATH, SYM_VAR, SYM_SWITCH } SymbolKind;

typedef struct {
  char       name[48];
  SymbolKind kind;
  char       path[256];   // SYM_PATH: a.b.c
  int        index;       // SYM_VAR, SYM_SWITCH
  SymValue   now, prev;
} Symbol;

static Symbol symbols[MAX_SYMBOLS];
static int    count = 0;
static int    first_tick = 1;

// ---------- la definicion ----------

static int parse_symbol(Symbol *s, const char *name, const char *def) {
  snprintf(s->name, sizeof s->name, "%s", name);
  s->now.type = s->prev.type = SV_MISSING;
  if (strncmp(def, "path:", 5) == 0 && def[5]) {
    s->kind = SYM_PATH;
    snprintf(s->path, sizeof s->path, "%s", def + 5);
    return 0;
  }
  char *end;
  if (strncmp(def, "var:", 4) == 0) {
    s->kind = SYM_VAR;
    s->index = (int)strtol(def + 4, &end, 10);
    return *end || end == def + 4 ? -1 : 0;
  }
  if (strncmp(def, "switch:", 7) == 0) {
    s->kind = SYM_SWITCH;
    s->index = (int)strtol(def + 7, &end, 10);
    return *end || end == def + 7 ? -1 : 0;
  }
  return -1;
}

int symbols_load(const char *config_path) {
  symbols_free();
  FILE *f = fopen(config_path, "r");
  if (!f) return 0;
  toml_result_t result = toml_parse_file(f);
  fclose(f);
  if (!result.ok) return 0;   // config_load ya lo ha dicho
  int bad = 0;
  toml_datum_t syms = toml_get(result.toptab, "symbols");
  if (syms.type == TOML_TABLE) {
    for (int i = 0; i < syms.u.tab.size && count < MAX_SYMBOLS; i++) {
      toml_datum_t v = syms.u.tab.value[i];
      if (v.type != TOML_STRING || parse_symbol(&symbols[count], syms.u.tab.key[i], v.u.s) != 0) {
        fprintf(stderr, "[symbols] %s: expected \"path:a.b.c\", \"var:N\" or \"switch:N\"\n", syms.u.tab.key[i]);
        bad = 1;
        continue;
      }
      count++;
    }
  }
  toml_free(result);
  first_tick = 1;
  return bad ? -1 : count;
}

void symbols_free(void) {
  count = 0;
  first_tick = 1;
}

int symbols_count(void) { return count; }
const char *symbols_name(int i) { return symbols[i].name; }
const SymValue *symbols_now(int i) { return &symbols[i].now; }
const SymValue *symbols_prev(int i) { return &symbols[i].prev; }

int symbols_find(const char *name) {
  for (int i = 0; i < count; i++) {
    if (strcmp(symbols[i].name, name) == 0) return i;
  }
  return -1;
}

// ---------- leer ----------

static void to_value(JSContext *ctx, JSValue v, SymValue *out) {
  out->type = SV_MISSING;
  if (JS_IsNumber(v)) {
    double d;
    JS_ToFloat64(ctx, &d, v);
    out->type = SV_NUM;
    out->num = d;
  } else if (JS_IsBool(v)) {
    out->type = SV_NUM;
    out->num = JS_ToBool(ctx, v);
  } else if (JS_IsString(v)) {
    const char *s = JS_ToCString(ctx, v);
    if (s) {
      out->type = SV_STR;
      snprintf(out->str, sizeof out->str, "%s", s);
      JS_FreeCString(ctx, s);
    }
  }
}

// window.a.b.c (hasta n caracteres de path); un getter que lanza o un paso
// que no es objeto: no existe
static JSValue get_path_n(JSContext *ctx, const char *path, size_t len) {
  JSValue cur = JS_GetGlobalObject(ctx);
  char part[128];
  const char *p = path, *stop = path + len;
  while (p < stop) {
    const char *dot = memchr(p, '.', (size_t)(stop - p));
    size_t n = dot ? (size_t)(dot - p) : (size_t)(stop - p);
    if (n == 0 || n >= sizeof part || !JS_IsObject(cur)) {
      JS_FreeValue(ctx, cur);
      return JS_UNDEFINED;
    }
    memcpy(part, p, n);
    part[n] = 0;
    JSValue next = JS_GetPropertyStr(ctx, cur, part);
    JS_FreeValue(ctx, cur);
    if (JS_IsException(next)) {
      JS_FreeValue(ctx, JS_GetException(ctx));
      return JS_UNDEFINED;
    }
    cur = next;
    p += n + (dot ? 1 : 0);
  }
  return cur;
}

static JSValue get_path(JSContext *ctx, const char *path) {
  return get_path_n(ctx, path, strlen(path));
}

// $gameVariables o $gameSwitches (si ya existen)
static JSValue rpg_store(JSContext *ctx, const Symbol *s) {
  return get_path(ctx, s->kind == SYM_VAR ? "$gameVariables" : "$gameSwitches");
}

JSValue symbols_get(JSContext *ctx, int i) {
  Symbol *s = &symbols[i];
  if (s->kind == SYM_PATH) return get_path(ctx, s->path);
  // RPG Maker: $gameVariables._data[N] (0 si no se ha tocado) y lo mismo con
  // $gameSwitches (false). Sin la partida creada, no existe.
  JSValue data = get_path(ctx, s->kind == SYM_VAR ? "$gameVariables._data" : "$gameSwitches._data");
  if (!JS_IsObject(data)) {
    JS_FreeValue(ctx, data);
    return JS_UNDEFINED;
  }
  JSValue v = JS_GetPropertyUint32(ctx, data, (uint32_t)s->index);
  JS_FreeValue(ctx, data);
  if (JS_IsException(v)) {
    JS_FreeValue(ctx, JS_GetException(ctx));
    v = JS_UNDEFINED;
  }
  if (JS_IsUndefined(v) || JS_IsNull(v)) return s->kind == SYM_VAR ? JS_NewInt32(ctx, 0) : JS_FALSE;
  return v;
}

static void read_symbol(JSContext *ctx, Symbol *s, int i) {
  JSValue v = symbols_get(ctx, i);
  to_value(ctx, v, &s->now);
  JS_FreeValue(ctx, v);
}

void symbols_tick(JSContext *ctx) {
  for (int i = 0; i < count; i++) {
    symbols[i].prev = first_tick ? (SymValue){ .type = SV_MISSING } : symbols[i].now;
    read_symbol(ctx, &symbols[i], i);
  }
  first_tick = 0;
}

// ---------- escribir ----------

int symbols_set(JSContext *ctx, int i, JSValueConst value) {
  Symbol *s = &symbols[i];
  int ok = 0;
  if (s->kind == SYM_PATH) {
    // el objeto de antes del ultimo paso, y en el ultimo paso, el valor
    const char *last = strrchr(s->path, '.');
    JSValue parent = last ? get_path_n(ctx, s->path, (size_t)(last - s->path)) : JS_GetGlobalObject(ctx);
    const char *key = last ? last + 1 : s->path;
    if (JS_IsObject(parent) && *key) {
      ok = JS_SetPropertyStr(ctx, parent, key, JS_DupValue(ctx, value)) >= 0;
      if (!ok) JS_FreeValue(ctx, JS_GetException(ctx));
    }
    JS_FreeValue(ctx, parent);
  } else {
    // RPG Maker: $gameSwitches.setValue(N, v) / $gameVariables.setValue(N, v)
    JSValue store = rpg_store(ctx, s);
    if (JS_IsObject(store)) {
      JSValue fn = JS_GetPropertyStr(ctx, store, "setValue");
      if (JS_IsFunction(ctx, fn)) {
        JSValue v = s->kind == SYM_SWITCH ? JS_NewBool(ctx, JS_ToBool(ctx, value)) : JS_DupValue(ctx, value);
        JSValue args[2] = { JS_NewInt32(ctx, s->index), v };
        JSValue r = JS_Call(ctx, fn, store, 2, args);
        ok = !JS_IsException(r);
        if (!ok) JS_FreeValue(ctx, JS_GetException(ctx));
        JS_FreeValue(ctx, r);
        JS_FreeValue(ctx, v);
      }
      JS_FreeValue(ctx, fn);
    }
    JS_FreeValue(ctx, store);
  }
  if (ok) read_symbol(ctx, s, i);
  return ok ? 0 : -1;
}

// ---------- la consola ----------

static void value_text(const SymValue *v, char *out, size_t room) {
  if (v->type == SV_NUM) snprintf(out, room, "%g", v->num);
  else if (v->type == SV_STR) snprintf(out, room, "\"%s\"", v->str);
  else snprintf(out, room, "missing");
}

size_t symbols_describe(char *out, size_t room) {
  size_t n = 0;
  if (room) out[0] = 0;
  for (int i = 0; i < count && n < room; i++) {
    char now[80], prev[80];
    value_text(&symbols[i].now, now, sizeof now);
    value_text(&symbols[i].prev, prev, sizeof prev);
    n += (size_t)snprintf(out + n, room - n, "symbol %s = %s (prev %s)\n", symbols[i].name, now, prev);
  }
  return n < room ? n : room;
}
