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

// save: los archivos que escribe el juego (partidas, localStorage), por el
// canal save del dispatch (core/dispatch.h).

#ifndef QUIX_SAVE_H
#define QUIX_SAVE_H

#include <stddef.h>

// Escribe data en path: en un temporal (path.quixtmp) que despues se
// renombra, asi una partida no queda a medias si se apaga; con append, al
// final del archivo. 1 si se escribio
int save_write(const char *path, const void *data, size_t len, int append);

#endif
