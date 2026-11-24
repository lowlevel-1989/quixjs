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

// runtime/binding/quix/common.h: lo que comparten las partes del objeto quix,
// y nadie mas (el resto de quixjs usa quix.h):
//   quix.c          el objeto quix y lo basico (exit, frame, version...)
//   input.c         quix.input
//   gpu.c           quix.gpu
//   achievements.c  quix.achievements y quix.steam
//   storage.c       quix.storage: el localStorage del juego en disco
//   fonts.c         quix.fonts: registrar fuentes
//   memory.c        quix.memory: el GC y la memoria del JS
//   lan.c           quix.lan: la red local para juegos
//   trace_saves.c   --trace-saves (con trace-saves.js)
#pragma once

#include "js/js.h"

// un grupo del objeto quix (quix.storage, quix.fonts...): un objeto con la lista
void quix_add_group(JSContext *ctx, JSValueConst quix, const char *name, const JSCFunctionListEntry *list, int n);

// cada parte agrega su grupo
void quix_add_input(JSContext *ctx, JSValueConst quix);
void quix_add_gpu(JSContext *ctx, JSValueConst quix);
void quix_add_achievements(JSContext *ctx, JSValueConst quix);   // y quix.steam
void quix_add_storage(JSContext *ctx, JSValueConst quix);
void quix_add_state(JSContext *ctx, JSValueConst quix);
void quix_add_fonts(JSContext *ctx, JSValueConst quix);
void quix_add_memory(JSContext *ctx, JSValueConst quix);
// quix.lan: la red local para juegos (runtime/lan) en JS. Solo entre
// consolas con el mismo juego ([saves] id o el titulo):
//   quix.lan.open()             la abre; true si va
//   quix.lan.close()
//   quix.lan.id, quix.lan.name  esta consola (su firma y su nombre; null cerrada)
//   quix.lan.peers()            [{id, name}]: las consolas que estan ahora
//   quix.lan.send(text[, id])   a una consola, o a todas sin id; true si salio
//   quix.lan.receive()          [{from, data}]: lo que llego desde la ultima vez
// peers() y receive() leen la red: se llaman en cada imagen.
void quix_add_lan(JSContext *ctx, JSValueConst quix);
// quix.traceSave, quix.tracingSaves y quix.saves.isSave
void quix_add_trace_saves(JSContext *ctx, JSValueConst quix);
