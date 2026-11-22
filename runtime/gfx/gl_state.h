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

// El estado de OpenGL del contexto de WebGL, guardado y devuelto: quixjs
// dibuja en ese contexto (el del juego) sin que el juego lo note. Lo usan
// runtime/gfx/gpu2d.c (el canvas 2d en la GPU) y runtime/backend/sdl/present.c
// (la imagen en pantalla con libmali, sin el thread que presenta).

#include "gl.h"

typedef struct {
  GLint fb, vp[4], scissor[4], abuf, active, tex[2], prog, unpack;
  GLint beq[2], bf[4];
  GLboolean en[5], cmask[4];
  GLfloat clear[4];
  struct { GLint en, size, type, norm, stride, buf; void *ptr; } at[3];
  // ES 3 (WebGL2): el vertex array y el buffer de subida de pixeles
  GLint vao, unpack_buf, unpack_row, unpack_skip_rows, unpack_skip_pixels;
} GlState;

// Guarda el estado actual
void gl_state_save(GlState *s);

// Deja un estado conocido para dibujar en 2d (sin depth, stencil, cull ni
// scissor; sin vertex array ni buffer de subida)
void gl_state_plain(void);

// Devuelve lo guardado
void gl_state_restore(const GlState *s);
