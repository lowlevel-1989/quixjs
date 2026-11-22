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

// Un .asar de Electron leido como un zip sin comprimir (ver asar.c)

#include <stddef.h>
#include <stdint.h>
#include "zip.h"

// 1 si los primeros 16 bytes de un archivo son la cabecera de un asar
int asar_is(const uint8_t head[16]);
// Los archivos del JSON de la cabecera (sin los de app.asar.unpacked), con
// su posicion en el archivo. NULL si el JSON no se entiende
ZipEntry *asar_entries(const uint8_t head[16], const char *json, size_t len, int *count);
