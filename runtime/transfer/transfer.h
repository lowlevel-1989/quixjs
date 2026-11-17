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

// Transfer Save: pasar las partidas a otro dispositivo de la misma red (un
// PC, una portatil), sin teclado y sin abrir puertos. Heredado de tamatama
// (src/transfer). Lo abre el lanzador (--launcher).

// La pantalla de Transfer Save, con las pantallas ya abiertas (ui_open).
// game: el id del juego ([game] id, o su titulo si no tiene); solo se
// pasan partidas del mismo juego.
// Devuelve 0 al volver con Back, o SCREEN_QUIT si se cerro la ventana.
int transfer_screen(const char *game);
