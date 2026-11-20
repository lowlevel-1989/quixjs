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

// La lista de las funciones nativas comunes a RPG Maker MV y MZ. Cada una va
// en su archivo (decrypt.c) y la usan los hooks de los dos motores, que caen
// a la original de JS si la nativa no puede (devuelve false).

#include "natives.h"

static const JSCFunctionListEntry funcs[] = {
  JS_CFUNC_DEF("setDecryptKey", 1, rpgmaker_set_decrypt_key),
};

JSValue rpgmaker_natives(JSContext *ctx) {
  JSValue o = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, o, funcs, sizeof(funcs) / sizeof(funcs[0]));
  return o;
}
