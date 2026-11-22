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

// Los hooks de [hooks]. Ver hooks.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tomlc17.h"
#include "binding/canvas.h"
#include "hooks.h"
#include "core/sandbox.h"
#include "engines.h"   // runtime/engines/<motor>/*.js, generado por tools/embed_engines.py

#define MAX_HOOKS 128

typedef enum { HOOK_GPU_CANVAS, HOOK_SKIP, HOOK_ONCE_PER_FRAME, HOOK_ENGINE } HookKind;

typedef struct {
  char        path[256];
  HookKind    kind;
  const char *engine_name;     // HOOK_ENGINE: "rpgmaker-mv/request-images-once"
  const char *engine_source;   // la funcion que recibe la original
  int         installed;
  // el hook de un mod (hooks_load_mods): su nombre ("rumble/rumble.js") y su
  // codigo, leido del archivo (engine_name y engine_source apuntan aqui). Va
  // encima de otro hook de la misma ruta
  char        mod_name[128];
  char       *mod_source;
} Hook;

static Hook hooks[MAX_HOOKS];
static int  count = 0;
static int  installed = 0;

// los scripts de los mods (scripts = [...] de su hooks.toml): corren una vez,
// en orden, antes de poner sus hooks (lo que comparten los archivos de un mod)
#define MAX_MOD_SCRIPTS 32
typedef struct {
  char  name[128];   // "multiplayer/net.js"
  char *source;
  int   ran;
} ModScript;
static ModScript mod_scripts[MAX_MOD_SCRIPTS];
static int       mod_script_count = 0;

static const char *kind_names[] = { "gpu-canvas", "skip", "once-per-frame" };

// la ruta de la funcion: solo nombres y puntos (va dentro del JS que la pone)
static int path_ok(const char *key) {
  if (key[0] == 0 || strlen(key) >= sizeof(hooks[0].path)) return 0;
  for (const char *p = key; *p; p++) {
    if (!(*p == '.' || *p == '_' || *p == '$' || (*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'z') ||
          (*p >= 'A' && *p <= 'Z'))) {
      return 0;
    }
  }
  return 1;
}

// un sitio nuevo en la lista para la ruta key, o NULL si no caben mas
static Hook *new_hook(const char *key) {
  if (count >= MAX_HOOKS) {
    fprintf(stderr, "[hooks] %s: more than %d hooks, left out\n", key, MAX_HOOKS);
    return NULL;
  }
  Hook *h = &hooks[count++];
  memset(h, 0, sizeof *h);
  snprintf(h->path, sizeof(h->path), "%s", key);
  return h;
}

// Agrega un hook (la ruta del juego y lo que se pone). 0 si vale, -1 si no
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
  if (kind < 0 || !path_ok(key)) {
    fprintf(stderr, "[hooks] %s: expected \"gpu-canvas\", \"skip\", \"once-per-frame\", \"off\" or an engine hook (%s...)\n",
            key, ENGINE_HOOKS[0].name ? ENGINE_HOOKS[0].name : "none");
    return -1;
  }
  Hook *h = new_hook(key);
  if (!h) return -1;
  h->kind = (HookKind)kind;
  h->engine_name = eh ? eh->name : NULL;
  h->engine_source = eh ? eh->source : NULL;
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

// lo de los hooks de los mods de la carga anterior
static void forget_hooks(void) {
  for (int i = 0; i < count; i++) {
    free(hooks[i].mod_source);
    hooks[i].mod_source = NULL;
  }
  count = installed = 0;
  for (int i = 0; i < mod_script_count; i++) free(mod_scripts[i].source);
  mod_script_count = 0;
}

