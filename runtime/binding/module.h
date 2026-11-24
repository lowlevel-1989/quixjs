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

// <script type="module">, lo que usa Construct 3: sus import "x" de efecto
// (sin nombres) se ejecutan antes, una sola vez cada uno y relativos al
// modulo; el resto, en su propia funcion (sus nombres no salen, como en un
// modulo). "export {};" (vacio) se ignora; import con nombres
// (import {a} from) y los demas export no estan.

#include "core/config/loader.h"
#include "js/js.h"

// rel: la ruta dentro de game_dir ("scripts/c3main.js"). Como
// scripts_run_file: 0 bien, 1 error de sintaxis (ya escrito), -1 no existe,
// -2 excepcion al ejecutarlo (queda pendiente en ctx)
int module_run_file(JSContext *ctx, const Config *cfg, const char *rel);
