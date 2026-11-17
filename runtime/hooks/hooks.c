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

// Los hooks de [hooks]. Ver hooks.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tomlc17.h"
#include "binding/canvas.h"
#include "hooks.h"
#include "engines.h"   // runtime/engines/<motor>/*.js, generado por tools/embed_engines.py

#define MAX_HOOKS 64

typedef enum { HOOK_GPU_CANVAS, HOOK_SKIP, HOOK_ONCE_PER_FRAME, HOOK_ENGINE } HookKind;

typedef struct {
  char        path[256];
  HookKind    kind;
  const char *engine_name;     // HOOK_ENGINE: "rpgmaker-mv/request-images-once"
  const char *engine_source;   // la funcion que recibe la original
  int         installed;
} Hook;

static Hook hooks[MAX_HOOKS];
static int  count = 0;
static int  installed = 0;

static const char *kind_names[] = { "gpu-canvas", "skip", "once-per-frame" };

// Anade un hook (la ruta del juego y lo que se pone). 0 si vale, -1 si no
static int add_hook(const char *key, toml_datum_t v) {
  int kind = -1;
  const EngineHook *eh = NULL;
  for (int k = 0; k < 3 && v.type == TOML_STRING; k++) {
    if (strcmp(v.u.s, kind_names[k]) == 0) kind = k;
  }
  // "motor/hook": uno de runtime/engines/
  for (int k = 0; v.type == TOML_STRING && kind < 0 && ENGINE_HOOKS[k].name; k++) {
    if (strcmp(v.u.s, ENGINE_HOOKS[k].name) == 0) {
      kind = HOOK_ENGINE;
      eh = &ENGINE_HOOKS[k];
    }
  }
  // la ruta: solo nombres y puntos (va dentro del JS que la pone)
  int path_ok = key[0] != 0 && strlen(key) < sizeof(hooks[0].path);
  for (const char *p = key; *p && path_ok; p++) {
    if (!(*p == '.' || *p == '_' || *p == '$' || (*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'z') ||
          (*p >= 'A' && *p <= 'Z'))) {
      path_ok = 0;
    }
  }
  if (kind < 0 || !path_ok) {
    fprintf(stderr, "[hooks] %s: expected \"gpu-canvas\", \"skip\", \"once-per-frame\", \"off\" or an engine hook (%s...)\n",
            key, ENGINE_HOOKS[0].name ? ENGINE_HOOKS[0].name : "none");
    return -1;
  }
  if (count >= MAX_HOOKS) {
    fprintf(stderr, "[hooks] %s: more than %d hooks, left out\n", key, MAX_HOOKS);
    return -1;
  }
  snprintf(hooks[count].path, sizeof(hooks[count].path), "%s", key);
  hooks[count].kind = (HookKind)kind;
  hooks[count].engine_name = eh ? eh->name : NULL;
  hooks[count].engine_source = eh ? eh->source : NULL;
  hooks[count].installed = 0;
  count++;
  return 0;
}

// lo que dice el toml del juego para una ruta (o un datum vacio)
static toml_datum_t own_value(toml_datum_t t, const char *key) {
  toml_datum_t none = {0};
  if (t.type != TOML_TABLE) return none;
  for (int i = 0; i < t.u.tab.size; i++) {
    if (strcmp(t.u.tab.key[i], key) == 0) return t.u.tab.value[i];
  }
  return none;
}

static int is_off(toml_datum_t v) {
  return v.type == TOML_STRING && strcmp(v.u.s, "off") == 0;
}

