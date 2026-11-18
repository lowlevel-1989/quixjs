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

// Ejecuta los scripts del juego. Ver runner.h.

#include <stdio.h>
#include <string.h>
#include "runner.h"

int runner_run_bundle(JSContext *ctx, const ScriptBundle *bundle, void (*before)(JSContext *ctx, int index)) {
  for (int i = 0; i < bundle->count; i++) {
    const char *name = bundle->filenames[i];
    printf("[%02d] %s\n", i, strncmp(name, "data:", 5) == 0 ? "inline script" : name);
    if (!bundle->sources[i]) {
      fprintf(stderr, "Error: %s was not loaded\n", bundle->filenames[i]);
      return -1;
    }
    if (before) before(ctx, i);
    JSValue val = JS_Eval(ctx, bundle->sources[i], bundle->sizes[i], bundle->paths[i], JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(val)) {
      fprintf(stderr, "Error executing %s:\n", bundle->filenames[i]);
      js_std_dump_error(ctx);
      return -1;
    }
    JS_FreeValue(ctx, val);
  }
  return 0;
}
