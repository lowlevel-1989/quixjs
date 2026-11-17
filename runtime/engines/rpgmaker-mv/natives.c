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

// La lista de las funciones nativas de RPG Maker MV. Cada una va en su
// archivo (adjust_tone.c...) y la usa el hook de esta carpeta con su mismo
// nombre (adjust-tone.js), que cae a la original de JS si la nativa no puede
// (devuelve false). Una nativa nueva: su .c, su declaracion en natives.h y
// una linea aqui.

#include "natives.h"

static const JSCFunctionListEntry funcs[] = {
  JS_CFUNC_DEF("adjustTone", 4, rpgmaker_mv_adjust_tone),
  JS_CFUNC_DEF("lzDecompressBase64", 1, rpgmaker_mv_lz_decompress_base64),
  JS_CFUNC_DEF("setDecryptKey", 1, rpgmaker_mv_set_decrypt_key),
  JS_CFUNC_DEF("splitJsonArray", 2, rpgmaker_mv_split_json_array),
  JS_CFUNC_DEF("tintSprite", 17, rpgmaker_mv_tint_sprite),
};

JSValue rpgmaker_mv_natives(JSContext *ctx) {
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, o, funcs, sizeof(funcs) / sizeof(funcs[0]));
  return o;
}
