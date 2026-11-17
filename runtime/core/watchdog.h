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

// Salida de emergencia: un hilo lee los mandos directamente del kernel
// (/dev/input/event*). Si Select + Start estan pulsados y el bucle principal
// lleva mas de un segundo sin pasar (el juego congelado en una tarea larga),
// quixjs sale. Con el juego respondiendo, sale por el camino normal (el
// backend sdl). Sin permiso para leer /dev/input (un PC sin root) no hace nada.

void watchdog_start(void);
// El bucle principal sigue vivo (en cada vuelta)
void watchdog_alive(void);
