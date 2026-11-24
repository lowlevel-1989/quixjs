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

// runtime/game/hooks/install.c: poner los hooks en el juego. Una funcion se
// envuelve cuando ya existe (hooks_pending / hooks_install, desde el bucle y
// al cargar scripts), y cada 30 frames se mira que siga puesta: un plugin
// puede sustituirla despues (hooks_check).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

int hooks_pending(void) {
  for (int i = 0; i < hooks_mod_script_count; i++) {
    if (!hooks_mod_scripts[i].ran) return 1;
  }
  return hooks_installed < hooks_total;
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
  if (hooks_total == 0) return;
  char code[16384];
  int n = snprintf(code, sizeof(code),
                   "(function () { var out = '';"
                   " function on(p) { var parts = p.split('.'), o = globalThis;"
                   "  for (var i = 0; i < parts.length - 1; i++) { o = o[parts[i]]; if (o == null) return true; }"
                   "  var f = o[parts[parts.length - 1]]; return typeof f !== 'function' || !!f.__quixHook; }");
  for (int i = 0; i < hooks_total && n < (int)sizeof(code) - 512; i++) {
    if (!hooks_list[i].installed) continue;
    n += snprintf(code + n, sizeof(code) - n, " if (!on('%s')) out += '%d,';", hooks_list[i].path, i);
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
    if (i >= 0 && i < hooks_total && hooks_list[i].installed) {
      hooks_list[i].installed = 0;
      hooks_installed--;
      again = 1;
      printf("[hooks] %s: replaced by the game, putting the hook back\n", hooks_list[i].path);
    }
    p = strchr(p, ',');
    if (!p) break;
    p++;
  }
  JS_FreeCString(ctx, lost);
  if (again) hooks_install(ctx);
}

int hooks_install(JSContext *ctx) {
  hooks_run_mod_scripts(ctx);
  for (int i = 0; i < hooks_total; i++) {
    Hook *h = &hooks_list[i];
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
               " g.__quixHook = '%s'; o[k] = g; return g === f ? 2 : 1; })()",
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
        hooks_installed++;
      } else {
        JS_ToInt32(ctx, &ok, r);
      }
      JS_FreeValue(ctx, r);
      if (ok) {
        h->installed = 1;
        hooks_installed++;
        // 2: el hook devolvio la original (no es la funcion que espera: un
        // plugin la cambio); queda marcada para no reintentarlo
        if (ok == 2) printf("[hooks] %s: %s kept the original (not the function it expects)\n", h->path, h->engine_name);
        else printf("[hooks] %s: %s\n", h->path, h->engine_name);
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
             h->path, wrapper(h->kind), hooks_kind_names[h->kind]);
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
      hooks_installed++;
      printf("[hooks] %s: %s\n", h->path, hooks_kind_names[h->kind]);
    }
  }
  return hooks_installed == hooks_total;
}
