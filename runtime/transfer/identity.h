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

// Como se presenta cada dispositivo en Transfer Save, sin teclado: su nombre
// y una firma de 4 caracteres. Heredado de tamatama (src/transfer/identity.h).

#include <stddef.h>

// usuario@hostname (sistema), por ejemplo root@rg-cubexx (ROCKNIX)
void identity_name(char *out, size_t room);
// Una firma nueva al azar, sin los caracteres que se confunden
void identity_new_signature(char out[5]);
// 1 si text es una firma valida
int  identity_valid_signature(const char *text);
// Deja solo ASCII imprimible y lo corta a room
void identity_clean(char *text, size_t room);
