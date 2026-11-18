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

// OpenGL ES 2.0 para WebGL: un contexto propio (ventana oculta de SDL; sin
// ventana con el driver offscreen) y las funciones cargadas con
// SDL_GL_GetProcAddress. Las cabeceras son las que trae SDL, asi que no hace
// falta GLES en el sistema para compilar (tampoco para PortMaster).

#define SDL_USE_BUILTIN_OPENGL_DEFINITIONS 1
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

#ifndef GL_DEPTH24_STENCIL8_OES
#define GL_DEPTH24_STENCIL8_OES 0x88F0
#endif

typedef struct {
#define QX_GL(type, name) type name;
#include "gl_funcs.h"
#undef QX_GL
} QxGL;

extern QxGL qgl;

// Carga las funciones con el contexto GL actual (lo usa el backend SDL para
// presentar). Devuelve 0 si estan todas.
int  gl_load(void);
// El backend SDL presenta con su contexto: el de WebGL se crea en esa ventana
// y comparte texturas con el (el canvas WebGL se muestra sin leerlo).
void gl_set_present_window(SDL_Window *window, SDL_GLContext context);
// headless: sin ventana (backend dummy, tests). Devuelve 0 si hay GL.
int  gl_init(int headless);
int  gl_available(void);
void gl_make_current(void);
int  gl_has_extension(const char *name);
const char *gl_renderer_name(void);
void gl_shutdown(void);

// Un segundo contexto que comparte texturas con el de WebGL: lo usan los
// canvas 2d en la GPU (gfx/gpu2d.c) para no tocar el estado GL del juego.
// gl_aux_begin lo hace actual (creandolo la primera vez) y gl_aux_end
// devuelve el que estaba. 0 si va.
int  gl_aux_begin(void);
void gl_aux_end(void);

// Presentar desde otro thread (backend sdl): gl_offthread_begin suelta la
// ventana en este thread y los contextos de WebGL siguen sin ella (EGL con
// EGL_KHR_surfaceless_context). 0 si se puede; si no, todo sigue como estaba.
// gl_offthread_end lo deshace.
int  gl_offthread_begin(void);
void gl_offthread_end(void);
// Lo dibujado hasta ahora en este thread: lo manda a la GPU y devuelve una
// marca (EGL_KHR_fence_sync; NULL si no hay) que quien presenta espera con
// gl_fence_wait_cpu antes de usar las texturas.
void *gl_fence(void);
// Esperar lo marcado y soltar la marca. _gpu: la GPU de este contexto espera
// (eglWaitSyncKHR; la CPU sigue). _cpu: la CPU de este thread espera; con
// libmali la espera en la GPU no basta (el menu del titulo y el de la batalla
// de RPG Maker pestaneaban en la RGB30). Cual usa el thread que presenta lo
// dice el driver (gfx/drivers/, thread_wait)
void gl_fence_wait_gpu(void *fence);
void gl_fence_wait_cpu(void *fence);
