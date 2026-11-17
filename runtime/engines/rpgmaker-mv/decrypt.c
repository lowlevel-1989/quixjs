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

// Las imagenes y los sonidos cifrados de RPG Maker MV (.rpgmvp, .rpgmvo,
// .rpgmvm), descifrados en C al leerlos (core/url.h, url_add_transform): asi
// el JS no crea un buffer de varios MB por archivo (la respuesta del XHR, la
// copia de slice y el Blob), que disparaban recolecciones a mitad de imagen.
// Lo mismo que el Decrypter de RPG Maker: quitar la cabecera de 16 bytes y
// hacer XOR de los 16 primeros con la clave ($dataSystem.encryptionKey).

#include <stdlib.h>
#include <string.h>
#include "core/url.h"
#include "natives.h"

#define HEADER 16

static uint8_t key[16];

static int decrypt(uint8_t *data, size_t *len) {
  if (*len < HEADER) return -1;
  memmove(data, data + HEADER, *len - HEADER);
  *len -= HEADER;
  data[*len] = 0;
  for (size_t i = 0; i < HEADER && i < *len; i++) data[i] ^= key[i];
  return 0;
}

static int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

// setDecryptKey("d41d8cd98f00b204e9800998ecf8427e"): true si la clave vale
JSValue rpgmaker_mv_set_decrypt_key(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  if (argc < 1) return JS_FALSE;
  const char *hex = JS_ToCString(ctx, argv[0]);
  if (!hex) return JS_FALSE;
  int ok = strlen(hex) >= 32;
  for (int i = 0; ok && i < 16; i++) {
    int hi = hex_digit(hex[i * 2]), lo = hex_digit(hex[i * 2 + 1]);
    if (hi < 0 || lo < 0) ok = 0;
    else key[i] = (uint8_t)(hi << 4 | lo);
  }
  JS_FreeCString(ctx, hex);
  if (!ok) return JS_FALSE;
  url_add_transform(".rpgmvp", decrypt);
  url_add_transform(".rpgmvo", decrypt);
  url_add_transform(".rpgmvm", decrypt);
  return JS_TRUE;
}
