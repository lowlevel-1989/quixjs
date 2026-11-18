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

// console (log, info, debug, warn, error...) y print: los argumentos
// convertidos a texto y separados por un espacio. Escriben lo mismo que en la
// primera version de quixjs, cuando los daba la libreria de QuickJS.

#include <stdio.h>
#include "js/js.h"
#include "console.h"

static JSValue print_args(JSContext *ctx, int argc, JSValueConst *argv, FILE *out) {
  for (int i = 0; i < argc; i++) {
    const char *s = JS_ToCString(ctx, argv[i]);
    if (!s) {
      // un objeto que no se deja convertir: como QuickJS, sin cortar el log
      JS_FreeValue(ctx, JS_GetException(ctx));
      s = NULL;
    }
    fprintf(out, "%s%s", i ? " " : "", s ? s : "[object]");
    JS_FreeCString(ctx, s);
  }
  fputc('\n', out);
  fflush(out);
  return JS_UNDEFINED;
}

static JSValue js_console_log(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  return print_args(ctx, argc, argv, stdout);
}

static JSValue js_console_error(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  return print_args(ctx, argc, argv, stderr);
}

static JSValue js_console_nothing(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)ctx; (void)this_val; (void)argc; (void)argv;
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_console_funcs[] = {
  JS_CFUNC_DEF("log", 1, js_console_log),
  JS_CFUNC_DEF("info", 1, js_console_log),
  JS_CFUNC_DEF("debug", 1, js_console_log),
  JS_CFUNC_DEF("trace", 1, js_console_log),
  JS_CFUNC_DEF("warn", 1, js_console_error),
  JS_CFUNC_DEF("error", 1, js_console_error),
  JS_CFUNC_DEF("assert", 2, js_console_nothing),
  JS_CFUNC_DEF("time", 1, js_console_nothing),
  JS_CFUNC_DEF("timeEnd", 1, js_console_nothing),
  JS_CFUNC_DEF("group", 1, js_console_nothing),
  JS_CFUNC_DEF("groupEnd", 0, js_console_nothing),
  JS_CFUNC_DEF("table", 1, js_console_log),
};

void js_api_add_console(JSContext *ctx) {
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue console = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, console, js_console_funcs, sizeof(js_console_funcs) / sizeof(js_console_funcs[0]));
  JS_SetPropertyStr(ctx, global, "console", console);
  JS_SetPropertyStr(ctx, global, "print", JS_NewCFunction(ctx, js_console_log, "print", 1));
  JS_FreeValue(ctx, global);
}
