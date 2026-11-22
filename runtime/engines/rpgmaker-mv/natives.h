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

// Las funciones nativas de RPG Maker MV (quix.engine["rpgmaker-mv"])

#include "js/js.h"

JSValue rpgmaker_mv_natives(JSContext *ctx);

// adjust_tone.c (hook adjust-tone.js)
JSValue rpgmaker_mv_adjust_tone(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

// blur.c (hook blur-native.js)
JSValue rpgmaker_mv_blur(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);


// lazy_data.c (hook lazy-data.js)
JSValue rpgmaker_mv_split_json_array(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

// tint.c (hook tint-native.js)
JSValue rpgmaker_mv_tint_sprite(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

// lz_string.c (hook lz-string-native.js)
JSValue rpgmaker_mv_lz_decompress_base64(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
