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

// El estado del runtime que comparten el loop principal y los bindings: el
// backend, la configuracion y el frame actual. Los bindings lo leen con
// JS_GetContextOpaque(ctx).

#include "backend/backend.h"
#include "core/config/loader.h"

typedef struct {
    Backend* backend;
    Config*  cfg;

    long     frame;           // frames mostrados
    double   frame_time;      // el momento programado de la imagen actual (ms, como performance.now)
    int      exit_requested;  // quix.exit()
    int      exit_code;
} AppContext;
