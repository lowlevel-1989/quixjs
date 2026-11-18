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

// Un thread de trabajo para cargar en segundo plano (decodificar imagenes, leer
// y descifrar sonidos): lo que tarda decenas de ms y en el thread principal era
// una caida. work corre en el thread (sin JS ni nada compartido); done, en el
// thread principal (worker_poll, una vez por vuelta del loop), con el JS.

#include "js/js.h"

// 0 si se pidio; -1 si no hay thread (el que llama lo hace al momento)
int  worker_submit(void (*work)(void *arg), void (*done)(JSContext *ctx, void *arg), void *arg);
void worker_poll(JSContext *ctx);
int  worker_pending(void);   // pedidos y aun no entregados