int hooks_load(const char *config_path) {
  forget_hooks();
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

// el archivo entero (malloc), o NULL
static char *read_file(const char *path) {
  // un enlace simbolico del mod que apunta fuera del port tampoco vale
  if (!sandbox_check(path, "mod script")) return NULL;
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *text = size >= 0 ? malloc((size_t)size + 1) : NULL;
  if (text && fread(text, 1, (size_t)size, f) != (size_t)size) {
    free(text);
    text = NULL;
  }
  if (text) text[size] = 0;
  fclose(f);
  return text;
}

// un nombre que va dentro del JS que pone el hook: letras, numeros, . _ - y /
// (sin "..")
static int safe_name(const char *s) {
  if (!s[0] || strstr(s, "..")) return 0;
  for (; *s; s++) {
    if (!(*s == '.' || *s == '_' || *s == '-' || *s == '/' || (*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'z') ||
          (*s >= 'A' && *s <= 'Z'))) {
      return 0;
    }
  }
  return 1;
}

int hooks_load_mods(const char *root, const char (*names)[64], int n) {
  int added = 0, bad = 0;
  for (int m = 0; m < n; m++) {
    char path[1024];
    snprintf(path, sizeof path, "%s/%s/hooks.toml", root, names[m]);
    FILE *f = fopen(path, "r");
    if (!f) continue;
    toml_result_t result = toml_parse_file(f);
    fclose(f);
    if (!result.ok) {
      fprintf(stderr, "[hooks] %s: %s\n", path, result.errmsg);
      bad = 1;
      continue;
    }
    // scripts = ["net.js", ...]: los archivos del mod que no son hooks
    toml_datum_t sl = toml_get(result.toptab, "scripts");
    for (int i = 0; sl.type == TOML_ARRAY && i < sl.u.arr.size; i++) {
      toml_datum_t v = sl.u.arr.elem[i];
      char name[128], file[1024];
      if (v.type != TOML_STRING || snprintf(name, sizeof name, "%s/%s", names[m], v.u.s) >= (int)sizeof name ||
          !safe_name(name)) {
        fprintf(stderr, "[hooks] %s: scripts: expected .js files of the mod\n", path);
        bad = 1;
        continue;
      }
      snprintf(file, sizeof file, "%s/%s", root, name);
      char *source = read_file(file);
      if (!source || mod_script_count >= MAX_MOD_SCRIPTS) {
        fprintf(stderr, "[hooks] %s: cannot read %s\n", path, file);
        free(source);
        bad = 1;
        continue;
      }
      ModScript *s = &mod_scripts[mod_script_count++];
      snprintf(s->name, sizeof s->name, "%s", name);
      s->source = source;
      s->ran = 0;
    }
    toml_datum_t t = toml_get(result.toptab, "hooks");
    for (int i = 0; t.type == TOML_TABLE && i < t.u.tab.size; i++) {
      const char *key = t.u.tab.key[i];
      toml_datum_t v = t.u.tab.value[i];
      char name[128];
      if (!path_ok(key) || v.type != TOML_STRING ||
          snprintf(name, sizeof name, "%s/%s", names[m], v.u.s) >= (int)sizeof name || !safe_name(name)) {
        fprintf(stderr, "[hooks] %s: \"%s\": expected the path of a function and a .js file of the mod\n", path, key);
        bad = 1;
        continue;
      }
      char file[1024];
      snprintf(file, sizeof file, "%s/%s", root, name);
      char *source = read_file(file);
      if (!source) {
        fprintf(stderr, "[hooks] %s: cannot read %s\n", key, file);
        bad = 1;
        continue;
      }
      Hook *h = new_hook(key);
      if (!h) {
        free(source);
        bad = 1;
        continue;
      }
      snprintf(h->mod_name, sizeof h->mod_name, "%s", name);
      h->mod_source = source;
      h->kind = HOOK_ENGINE;
      h->engine_name = h->mod_name;
      h->engine_source = h->mod_source;
      added++;
    }
    toml_free(result);
  }
  if (added) printf("Hooks of the mods: %d\n", added);
  return bad ? -1 : added;
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

int hooks_pending(void) {
  for (int i = 0; i < mod_script_count; i++) {
    if (!mod_scripts[i].ran) return 1;
  }
  return installed < count;
}

// los scripts de los mods que aun no corrieron (la primera vez que se ponen
// hooks: el juego ya cargo sus scripts)
static void run_mod_scripts(JSContext *ctx) {
  for (int i = 0; i < mod_script_count; i++) {
    ModScript *s = &mod_scripts[i];
    if (s->ran) continue;
    s->ran = 1;
    JSValue r = JS_Eval(ctx, s->source, strlen(s->source), s->name, JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(r)) {
      JSValue e = JS_GetException(ctx);
      const char *msg = JS_ToCString(ctx, e);
      fprintf(stderr, "[hooks] %s: %s\n", s->name, msg ? msg : "error");
      if (msg) JS_FreeCString(ctx, msg);
      JS_FreeValue(ctx, e);
    } else {
      printf("[hooks] script %s\n", s->name);
    }
    JS_FreeValue(ctx, r);
  }
}

// El JS que envuelve la funcion. Devuelve 1 si la ha puesto, 0 si aun no existe.
static const char *wrapper(HookKind kind) {
  switch (kind) {
    case HOOK_GPU_CANVAS:
      return "function () { quix.gpu.scope(1); try { return f.apply(this, arguments); }"
             " finally { quix.gpu.scope(-1); } }";
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
  run_mod_scripts(ctx);
  for (int i = 0; i < count; i++) {
    Hook *h = &hooks[i];
    if (h->installed) continue;
    if (h->kind == HOOK_ENGINE) {
      // la funcion del archivo recibe la original (y la ruta, por si el
      // mismo archivo sirve para varias) y devuelve la nueva
      // el de un mod va encima de otro hook (el del motor en esa ruta): solo
      // no se pone si ya esta el suyo
      char mine[192];
      snprintf(mine, sizeof mine, h->mod_source ? "f.__quixHook === '%s'" : "f.__quixHook", h->engine_name);
      size_t room = strlen(h->engine_source) + 1024;
      char *code = malloc(room);
      if (!code) continue;
      snprintf(code, room,
               "(function () {"
               " var parts = '%s'.split('.'), o = globalThis;"
               " for (var i = 0; i < parts.length - 1; i++) { o = o[parts[i]]; if (o == null) return 0; }"
               " var k = parts[parts.length - 1], f = o[k];"
               " if (typeof f !== 'function') return 0;"
               " if (%s) return 1;"
               " var g = (%s)(f, '%s');"
               " g.__quixHook = '%s'; o[k] = g; return 1; })()",
               h->path, mine, h->engine_source, h->path, h->engine_name);
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
