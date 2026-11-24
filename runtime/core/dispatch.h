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

// dispatch: el JS no habla con el hardware; deja una peticion en un canal y
// quixjs la manda al aparato: enseguida si esta libre (sin retraso), y si
// esta ocupado, cuando quede libre (dispatch_run, en el bucle principal, una
// vez por imagen). De una en una y a un ritmo que el aparato aguanta. Para
// el juego no cambia nada.
//
// Cada canal guarda como mucho una peticion pendiente (la que gana con
// merge, si llegan varias) y la manda cuando el aparato esta libre: send
// devuelve cuanto lo deja ocupado (ms) y despues se espera gap_ms mas.
//
// Los canales:
//   vibrate  el mando (backend/backend.c): una cada vez y con pausa; en la RG
//            CubeXX muchas seguidas colgaban la consola
//   audio    el mezclador (binding/audio/audio.c): play, set, stop
//   lan      la red local (binding/quix/lan.c): los envios
//   save     los archivos que escribe el juego (core/save.c): partidas,
//            localStorage
//   (la imagen ya la presenta un solo thread: backend/sdl/thread.c)
// audio, lan y save devuelven una respuesta al juego en el momento (el id de
// la voz, si se escribio...): no esperan nunca (busy 0, gap 0), salen al
// instante y la respuesta va en la propia peticion (un puntero).

#ifndef QUIX_DISPATCH_H
#define QUIX_DISPATCH_H

#define DISPATCH_ITEM_MAX 96   // bytes de una peticion

typedef struct DispatchChannel {
  const char *name;
  int gap_ms;   // la espera entre una y la siguiente, despues de su duracion
  // manda item al aparato; devuelve cuanto tarda en quedar libre (ms)
  int (*send)(void *user, const void *item);
  // 1 si la nueva (b) sustituye a la pendiente (a)
  int (*merge)(const void *a, const void *b);
  void *user;
  // el estado (lo lleva dispatch.c)
  unsigned char pending[DISPATCH_ITEM_MAX];
  int has_pending;
  double ready_at;   // desde cuando se puede mandar la siguiente (ms)
} DispatchChannel;

// deja la peticion en el canal (size <= DISPATCH_ITEM_MAX)
void dispatch_push(DispatchChannel *c, const void *item, int size);
// manda lo pendiente de los canales que ya estan libres (bucle principal)
void dispatch_run(void);

#endif
