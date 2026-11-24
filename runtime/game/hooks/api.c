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

// runtime/game/hooks/api.c: la API de JS de los hooks (quix.gpu.scope y
// quix.engine.lib) y los hooks de motor embebidos en el binario
// (runtime/engines/, en engines.h): solo este archivo incluye engines.h, y
// las demas partes preguntan con hooks_engine_find / preset / example.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "binding/canvas.h"
#include "common.h"
#include "engines.h"   // runtime/engines/<motor>/*.js, generado por tools/embed_engines.py

int hooks_engine_find(const char *name, const char **out_name, const char **out_source) {
  for (int k = 0; ENGINE_HOOKS[k].name; k++) {
    if (strcmp(ENGINE_HOOKS[k].name, name) != 0) continue;
    *out_name = ENGINE_HOOKS[k].name;
    *out_source = ENGINE_HOOKS[k].source;
    return 1;
  }
  return 0;
}

const char *hooks_engine_preset(const char *engine) {
  for (int k = 0; ENGINE_PRESETS[k].engine; k++) {
    if (strcmp(ENGINE_PRESETS[k].engine, engine) == 0) return ENGINE_PRESETS[k].hooks;
  }
  return NULL;
}

const char *hooks_engine_example(void) {
  return ENGINE_HOOKS[0].name ? ENGINE_HOOKS[0].name : "none";
}

static JSValue js_gpu_scope(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  int32_t d = 0;
  if (argc > 0) JS_ToInt32(ctx, &d, argv[0]);
  canvas_gpu_scope(d);
  return JS_UNDEFINED;
}

// quix.gpu.scope(+1 / -1): los canvas que se crean dentro van a la GPU
void hooks_add_api(JSContext *ctx) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue quix = JS_GetPropertyStr(ctx, global, "quix");
  JSValue gpu = JS_GetPropertyStr(ctx, quix, "gpu");
  if (JS_IsObject(gpu)) JS_SetPropertyStr(ctx, gpu, "scope", JS_NewCFunction(ctx, js_gpu_scope, "scope", 1));
  JS_FreeValue(ctx, gpu);
  JS_FreeValue(ctx, quix);
  JS_FreeValue(ctx, global);
}

// quix.engine.lib(nombre): data[0] guarda los modulos ya evaluados
static JSValue js_engine_lib(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic,
                             JSValue *data) {
  (void)this_val;
  (void)magic;
  const char *name = argc > 0 ? JS_ToCString(ctx, argv[0]) : NULL;
  if (!name) return JS_UNDEFINED;
  JSValue mod = JS_GetPropertyStr(ctx, data[0], name);
  if (JS_IsUndefined(mod)) {
    for (int k = 0; ENGINE_LIBS[k].name; k++) {
      if (strcmp(ENGINE_LIBS[k].name, name) != 0) continue;
      size_t room = strlen(ENGINE_LIBS[k].source) + 16;
      char *code = malloc(room);
      if (!code) break;
      snprintf(code, room, "(%s)()", ENGINE_LIBS[k].source);
      char file[96];
      snprintf(file, sizeof(file), "lib/%s", name);
      mod = JS_Eval(ctx, code, strlen(code), file, JS_EVAL_TYPE_GLOBAL);
      free(code);
      if (JS_IsException(mod)) {
        JS_FreeCString(ctx, name);
        return mod;
      }
      JS_SetPropertyStr(ctx, data[0], name, JS_DupValue(ctx, mod));
      break;
    }
  }
  JS_FreeCString(ctx, name);
  return mod;
}

JSValue hooks_lib_function(JSContext *ctx) {
  JSValue cache = JS_NewObject(ctx);
  JSValue fn = JS_NewCFunctionData(ctx, js_engine_lib, 1, 0, 1, &cache);
  JS_FreeValue(ctx, cache);
  return fn;
}

const char *hooks_engine_source(const char *name) {
  for (int k = 0; ENGINE_HOOKS[k].name; k++) {
    if (strcmp(ENGINE_HOOKS[k].name, name) == 0) return ENGINE_HOOKS[k].source;
  }
  return NULL;
}

int hooks_run_engine_entry(JSContext *ctx, const char *engine) {
  if (!engine || !engine[0]) return 0;
  char name[96];
  snprintf(name, sizeof name, "%s/entry", engine);
  for (int k = 0; ENGINE_ENTRIES[k].name; k++) {
    if (strcmp(ENGINE_ENTRIES[k].name, engine) != 0) continue;
    JSValue r = JS_Eval(ctx, ENGINE_ENTRIES[k].source, strlen(ENGINE_ENTRIES[k].source), name, JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(r)) {
      js_std_dump_error(ctx);
      return -1;
    }
    JS_FreeValue(ctx, r);
    printf("[engine] %s\n", name);
    return 0;
  }
  return 0;
}
