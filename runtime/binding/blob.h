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

#pragma once

// Blob y URL.createObjectURL / revokeObjectURL.
//
// Los bytes de un Blob viven en C (con contador de referencias): la URL
// "blob:quix/N" que da createObjectURL la pueden leer Image y XMLHttpRequest
// sin copiar los datos a JS. RPG Maker lo usa para las imagenes cifradas: el
// juego las descifra con su codigo y se las pasa a Image con una URL blob.

#include <stddef.h>
#include <stdint.h>
#include "js/js.h"

void js_api_add_blob(JSContext *ctx);

// Datos de una URL blob (NULL si no existe o ya se revoco). No copia.
const uint8_t *blob_lookup(const char *url, size_t *len);
void blob_free_all(void);
