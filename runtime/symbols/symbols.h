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

// [symbols]: nombres para el estado del juego, que leen los logros y escriben
// los trucos.
//
//   [symbols]
//   boss1_dead = "path:Xq.a7.$f"    una propiedad desde window (a.b.c)
//   ending     = "var:88"           variable 88 de RPG Maker ($gameVariables)
//   lever      = "switch:12"        interruptor 12 de RPG Maker ($gameSwitches)
//
// Cada imagen (symbols_tick) se leen todos: el valor de ahora y el de la
// imagen anterior. Un simbolo que no existe (el juego aun no lo ha creado)
// queda como SV_MISSING. Escribir uno (symbols_set) hace lo mismo que el juego:
// var y switch con setValue de RPG Maker (avisan como un evento), path
// asignando la propiedad.

#include <stddef.h>
#include "js/js.h"

typedef enum { SV_MISSING, SV_NUM, SV_STR } SymValueType;

typedef struct {
  SymValueType type;
  double       num;       // SV_NUM (los booleanos son 0 o 1)
  char         str[64];   // SV_STR
} SymValue;

// Lee [symbols] del toml del juego. Cuantos hay, o -1 si alguno esta mal escrito.
int  symbols_load(const char *config_path);
void symbols_free(void);
int  symbols_count(void);
int  symbols_find(const char *name);   // -1 si no esta
const char *symbols_name(int i);

// Una vez por imagen: el de ahora pasa a ser el anterior y se lee de nuevo
void symbols_tick(JSContext *ctx);
const SymValue *symbols_now(int i);
const SymValue *symbols_prev(int i);

// El valor de ahora como valor de JS (hay que liberarlo); undefined si no existe
JSValue symbols_get(JSContext *ctx, int i);
// Escribe el simbolo; 0 si se pudo (el juego ya lo tiene creado)
int  symbols_set(JSContext *ctx, int i, JSValueConst value);

// Los simbolos y sus valores en texto (la consola: ach); devuelve lo escrito
size_t symbols_describe(char *out, size_t room);
