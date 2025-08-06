/*
 * QuixJS - A lightweight JS runtime using SDL2 and QuickJS
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

#include <stdio.h>
#include "runner.h"

int runner_run_bundle(JSContext *ctx, const BytecodeBundle *bundle) {
  for (int i = 0; i < bundle->count; i++) {
    printf("[%02d] %s\n", i, bundle->filenames[i]);
    js_std_eval_binary(ctx, bundle->bytecodes[i], bundle->sizes[i], 0);

    JSValue ex = JS_GetException(ctx);
    if (JS_IsException(ex)) {
      const char *err = JS_ToCString(ctx, ex);
      fprintf(stderr, "Error executing bytecode %d: %s\n", i, err);
      JS_FreeCString(ctx, err);
      JS_FreeValue(ctx, ex);
      return -1;
    }
    JS_FreeValue(ctx, ex);
  }
  return 0;
}

