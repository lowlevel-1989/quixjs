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

// Logros para los ports, definidos en el toml del juego (port.toml en un
// port):
//
//   [symbols]
//   boss1_dead = "path:Xq.a7.$f"    una propiedad desde window (a.b.c)
//   ending     = "var:88"           variable 88 de RPG Maker ($gameVariables)
//   lever      = "switch:12"        interruptor 12 de RPG Maker ($gameSwitches)
//
//   [[achievements]]
//   id          = "boss_1"
//   title       = "First Blood"
//   description = "Defeat the first boss"     (opcional)
//   conditions  = [
//     "boss1_dead == 1",
//     "prev(boss1_dead) == 0",
//   ]
//
// En cada imagen (frame) del juego se leen los simbolos y se mira cada logro aun no
// conseguido: se consigue cuando se cumplen todas sus condiciones. Una
// condicion es "a OP b" (OP: == != < <= > >=) o solo "a" (distinto de 0);
// a y b son un simbolo, prev(simbolo) (su valor en la imagen anterior), un
// numero, "texto", true o false. Un simbolo que no existe (el juego aun no lo
// ha creado) no cumple ninguna condicion, ni prev() en la primera imagen.
// Una condicion puede juntar alternativas con || (basta con que se cumpla
// una): "a == 1 || b == 2".
//
// Los conseguidos se guardan en [saves] achievements (por defecto
// quixjs-achievements.txt, en la carpeta de las partidas: [saves] dir, o
// game_dir sin el). Cada logro es una linea "id fecha_unix firma", y el
// archivo va cifrado para que no se pueda editar a mano (ver
// achievements.c).

#include <stddef.h>
#include "js/js.h"
#include "backend/backend.h"

// Lee [[achievements]] del toml del juego. game_dir: la carpeta donde va el
// fichero de los conseguidos (la de las partidas); file es relativo a ella
// ("" para no guardarlos).
// Devuelve cuantos logros hay, o -1 si alguno esta mal escrito.
// game: el juego ([game] id o el titulo): entra en la firma de cada linea
// del archivo (las que no la tienen bien se ignoran)
int  ach_load(const char *config_path, const char *game_dir, const char *file, const char *game);
void ach_free(void);
int  ach_count(void);
const char *ach_id(int i);
const char *ach_title(int i);
const char *ach_description(int i);
const char *ach_type(int i);   // [[achievements]] type, o ""
// El color del tipo (0xRRGGBB), el del aviso: dorado los finales, azul el
// progreso, plateado lo secundario
unsigned ach_color(int i);
// 0 si no, o la fecha unix en la que se consiguio
long long ach_unlocked(int i);
int  ach_unlocked_by_id(const char *id);
// [achievements_menu] icon = N: el icono (de IconSet) de la opcion de los
// logros en el menu del juego (hook rpgmaker-mv/achievements-menu); 0 sin icono
int  ach_menu_icon(void);

// Una vez por imagen, despues del juego: lee los simbolos y mira los logros.
// El aviso de un logro conseguido va en backend->toast unos segundos.
void ach_tick(JSContext *ctx, Backend *backend, double now_ms);
// Todos los logros y los valores de los simbolos, en texto (la consola: ach)
void ach_describe(char *out, size_t room);
// Olvida los conseguidos (la consola: ach reset)
void ach_reset(void);
// vuelve a leer los conseguidos del archivo: despues de Transfer Save (Receive
// Save o Restore Backup lo cambian por otro), si no, lo que hay en memoria lo
// pisaria con el siguiente logro
void ach_reload(void);
