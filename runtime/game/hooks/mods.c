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

// runtime/game/hooks/mods.c: los mods (mods/<nombre>/hooks.toml): sus hooks,
// que van encima de los del juego, y sus scripts (scripts = [...]), que
// corren una vez, en orden, antes de poner los hooks.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tomlc17.h"
#include "core/sandbox.h"
#include "common.h"

int hooks_load_mods(const char *root, const char (*names)[64], int n) {
  int added = 0, bad = 0;
  for (int m = 0; m < n; m++) {
    char path[1024];
    snprintf(path, sizeof path, "%s/%s/hooks.toml", root, names[m]);
    // un enlace simbolico que apunte fuera del port no vale
    FILE *f = sandbox_fopen(path, "r", "mod hooks.toml");
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
          !hooks_safe_name(name)) {
        fprintf(stderr, "[hooks] %s: scripts: expected .js files of the mod\n", path);
        bad = 1;
        continue;
      }
      snprintf(file, sizeof file, "%s/%s", root, name);
      char *source = hooks_read_file(file);
      if (!source || hooks_mod_script_count >= MAX_MOD_SCRIPTS) {
        fprintf(stderr, "[hooks] %s: cannot read %s\n", path, file);
        free(source);
        bad = 1;
        continue;
      }
      ModScript *s = &hooks_mod_scripts[hooks_mod_script_count++];
      snprintf(s->name, sizeof s->name, "%s", name);
      s->source = source;
      s->ran = 0;
    }
    toml_datum_t t = toml_get(result.toptab, "hooks");
    for (int i = 0; t.type == TOML_TABLE && i < t.u.tab.size; i++) {
      const char *key = t.u.tab.key[i];
      toml_datum_t v = t.u.tab.value[i];
      char name[128];
      if (!hooks_path_ok(key) || v.type != TOML_STRING ||
          snprintf(name, sizeof name, "%s/%s", names[m], v.u.s) >= (int)sizeof name || !hooks_safe_name(name)) {
        fprintf(stderr, "[hooks] %s: \"%s\": expected the path of a function and a hook of the mod (its .js file without .js)\n", path, key);
        bad = 1;
        continue;
      }
      // "rumble" es mods/<mod>/rumble.js
      char file[1024];
      snprintf(file, sizeof file, "%s/%s.js", root, name);
      char *source = hooks_read_file(file);
      if (!source) {
        fprintf(stderr, "[hooks] %s: cannot read %s\n", key, file);
        bad = 1;
        continue;
      }
      Hook *h = hooks_new_hook(key);
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

// los scripts de los mods que aun no corrieron (la primera vez que se ponen
// hooks: el juego ya cargo sus scripts)
void hooks_run_mod_scripts(JSContext *ctx) {
  for (int i = 0; i < hooks_mod_script_count; i++) {
    ModScript *s = &hooks_mod_scripts[i];
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
