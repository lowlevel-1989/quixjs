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

// runtime/backend/star.h: la estrella dorada de un logro conseguido en modo
// hardcore, en pixeles como la letra de las pantallas de quixjs. La dibujan
// el aviso (backend/toast.c) y el launcher (launcher/ui/draw.c), cada pixel
// un cuadrado de la escala de su texto.
#pragma once

#define STAR_SIZE 9
#define STAR_RGB  0xf2c14e
#define STAR_GAP  5          // pixeles (de la escala del texto) entre el nombre y la estrella

static const char STAR_ART[STAR_SIZE][STAR_SIZE + 1] = {
  "....X....",
  "....X....",
  "...XXX...",
  "XXXXXXXXX",
  ".XXXXXXX.",
  "..XXXXX..",
  "..XXXXX..",
  ".XX...XX.",
  ".X.....X.",
};
