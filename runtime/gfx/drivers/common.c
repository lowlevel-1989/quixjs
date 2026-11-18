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

// common.c: las piezas que comparten los drivers (ver common.h)

#include <stddef.h>
#include "common.h"
#include "gfx/gl.h"

// Espera a que la GPU termine la imagen y la muestra. La CPU se para hasta
// que la GPU acaba: es lo mas simple, y en un PC sobra.
void drv_present_now(const Presenter *p, GpuFrame *f) {
  p->wait(f);
  p->show(f);
}

// Se muestra la imagen anterior, que la GPU termino mientras el juego hacia
// esta; esta, en la siguiente. La CPU no espera a la GPU antes del refresco;
// cuesta una imagen mas de retraso (33 ms a 30 fps), como present_ahead.
// Necesita tres texturas por canvas (webgl_canvas_handoff con 3: ver el
// README de esta carpeta).
static GpuFrame *prev;

void drv_present_previous(const Presenter *p, GpuFrame *f) {
  if (!f->later) {
    // hay que mostrarla ahora: la guardada ya no se vera
    if (prev) p->drop(prev);
    prev = NULL;
    drv_present_now(p, f);
    return;
  }
  if (prev) drv_present_now(p, prev);
  prev = f;
}

// La GPU del contexto que presenta espera la marca (eglWaitSyncKHR) y el
// thread sigue: entrega el cambio de imagen sin pararse.
void drv_wait_gpu(void *fence) {
  gl_fence_wait_gpu(fence);
}

// La CPU del thread espera la marca: el cambio de imagen sale cuando la GPU
// ya termino. Mas seguro con drivers que no ordenan bien dos contextos.
void drv_wait_cpu(void *fence) {
  gl_fence_wait_cpu(fence);
}
