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

// <script> anadidos con appendChild: los que tienen src se cargan en el
// siguiente ciclo del loop y despues reciben load
// o error, en el orden en que se anadieron; los que no tienen src ejecutan
// su texto al momento. Asi cargan sus plugins RPG Maker y muchas librerias.
//
// script_document_loaded() lanza DOMContentLoaded y load cuando terminan de
// cargar los scripts del arranque.

#include "js/js.h"

void script_maybe_run(JSContext *ctx, JSValueConst element);
void script_document_loaded(JSContext *ctx);

// Los scripts de [scripts] del toml del juego tambien estan en el documento (como en su
// index.html): document.getElementsByTagName('script') los encuentra y
// document.currentScript apunta al que se esta ejecutando.
void script_add_static(JSContext *ctx, char **srcs, int count);
void script_before_static(JSContext *ctx, int index);
void script_free(JSContext *ctx);
