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

// Los trucos de un port ([[cheat]] en el toml del juego). Se activan en el
// lanzador (Cheats) y un truco activo fija simbolos de [symbols] mientras se
// juega:
//
//   [symbols]
//   encounters = "path:$gameSystem._encounterEnabled"
//
//   [[cheat]]
//   id          = "no_encounters"
//   name        = "No random encounters"
//   description = "..."                     (opcional)
//   set         = { encounters = false }
//
// La partida en la que se juega con un truco activo queda marcada para
// siempre: la marca se guarda con la partida (el motor dice donde:
// runtime/engines/<motor>/playthrough.js) y esa partida no consigue logros,
// aunque luego se apague el truco. Al empezar o cargar una partida marcada
// sale un aviso. Las demas partidas (nuevas o limpias, sin trucos activos)
// siguen consiguiendo logros. Si un truco ya no esta activo, al cargar
// una partida sus simbolos vuelven al valor que tenian antes del truco.
//
// Sin un motor conocido (sin playthrough.js que lo reconozca) los trucos no
// se aplican: no habria forma de proteger los logros.

#include <stddef.h>
#include "js/js.h"

// Lee [[cheat]] (despues de symbols_load). Cuantos hay, o -1 si alguno esta mal.
int  cheats_load(const char *config_path);
void cheats_free(void);
int  cheats_count(void);
const char *cheats_id(int i);
const char *cheats_name(int i);
const char *cheats_description(int i);
int  cheats_active(int i);
// Los activos, por id (el resto, no)
void cheats_set_active(const char *const *ids, int n);

// Una vez por imagen, despues de symbols_tick y antes de ach_tick
void cheats_tick(JSContext *ctx);
// La partida en curso uso trucos (sin logros para ella)
int  cheats_playthrough_cheated(void);

// Los trucos en texto (la consola: ach); devuelve lo escrito
size_t cheats_describe(char *out, size_t room);
