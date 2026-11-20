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

// quix.lan: la red local para juegos (runtime/lan) en JS. Solo entre
// consolas con el mismo juego ([saves] id o el titulo):
//   quix.lan.open()             la abre; true si va
//   quix.lan.close()
//   quix.lan.id, quix.lan.name  esta consola (su firma y su nombre; null cerrada)
//   quix.lan.peers()            [{id, name}]: las consolas que estan ahora
//   quix.lan.send(text[, id])   a una consola, o a todas sin id; true si salio
//   quix.lan.receive()          [{from, data}]: lo que llego desde la ultima vez
// peers() y receive() leen la red: se llaman en cada imagen.

#include "js/js.h"

void js_api_add_lan(JSContext *ctx);