int hooks_load(const char *config_path) {
  count = installed = 0;
  FILE *f = fopen(config_path, "r");
  if (!f) return 0;
  toml_result_t result = toml_parse_file(f);
  fclose(f);
  if (!result.ok) return 0;
  int bad = 0;
  toml_datum_t t = toml_get(result.toptab, "hooks");
  // engine = "rpgmaker-mv": los hooks del motor (runtime/engines/<motor>/
  // hooks.toml). Los que el juego pone en su [hooks] mandan: otro hook en la
  // misma ruta, o "off" para no usarlo
  toml_datum_t engine = own_value(t, "engine");
  if (engine.type == TOML_STRING) {
    const char *preset = NULL;
    for (int k = 0; ENGINE_PRESETS[k].engine; k++) {
      if (strcmp(ENGINE_PRESETS[k].engine, engine.u.s) == 0) preset = ENGINE_PRESETS[k].hooks;
    }
    if (!preset) {
      fprintf(stderr, "[hooks] engine = \"%s\": no hooks for that engine\n", engine.u.s);
      bad = 1;
    } else {
      toml_result_t pr = toml_parse(preset, (int)strlen(preset));
      toml_datum_t pt = pr.ok ? toml_get(pr.toptab, "hooks") : (toml_datum_t){0};
      for (int i = 0; pt.type == TOML_TABLE && i < pt.u.tab.size; i++) {
        const char *key = pt.u.tab.key[i];
        if (own_value(t, key).type != TOML_UNKNOWN) continue;   // el juego manda
        if (add_hook(key, pt.u.tab.value[i]) != 0) bad = 1;
      }
      if (pr.ok) toml_free(pr);
    }
  }
  if (t.type == TOML_TABLE) {
    for (int i = 0; i < t.u.tab.size; i++) {
      const char *key = t.u.tab.key[i];
      toml_datum_t v = t.u.tab.value[i];
      if (strcmp(key, "engine") == 0 || is_off(v)) continue;
      if (add_hook(key, v) != 0) bad = 1;
    }
  }
  toml_free(result);
  if (count) printf("Hooks: %d\n", count);
  return bad ? -1 : count;
}

static JSValue js_gpu_scope(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  int32_t d = 0;
  if (argc > 0) JS_ToInt32(ctx, &d, argv[0]);
  canvas_gpu_scope(d);
  return JS_UNDEFINED;
}

void hooks_add_api(JSContext *ctx) {
  JSValue global = JS_GetGlobalObject(ctx);
  JS_DefinePropertyValueStr(ctx, global, "__quixGpuScope", JS_NewCFunction(ctx, js_gpu_scope, "__quixGpuScope", 1),
                            JS_PROP_CONFIGURABLE);
  JS_FreeValue(ctx, global);
}

int hooks_pending(void) {
  return installed < count;
}

// El JS que envuelve la funcion. Devuelve 1 si la ha puesto, 0 si aun no existe.
static const char *wrapper(HookKind kind) {
  switch (kind) {
    case HOOK_GPU_CANVAS:
      return "function () { __quixGpuScope(1); try { return f.apply(this, arguments); }"
             " finally { __quixGpuScope(-1); } }";
    case HOOK_SKIP:
      return "function () {}";
    default:
      return "function () { var fr = quix.frame; if (g.__quixLast === fr) return; g.__quixLast = fr;"
             " return f.apply(this, arguments); }";
  }
}

// Un plugin puede cambiar la funcion despues de poner el hook (por ejemplo
// TDDP_FluidTimestep cambia SceneManager.updateMain al cargarse): el hook se
// pierde. Cada cierto tiempo se mira si cada hook sigue puesto, con un solo
// JS pequeno, y los que se han perdido se vuelven a poner sobre la funcion
// nueva.
void hooks_check(JSContext *ctx) {
  if (count == 0) return;
  char code[16384];
  int n = snprintf(code, sizeof(code),
                   "(function () { var out = '';"
                   " function on(p) { var parts = p.split('.'), o = globalThis;"
                   "  for (var i = 0; i < parts.length - 1; i++) { o = o[parts[i]]; if (o == null) return true; }"
                   "  var f = o[parts[parts.length - 1]]; return typeof f !== 'function' || !!f.__quixHook; }");
  for (int i = 0; i < count && n < (int)sizeof(code) - 512; i++) {
    if (!hooks[i].installed) continue;
    n += snprintf(code + n, sizeof(code) - n, " if (!on('%s')) out += '%d,';", hooks[i].path, i);
  }
  snprintf(code + n, sizeof(code) - n, " return out; })()");
  JSValue r = JS_Eval(ctx, code, strlen(code), "<hooks-check>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(r)) {
    JS_FreeValue(ctx, JS_GetException(ctx));
    return;
  }
  const char *lost = JS_ToCString(ctx, r);
  JS_FreeValue(ctx, r);
  if (!lost) return;
  int again = 0;
  for (const char *p = lost; *p; ) {
    int i = atoi(p);
    if (i >= 0 && i < count && hooks[i].installed) {
      hooks[i].installed = 0;
      installed--;
      again = 1;
      printf("[hooks] %s: replaced by the game, putting the hook back\n", hooks[i].path);
    }
    p = strchr(p, ',');
    if (!p) break;
    p++;
  }
  JS_FreeCString(ctx, lost);
  if (again) hooks_install(ctx);
}

