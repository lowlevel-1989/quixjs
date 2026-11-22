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

// dispatch: ver dispatch.h.

#include <string.h>
#include <time.h>
#include "dispatch.h"

#define CHANNELS_MAX 8
static DispatchChannel *channels[CHANNELS_MAX];
static int channels_n;

static double now_ms(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static void add(DispatchChannel *c) {
  for (int i = 0; i < channels_n; i++)
    if (channels[i] == c) return;
  if (channels_n < CHANNELS_MAX) channels[channels_n++] = c;
}

static void send(DispatchChannel *c, const void *item, double now) {
  int busy = c->send(c->user, item);
  c->ready_at = now + (busy > 0 ? busy : 0) + c->gap_ms;
}

void dispatch_push(DispatchChannel *c, const void *item, int size) {
  if (!c || size > DISPATCH_ITEM_MAX) return;
  add(c);
  double now = now_ms();
  // libre y sin nada esperando: sale ya, sin retraso
  if (!c->has_pending && now >= c->ready_at) {
    send(c, item, now);
    return;
  }
  if (c->has_pending && c->merge && !c->merge(c->pending, item)) return;
  memcpy(c->pending, item, (size_t)size);
  c->has_pending = 1;
}

void dispatch_run(void) {
  double now = now_ms();
  for (int i = 0; i < channels_n; i++) {
    DispatchChannel *c = channels[i];
    if (!c->has_pending || now < c->ready_at) continue;
    c->has_pending = 0;
    send(c, c->pending, now);
  }
}
