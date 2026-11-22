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

// quix.engine: un objeto por motor con sus funciones nativas. Ver
// engine_api.h.

#include <string.h>
#include "engines/engine_api.h"
#include "engines/rpgmaker/natives.h"
#include "engines/rpgmaker-mv/natives.h"
#include "hooks/hooks.h"
// runtime/engines/same-code.js: tools/embed_js.py lo mete en el binario al
// compilar; js_same_code.h (en obj/gen/) define SAME_CODE_JS, su texto. Ver
// docs/embedded-js.md.
#include "js_same_code.h"

// Al arrancar (main), antes de cargar el juego:
//   1. crea quix.engine, con las funciones nativas de cada motor (las
//      comunes a MV y MZ en "rpgmaker") y lib, los modulos de engines/lib
//   2. ejecuta runtime/engines/same-code.js, que agrega quix.engine.sameCode
// Despues, al cargar el juego, runtime/hooks/hooks.c instala los hooks, que los
// usan.
void engines_add_api(JSContext *ctx) {
  // 1. quix.engine = { "rpgmaker": {...}, "rpgmaker-mv": { ...nativas }, lib }
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue quix = JS_GetPropertyStr(ctx, global, "quix");
  JSValue engines = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, engines, "rpgmaker", rpgmaker_natives(ctx));
  JS_SetPropertyStr(ctx, engines, "rpgmaker-mv", rpgmaker_mv_natives(ctx));
  // quix.engine.lib("<nombre>"): los modulos de runtime/engines/lib (hooks.c)
  JS_SetPropertyStr(ctx, engines, "lib", hooks_lib_function(ctx));
  JS_SetPropertyStr(ctx, quix, "engine", engines);
  JS_FreeValue(ctx, quix);
  JS_FreeValue(ctx, global);

  // 2. runtime/engines/same-code.js: agrega quix.engine.sameCode (el archivo
  // se llama a si
  //    mismo con quix.engine al final)
  JSValue r = JS_Eval(ctx, SAME_CODE_JS, strlen(SAME_CODE_JS), "<quix-same-code>", JS_EVAL_TYPE_GLOBAL);
  if (JS_IsException(r)) js_std_dump_error(ctx);
  JS_FreeValue(ctx, r);
}
