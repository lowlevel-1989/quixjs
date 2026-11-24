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

// runtime/game/achievements/common.h: lo que comparten las partes de los
// logros, y nadie mas (el resto de quixjs usa achievements.h):
//   achievements.c  la lista, lo que se consulta de ella y el comando ach
//   define.c        leer [[achievements]] del toml y sus condiciones
//   store.c         el archivo de los conseguidos, cifrado y firmado
//   check.c         cada frame: las condiciones, el progreso y el aviso
#pragma once

#include "game/symbols/symbols.h"
#include "achievements.h"

#define MAX_ACHIEVEMENTS 256
#define MAX_CONDITIONS   8

typedef enum { OP_TRUTHY, OP_EQ, OP_NE, OP_LT, OP_LE, OP_GT, OP_GE } CondOp;
typedef enum { OPD_SYMBOL, OPD_PREV, OPD_CONST } OperandKind;

typedef struct {
  OperandKind kind;
  int         symbol;
  SymValue    value;
} Operand;

typedef struct {
  Operand a, b;
  CondOp  op;
} Clause;

#define MAX_ALTERNATIVES 8

// Una condicion: una o varias alternativas separadas por || (basta una)
typedef struct {
  Clause alt[MAX_ALTERNATIVES];
  int    count;
} Condition;

typedef struct {
  char      id[64];
  char      title[128];
  char      description[256];
  char      type[32];        // progression, win_condition... (para agrupar)
  Condition conditions[MAX_CONDITIONS];
  int       condition_count;
  int       progress;        // progress: el simbolo que cuenta, o -1
  int       goal;            // goal: cuantos hacen falta
  long long unlocked;
  int       hardcore;        // conseguido en modo hardcore (sin trucos)
} Achievement;

// La lista de los logros del juego (define.c la llena al cargar)
extern Achievement ach_list[MAX_ACHIEVEMENTS];
extern int         ach_total;
extern int         ach_icon;            // [achievements_menu] icon
extern char        ach_store_path[1024];   // el archivo de los conseguidos, o ""
extern char        ach_game_id[128];    // el juego: una firma de otro juego no vale

// store.c: leer (sobre ach_list) y guardar los conseguidos
void achievements_store_load(void);
void achievements_store_save(void);
