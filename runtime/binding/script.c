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

// Los <script> del documento. Ver script.h.

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "core/app_context.h"
#include "core/scripts/load.h"
#include "core/timer.h"
#include "core/url/url.h"
#include "events.h"
#include "game/hooks/hooks.h"
#include "module.h"
#include "script.h"

static JSValue static_scripts = { 0 };   // <script> de [scripts] del toml del juego
static int     static_ready = 0;

static void set_current_script(JSContext *ctx, JSValueConst el) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue document = JS_GetPropertyStr(ctx, global, "document");
  JS_SetPropertyStr(ctx, document, "currentScript", JS_DupValue(ctx, el));
  JS_FreeValue(ctx, document);
  JS_FreeValue(ctx, global);
}

void script_add_static(JSContext *ctx, char **srcs, int count) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue document = JS_GetPropertyStr(ctx, global, "document");
  JSValue body = JS_GetPropertyStr(ctx, document, "body");
  JSValue append = JS_GetPropertyStr(ctx, body, "appendChild");
  JSValue create = JS_GetPropertyStr(ctx, document, "createElement");

  static_scripts = JS_NewArray(ctx);
  static_ready = 1;
  for (int i = 0; i < count; i++) {
    JSValue tag = JS_NewString(ctx, "script");
    JSValue el = JS_Call(ctx, create, document, 1, &tag);
    JS_FreeValue(ctx, tag);
    if (JS_IsException(el)) {
      js_std_dump_error(ctx);
      continue;
    }
    // ya se ejecuta desde el bundle: appendChild no lo vuelve a cargar
    JS_DefinePropertyValueStr(ctx, el, "__quixStarted", JS_TRUE, JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE);
    JS_SetPropertyStr(ctx, el, "type", JS_NewString(ctx, "text/javascript"));
    JS_SetPropertyStr(ctx, el, "src", JS_NewString(ctx, srcs[i]));
    JSValue r = JS_Call(ctx, append, body, 1, (JSValueConst *)&el);
    JS_FreeValue(ctx, r);
    JS_SetPropertyUint32(ctx, static_scripts, i, el);
  }

  JS_FreeValue(ctx, create);
  JS_FreeValue(ctx, append);
  JS_FreeValue(ctx, body);
  JS_FreeValue(ctx, document);
  JS_FreeValue(ctx, global);
}

void script_before_static(JSContext *ctx, int index) {
  if (!static_ready) return;
  JSValue el = JS_GetPropertyUint32(ctx, static_scripts, index);
  set_current_script(ctx, el);
  JS_FreeValue(ctx, el);
}

void script_free(JSContext *ctx) {
  if (static_ready) JS_FreeValue(ctx, static_scripts);
  static_ready = 0;
}

static int is_script(JSContext *ctx, JSValueConst el) {
  JSValue tag = JS_GetPropertyStr(ctx, el, "tagName");
  const char *t = JS_IsString(tag) ? JS_ToCString(ctx, tag) : NULL;
  int r = t && strcasecmp(t, "script") == 0;
  if (t) JS_FreeCString(ctx, t);
  JS_FreeValue(ctx, tag);
  return r;
}

static void dispatch(JSContext *ctx, JSValueConst target, const char *type) {
  JSValue ev = events_new(ctx, type);
  events_dispatch(ctx, target, ev);
  JS_FreeValue(ctx, ev);
}

// Ruta del script relativa a game_dir ("js/plugins/X.js"), sin los mods:
// scripts_run_file la lee con url_resolve (el del mod si lo trae)
static int script_path(JSContext *ctx, const char *src, char *out, size_t size) {
  AppContext *app = JS_GetContextOpaque(ctx);
  char path[1024];
  if (url_resolve_game(app->cfg->game.game_dir, src, path, sizeof(path)) != URL_FILE) return -1;
  // game_dir tal cual o absoluto: un file:///.../gamedata/x.js (nw.js) da
  // la ruta absoluta
  char abs_dir[PATH_MAX];
  const char *dirs[2] = { app->cfg->game.game_dir, realpath(app->cfg->game.game_dir, abs_dir) ? abs_dir : NULL };
  const char *rel = NULL;
  for (int i = 0; i < 2 && !rel; i++) {
    size_t gl = dirs[i] ? strlen(dirs[i]) : 0;
    if (gl && strncmp(path, dirs[i], gl) == 0 && path[gl] == '/') rel = path + gl + 1;
  }
  if (rel) {
    snprintf(out, size, "%s", rel);
  } else {
    // ruta fuera de game_dir: no se carga (el script recibe error)
    return -1;
  }
  return 0;
}

