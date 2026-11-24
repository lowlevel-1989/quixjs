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

// runtime/binding/quix/fonts.c: quix.fonts: registrar una fuente (un archivo
// del juego o sus bytes, lo que usan FontFace y @font-face) y saber si una
// familia ya esta.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/app_context.h"
#include "core/url/url.h"
#include "gfx/soft/font.h"
#include "common.h"

// quix.fonts.register(family, url): la usan FontFace y @font-face
static JSValue js_quix_registerFont(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  AppContext *app = JS_GetContextOpaque(ctx);
  if (argc < 2) return JS_ThrowTypeError(ctx, "registerFont: family and url required");
  const char *family = JS_ToCString(ctx, argv[0]);
  const char *url = JS_ToCString(ctx, argv[1]);
  int ok = 0;
  if (family && url) {
    char path[1024];
    if (url_resolve(app->cfg->game.game_dir, url, path, sizeof(path)) == URL_FILE) {
      font_init();
      ok = font_register(family, path) == 0;
    }
  }
  if (family) JS_FreeCString(ctx, family);
  if (url) JS_FreeCString(ctx, url);
  return JS_NewBool(ctx, ok);
}

// quix.fonts.register(family, buffer): FontFace con un ArrayBuffer (o un
// typed array)
static JSValue js_quix_registerFontData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 2) return JS_ThrowTypeError(ctx, "registerFontData: family and buffer required");
  size_t len = 0;
  uint8_t *data = JS_GetBufferBytes(ctx, argv[1], &len);
  if (!data || len == 0) return JS_FALSE;
  const char *family = JS_ToCString(ctx, argv[0]);
  if (!family) return JS_EXCEPTION;
  font_init();
  int ok = font_register_data(family, data, len) == 0;
  JS_FreeCString(ctx, family);
  return JS_NewBool(ctx, ok);
}

static JSValue js_quix_fonts_has(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1) return JS_FALSE;
  const char *family = JS_ToCString(ctx, argv[0]);
  if (!family) return JS_EXCEPTION;
  int r = font_is_registered(family);
  JS_FreeCString(ctx, family);
  return JS_NewBool(ctx, r);
}


// quix.fonts.register(familia, url | bytes): un archivo de fuente o sus bytes
// (FontFace con un ArrayBuffer)
static JSValue js_quix_fonts_register(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc >= 2 && !JS_IsString(argv[1])) return js_quix_registerFontData(ctx, this_val, argc, argv);
  return js_quix_registerFont(ctx, this_val, argc, argv);
}

static const JSCFunctionListEntry js_quix_fonts[] = {
  JS_CFUNC_DEF("register", 2, js_quix_fonts_register),
  JS_CFUNC_DEF("has", 1, js_quix_fonts_has),
};

void quix_add_fonts(JSContext *ctx, JSValueConst quix) {
  quix_add_group(ctx, quix, "fonts", js_quix_fonts, countof(js_quix_fonts));
}
