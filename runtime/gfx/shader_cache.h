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

#pragma once

// Los programas de WebGL ya enlazados, guardados en disco
// (GL_OES_get_program_binary). Compilar y enlazar los shaders de PixiJS la
// primera vez que se dibuja el mapa costaba ~1 s con libmali en la RX6S
// (RK3326): la siguiente vez que se abre el juego, el programa sale de la
// cache en unos ms. La clave es la GPU, su driver, los sources de los
// shaders y los bindAttribLocation; si el driver ya no acepta un binario
// guardado (otra version), se enlaza como siempre y se guarda de nuevo.

#include "gl.h"
#include <stdint.h>

// La carpeta de la cache (se crea al guardar el primero). Sin la extension
// (se mira al primer programa) o sin carpeta, no hace nada.
void shader_cache_init(const char *dir);
// Un bindAttribLocation(program, index, name): entra en la clave
void shader_cache_bind(GLuint program, GLuint index, const char *name);
// En lugar de glLinkProgram: el programa de la cache o enlazado (y guardado)
void shader_cache_link(GLuint program);
// El programa se borro: su nombre puede volver con otros bindings
void shader_cache_forget(GLuint program);
