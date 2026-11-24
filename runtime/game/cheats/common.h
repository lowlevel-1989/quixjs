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

// runtime/game/cheats/common.h: lo que comparten las partes de los trucos, y
// nadie mas (el resto de quixjs usa cheats.h):
//   cheats.c       la lista, lo que se consulta de ella, encenderlos y lo que
//                  dice el comando ach de la consola
//   define.c       leer [[cheat]] del toml
//   playthrough.c  quix.playthrough: la partida en curso, firmada
//   apply.c        cada frame: aplicar los trucos activos a la partida
#pragma once

#include "js/js.h"
#include "cheats.h"

#define MAX_CHEATS     32
#define MAX_CHEAT_SETS 8

typedef struct {
  int    symbol;
  int    kind;           // 0 booleano, 1 numero, 2 texto
  double num;            // el booleano (0/1) o el numero
  char   str[64];
} CheatSet;

typedef struct {
  char     id[64];
  char     name[128];
  char     description[256];
  CheatSet sets[MAX_CHEAT_SETS];
  int      set_count;
  char    *script;       // JS que cambia el juego mientras el truco esta activo (o NULL)
  int      active;
} Cheat;

// La lista de los trucos del juego (define.c la llena al cargar)
extern Cheat cheats_list[MAX_CHEATS];
extern int   cheats_total;

// El estado de la partida en curso. playthrough.c la cambia (quix.playthrough
// start y load); apply.c la mira en cada frame
typedef struct {
  JSValue  pt_data;       // sus datos: cheats (marcada), orig (los valores de antes)
  int      pt_has_data;   // hay partida: los trucos se aplican
  unsigned pt_gen;        // cada start o load: otra partida
  unsigned pt_seen;       // la ultima partida que vio cheats_tick
  int      pt_state;      // 0 sin mirar, 2 juego sin logros (no hace falta la partida)
  int      pt_warned;
  int      cheated;       // la partida en curso uso trucos: sin logros
  int      scripts_run;   // los script de los trucos activos ya se ejecutaron
  unsigned frame;
  int      noticed;       // el aviso de esta partida ya salio
  int      hardcore_mode; // Options > Achievements: Hardcore (sin trucos)
  int      pt_hardcore;   // la partida en curso empezo en hardcore
} CheatsState;
extern CheatsState cheats_st;

// cheats.c: si hay algun truco encendido
int cheats_any_active(void);
// playthrough.c: un dato de la partida en curso (hay que liberarlo; undefined
// si no hay partida) y cambiarlo
JSValue cheats_pt_get(JSContext *ctx, const char *key);
void    cheats_pt_set(JSContext *ctx, const char *key, JSValueConst value);