static JSValue js_script_load(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv,
                              int magic, JSValue *data) {
  AppContext *app = JS_GetContextOpaque(ctx);
  JSValue el = data[0];
  const char *src = JS_ToCString(ctx, data[1]);
  if (!src) return JS_EXCEPTION;

  char rel[1024];
  int r = -1;
  set_current_script(ctx, el);
  if (script_path(ctx, src, rel, sizeof(rel)) == 0) {
    // type="module" (Construct 3): sus import antes (binding/module.c)
    JSValue type = JS_GetPropertyStr(ctx, el, "type");
    const char *t = JS_IsString(type) ? JS_ToCString(ctx, type) : NULL;
    int module = t && strcasecmp(t, "module") == 0;
    if (t) JS_FreeCString(ctx, t);
    JS_FreeValue(ctx, type);
    r = module ? module_run_file(ctx, app->cfg, rel) : scripts_run_file(ctx, app->cfg, rel);
  }
  set_current_script(ctx, JS_NULL);
  if (r == 1) r = 0;   // error de sintaxis, ya escrito: load como el navegador
  if (r == -2) {
    // un error dentro del script no impide el evento load (como el navegador)
    fprintf(stderr, "Error in script %s:\n", src);
    js_std_dump_error(ctx);
    r = 0;
  }
  if (r != 0) fprintf(stderr, "[script] failed to load: %s\n", src);
  JS_FreeCString(ctx, src);

  dispatch(ctx, el, r == 0 ? "load" : "error");
  return JS_UNDEFINED;
}

void script_maybe_run(JSContext *ctx, JSValueConst el) {
  if (!is_script(ctx, el)) return;

  JSValue done = JS_GetPropertyStr(ctx, el, "__quixStarted");
  int started = JS_ToBool(ctx, done);
  JS_FreeValue(ctx, done);
  if (started) return;   // un script se ejecuta una sola vez
  JS_DefinePropertyValueStr(ctx, el, "__quixStarted", JS_TRUE, JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE);

  JSValue src = JS_GetPropertyStr(ctx, el, "src");
  const char *s = JS_IsString(src) ? JS_ToCString(ctx, src) : NULL;
  int has_src = s && *s;
  if (s) JS_FreeCString(ctx, s);

  if (has_src) {
    JSValue data[2] = { JS_DupValue(ctx, el), src };
    JSValue fn = JS_NewCFunctionData(ctx, js_script_load, 0, 0, 2, data);
    timer_set(ctx, fn, 0, 0, 0, NULL);
    JS_FreeValue(ctx, fn);
    JS_FreeValue(ctx, data[0]);
    JS_FreeValue(ctx, src);
    return;
  }
  JS_FreeValue(ctx, src);

  // script en linea: se ejecuta ya
  JSValue text = JS_GetPropertyStr(ctx, el, "textContent");
  const char *code = JS_IsString(text) ? JS_ToCString(ctx, text) : NULL;
  if (code && *code) {
    JSValue r = JS_Eval(ctx, code, strlen(code), "<inline script>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(r)) js_std_dump_error(ctx);
    JS_FreeValue(ctx, r);
  }
  if (code) JS_FreeCString(ctx, code);
  JS_FreeValue(ctx, text);
}

static JSValue js_document_loaded(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  set_current_script(ctx, JS_NULL);
  // los hooks de lo que crearon los <script> del arranque, antes de load: RPG
  // Maker MZ llama en window load a effekseer.initRuntime, de un <script>
  // (si no, el hook llegaba en la imagen siguiente, tarde)
  if (hooks_pending()) hooks_install(ctx);
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue document = JS_GetPropertyStr(ctx, global, "document");
  JS_SetPropertyStr(ctx, document, "readyState", JS_NewString(ctx, "interactive"));
  dispatch(ctx, document, "DOMContentLoaded");
  JS_SetPropertyStr(ctx, document, "readyState", JS_NewString(ctx, "complete"));
  dispatch(ctx, document, "readystatechange");
  dispatch(ctx, global, "load");
  JS_FreeValue(ctx, document);
  JS_FreeValue(ctx, global);
  return JS_UNDEFINED;
}

void script_document_loaded(JSContext *ctx) {
  // despues de los <script src> que se agregaron durante el arranque
  JSValue fn = JS_NewCFunction(ctx, js_document_loaded, "documentLoaded", 0);
  timer_set(ctx, fn, 0, 0, 0, NULL);
  JS_FreeValue(ctx, fn);
}
