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

// Los trucos de un port ([[cheat]] en el toml del juego). Se activan en el
// launcher (Cheats) y un truco activo fija simbolos de [symbols] mientras se
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
// siempre y no consigue logros, aunque luego se apague el truco. Al empezar
// o cargar una partida marcada sale un aviso. Las demas partidas (nuevas o
// limpias, sin trucos activos) siguen consiguiendo logros. Si un truco ya no
// esta activo, al cargar una partida sus simbolos vuelven al valor que
// tenian antes del truco.
//
// La partida la da el juego con quix.playthrough, desde hooks de su motor o
// de su port (los de RPG Maker: runtime/engines/rpgmaker/hooks/playthrough-*):
//   quix.playthrough.start()      al empezar una partida nueva
//   quix.playthrough.save()       al guardar: lo que se guarda con ella
//   quix.playthrough.load(datos)  al cargarla: lo que dio save
// Esa es la regla para tener trucos: sin start ni load no hay partida y los
// trucos no se aplican (no habria donde marcarla). Un juego sin logros no la
// necesita: sus trucos van siempre.

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
// El modo de los logros (Options > Achievements), antes de empezar el juego:
// en hardcore no hay trucos, y una partida empezada asi lo apunta en sus
// datos (quix.playthrough, firmados con la partida)
void cheats_set_hardcore(int on);
int  cheats_hardcore_mode(void);
// Los logros de ahora cuentan como hardcore: el modo es hardcore, la partida
// empezo en hardcore (o el juego no da la partida a quixjs) y sin trucos
int  cheats_playthrough_hardcore(void);
// se acaba de cargar un savestate (binding/quix/state.c): la partida ya no
// cuenta como hardcore, y lo lleva en sus datos firmados
void cheats_state_loaded(JSContext *ctx);
// quix.playthrough (start, save, load) en el objeto quix
void cheats_add_api(JSContext *ctx, JSValueConst quix);

// Los trucos en texto (la consola: ach); devuelve lo escrito
size_t cheats_describe(char *out, size_t room);
