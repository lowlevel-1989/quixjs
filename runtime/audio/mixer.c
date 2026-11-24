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

// El mezclador: los clips (ogg, mp3, wav: decoder.h), las voces que suenan y la mezcla en el thread
// de audio de SDL. Ver mixer.h.
//
// El codigo esta en mixer/, partido por temas para que se pueda leer; este
// archivo los une y se compila como uno solo. Asi el compilador ve todo junto
// y mete dentro (inline) las funciones pequenas de una parte en otra (aqui
// esta la mezcla de cada bloque de audio, en el thread de SDL).
// Una parte nueva se agrega aqui, en su sitio (despues de lo que usa). Ver
// mixer/README.md.

#include "mixer/common.h"
#include "mixer/clip.c"
#include "mixer/decode.c"
#include "mixer/voice.c"
#include "mixer/device.c"
#include "mixer/stream.c"
