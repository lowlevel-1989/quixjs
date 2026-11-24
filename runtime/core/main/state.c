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

// Los savestates desde el mando: Select + X guarda, Select + Y carga y Select
// + izquierda o derecha cambia de ranura. El backend lo pide (want_state) y
// el loop lo hace aqui, entre dos frames (sin JS a medias), con un aviso. Si
// el juego no deja ahora, la peticion espera y se hace cuando sea seguro
// (quix_state_tick, en cada vuelta del loop).
// Guardar y cargar lo hace quix.state (runtime/binding/quix/state.c).
// Parte de core/main.c, que une las partes en orden y se compila como un
// solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "run.h"

static void do_state(Loop *l) {
  int want = l->backend->want_state;
  l->backend->want_state = 0;
  int slot = quix_state_slot();
  char title[64], text[96];
  if (want == WANT_STATE_PREV || want == WANT_STATE_NEXT) {
    slot += want == WANT_STATE_NEXT ? 1 : -1;
    if (slot < 1) slot = QUIX_STATE_SLOTS;
    if (slot > QUIX_STATE_SLOTS) slot = 1;
    quix_state_set_slot(slot);
    long long ms = quix_state_time(l->ctx, slot);
    int wload = 0;
    snprintf(title, sizeof title, "Slot %d of %d", slot, QUIX_STATE_SLOTS);
    if (quix_state_waiting(&wload)) {
      // una peticion que espera: pasa a esta ranura
      snprintf(text, sizeof text, wload ? "Waiting to load from here" : "Waiting to save here");
    } else if (ms) {
      time_t when = (time_t)(ms / 1000), now = time(NULL);
      struct tm w = *localtime(&when), n = *localtime(&now);
      char stamp[32];
      // de hoy, la hora; si no, el dia y la hora
      strftime(stamp, sizeof stamp, w.tm_yday == n.tm_yday && w.tm_year == n.tm_year ? "%H:%M" : "%Y-%m-%d %H:%M", &w);
      snprintf(text, sizeof text, "Saved %s", stamp);
    } else {
      snprintf(text, sizeof text, "Empty");
    }
    quix_state_toast(QUIX_STATE_SLOT, slot, title, text);
    return;
  }
  // pedir: ya, o cuando sea seguro (quix_state_tick); los avisos los da
  // binding/quix/state.c
  if (want == WANT_STATE_SAVE) quix_state_request(l->ctx, 0, slot);
  if (want == WANT_STATE_LOAD) quix_state_request(l->ctx, 1, slot);
}