int hooks_install(JSContext *ctx) {
  for (int i = 0; i < count; i++) {
    Hook *h = &hooks[i];
    if (h->installed) continue;
    if (h->kind == HOOK_ENGINE) {
      // la funcion del archivo recibe la original y devuelve la nueva
      size_t room = strlen(h->engine_source) + 1024;
      char *code = malloc(room);
      if (!code) continue;
      snprintf(code, room,
               "(function () {"
               " var parts = '%s'.split('.'), o = globalThis;"
               " for (var i = 0; i < parts.length - 1; i++) { o = o[parts[i]]; if (o == null) return 0; }"
               " var k = parts[parts.length - 1], f = o[k];"
               " if (typeof f !== 'function') return 0;"
               " if (f.__quixHook) return 1;"
               " var g = (%s)(f);"
               " g.__quixHook = '%s'; o[k] = g; return 1; })()",
               h->path, h->engine_source, h->engine_name);
      JSValue r = JS_Eval(ctx, code, strlen(code), h->engine_name, JS_EVAL_TYPE_GLOBAL);
      free(code);
      int ok = 0;
      if (JS_IsException(r)) {
        JSValue e = JS_GetException(ctx);
        const char *msg = JS_ToCString(ctx, e);
        fprintf(stderr, "[hooks] %s: %s\n", h->engine_name, msg ? msg : "error");
        if (msg) JS_FreeCString(ctx, msg);
        JS_FreeValue(ctx, e);
        h->installed = 1;   // no se reintenta
        installed++;
      } else {
        ok = JS_ToBool(ctx, r);
      }
      JS_FreeValue(ctx, r);
      if (ok) {
        h->installed = 1;
        installed++;
        printf("[hooks] %s: %s\n", h->path, h->engine_name);
      }
      continue;
    }
    char code[1024];
    snprintf(code, sizeof(code),
             "(function () {"
             " var parts = '%s'.split('.'), o = globalThis;"
             " for (var i = 0; i < parts.length - 1; i++) { o = o[parts[i]]; if (o == null) return 0; }"
             " var k = parts[parts.length - 1], f = o[k];"
             " if (typeof f !== 'function') return 0;"
             " if (f.__quixHook) return 1;"
             " var g = %s;"
             " g.__quixHook = '%s'; o[k] = g; return 1; })()",
             h->path, wrapper(h->kind), kind_names[h->kind]);
    JSValue r = JS_Eval(ctx, code, strlen(code), "<hooks>", JS_EVAL_TYPE_GLOBAL);
    int ok = 0;
    if (JS_IsException(r)) {
      JS_FreeValue(ctx, JS_GetException(ctx));
    } else {
      ok = JS_ToBool(ctx, r);
    }
    JS_FreeValue(ctx, r);
    if (ok) {
      h->installed = 1;
      installed++;
      printf("[hooks] %s: %s\n", h->path, kind_names[h->kind]);
    }
  }
  return installed == count;
}

const char *hooks_engine_source(const char *name) {
  for (int k = 0; ENGINE_HOOKS[k].name; k++) {
    if (strcmp(ENGINE_HOOKS[k].name, name) == 0) return ENGINE_HOOKS[k].source;
  }
  return NULL;
}

const char *hooks_engine_file(int i, const char **name) {
  for (int k = 0; ENGINE_HOOKS[k].name; k++) {
    if (k == i) {
      if (name) *name = ENGINE_HOOKS[k].name;
      return ENGINE_HOOKS[k].source;
    }
  }
  return NULL;
}
