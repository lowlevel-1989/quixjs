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

// Tabla de teclas: KeyboardEvent.code -> key y keyCode, como los da el
// navegador con un teclado US. La usan el backend sdl y quix.pressKey().

#include "backend/backend.h"

typedef struct {
  const char *code;
  const char *key;
  const char *shift_key;   // key con shift (NULL = igual que key)
  int         key_code;
} KeyInfo;

const KeyInfo *keymap_find(const char *code);

// Rellena ev a partir del code. Devuelve -1 si el code no existe.
int keymap_make_event(InputEvent *ev, InputType type, const char *code, int shift, int repeat);
