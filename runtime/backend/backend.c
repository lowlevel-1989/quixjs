/*
 * QuixJS - A lightweight JS runtime using SDL2 and JavaScriptCore
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

// Lo comun a los backends: crear el elegido (sdl o dummy), pasar cada
// llamada a sus funciones y la cola de entrada. Ver backend.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "backend/dummy/backend.h"
#include "backend/sdl/backend.h"
#include "core/args.h"
#include "backend.h"

Backend* backend_create(int type) {
  Backend *backend = NULL;
  if (type == BACKEND_SDL) {
    backend = sdl_backend_create();
  } else if (type == BACKEND_DUMMY) {
    backend = dummy_backend_create();
  }
  if (backend) {
    backend->input_head = 0;
    backend->input_len = 0;
  }
  return backend;
}

void backend_init(Backend* backend, int width, int height) {
  if (backend && backend->iface && backend->iface->init) {
    backend->iface->init(backend, width, height);
  }
}

void backend_process_events(Backend* backend) {
  if (backend && backend->iface && backend->iface->process_events) {
    backend->iface->process_events(backend);
  }
}

void backend_present(Backend* backend, const uint8_t *rgba, int w, int h) {
  if (backend && backend->iface && backend->iface->present) {
    backend->iface->present(backend, rgba, w, h);
  }
}

int backend_can_present_gl(Backend* backend) {
  return backend && backend->iface && backend->iface->present_gl != NULL;
}

void backend_present_gl(Backend* backend, unsigned tex, int w, int h, int premultiplied) {
  if (backend_can_present_gl(backend)) backend->iface->present_gl(backend, tex, w, h, premultiplied);
}

int backend_can_present_layers(Backend* backend) {
  return backend && backend->iface && backend->iface->present_layers != NULL;
}

void backend_present_layers(Backend* backend, const PresentLayer *layers, int n) {
  if (backend_can_present_layers(backend)) backend->iface->present_layers(backend, layers, n);
}

void backend_cleanup(Backend* backend) {
  if (backend && backend->iface && backend->iface->cleanup) {
    backend->iface->cleanup(backend);
  }
}

int backend_should_close(Backend* backend) {
  if (backend && backend->iface && backend->iface->should_close) {
    return backend->iface->should_close(backend);
  }
  return 1; // Si no hay backend, asumir que debe cerrarse
}

int backend_get_gamepads(Backend* backend, GamepadState *pads, int max) {
  if (backend && backend->iface && backend->iface->get_gamepads) {
    return backend->iface->get_gamepads(backend, pads, max);
  }
  return 0;
}

int backend_capture_input(Backend* backend, int step, char *out, size_t room) {
  if (backend && backend->iface && backend->iface->capture_input) {
    return backend->iface->capture_input(backend, step, out, room);
  }
  return -1;
}

int backend_save_controller(Backend* backend, const char *pairs) {
  if (backend && backend->iface && backend->iface->save_controller) {
    return backend->iface->save_controller(backend, pairs);
  }
  return -1;
}

void backend_set_fullscreen(Backend* backend, int on) {
  if (backend && backend->iface && backend->iface->set_fullscreen) backend->iface->set_fullscreen(backend, on);
}

double backend_refresh_rate(Backend* backend) {
  if (backend && backend->iface && backend->iface->refresh_rate) return backend->iface->refresh_rate(backend);
  return 0;
}

const char *backend_name(Backend* backend) {
  if (backend && backend->iface && backend->iface->name) return backend->iface->name;
  return "none";
}

void backend_push_input(Backend* backend, const InputEvent *ev) {
  if (!backend || backend->input_len >= INPUT_QUEUE_SIZE) return;  // cola llena: se pierde
  int idx = (backend->input_head + backend->input_len) % INPUT_QUEUE_SIZE;
  backend->input[idx] = *ev;
  backend->input_len++;
}

int backend_pop_input(Backend* backend, InputEvent *ev) {
  if (!backend || backend->input_len == 0) return 0;
  *ev = backend->input[backend->input_head];
  backend->input_head = (backend->input_head + 1) % INPUT_QUEUE_SIZE;
  backend->input_len--;
  return 1;
}

void backend_present_wait(Backend* backend) {
  if (backend && backend->iface && backend->iface->present_wait) backend->iface->present_wait(backend);
}

int backend_presented(Backend* backend, double *ms, int max) {
  if (backend && backend->iface && backend->iface->presented) return backend->iface->presented(backend, ms, max);
  return -1;
}
