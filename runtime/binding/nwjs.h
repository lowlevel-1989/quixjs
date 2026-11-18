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

// Modo nw.js ([engine] nwjs = true en el toml del juego): require('fs'),
// require('path'), require('events'), require('os'), require('nw.gui'),
// process y global, lo minimo que usan los juegos hechos para nw.js (RPG Maker
// MV guarda las partidas con fs en www/save/).
//
// - Las rutas relativas son relativas a game_dir (donde esta el index.html).
// - require('./x') carga x.js como modulo CommonJS.
// - Los modulos nativos (.node) y los que no existen devuelven un sustituto
//   cuyas funciones devuelven false (como un juego sin Steam abierto).

#include "js/js.h"

int js_api_add_nwjs(JSContext *ctx);
