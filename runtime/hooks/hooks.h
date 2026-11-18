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

// Hooks: cambian como corre una funcion del juego sin tocar sus archivos.
// Se definen en el toml del juego (config.toml, o port.toml en un port):
//
//   [hooks]
//   "Spriteset_Map.prototype.createLightmask" = "gpu-canvas"
//
// La clave es la ruta de la funcion desde window; el valor, el hook:
//   gpu-canvas      los canvas que se crean dentro de esa funcion dibujan en
//                   la GPU (gfx/gpu2d.c): para las mascaras de luces
//   skip            la funcion no hace nada (desactivar un efecto)
//   once-per-frame  solo la primera llamada de cada imagen; las demas no
//                   hacen nada (lo que se repite en cada actualizacion de
//                   RPG Maker cuando va con retraso)
//   <motor>/<hook>  un hook de motor: runtime/engines/<motor>/<hook>.js,
//                   por ejemplo "rpgmaker-mv/skip-branch-table"
//   off             no poner el hook de motor de esa ruta
//
// engine = "rpgmaker-mv" en [hooks] pone todos los hooks de motor de
// runtime/engines/rpgmaker-mv/hooks.toml; los del juego mandan sobre ellos.
//
// Se ponen en cuanto la funcion existe (los plugins se cargan despues del
// arranque): hooks_install se llama en cada imagen hasta que estan todos.

#include "js/js.h"

// Lee [hooks]. Devuelve cuantos hay, o -1 si alguno esta mal escrito.
int  hooks_load(const char *config_path);
// Las funciones nativas que usan los hooks (__quixGpuScope)
void hooks_add_api(JSContext *ctx);
// Pone los que aun no estan; 1 cuando estan todos
int  hooks_install(JSContext *ctx);
int  hooks_pending(void);
// Vuelve a poner los hooks que un plugin ha pisado despues (cada medio segundo)
void hooks_check(JSContext *ctx);
// El codigo de un archivo de motor metido en el binario
// (runtime/engines/<motor>/<nombre>.js) por su nombre ("rpgmaker-mv/playthrough"),
// o NULL. i-esimo: hooks_engine_file(i, &name) da el codigo y el nombre (NULL al acabar).
const char *hooks_engine_source(const char *name);
const char *hooks_engine_file(int i, const char **name);
