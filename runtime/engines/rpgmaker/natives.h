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
#pragma once

// Las funciones nativas comunes a RPG Maker MV y MZ (quix.engine["rpgmaker"]).
// Una nativa nueva: su .c, su declaracion aqui y una linea en natives.c.

#include "js/js.h"

JSValue rpgmaker_natives(JSContext *ctx);

// decrypt.c (hooks rpgmaker-mv/decrypt-image-native.js,
// rpgmaker-mv/load-audio-native.js y rpgmaker-mz/decrypt-image-native.js)
JSValue rpgmaker_set_decrypt_key(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
