/*
 * QuixJS - A lightweight JS runtime using SDL2 and QuickJS
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

#include <stdio.h>
#include <string.h>
#include "backend/dummy/backend.h"
#include "backend/sdl/backend.h"
#include "core/args.h"
#include "backend.h"

Backend* backend_create(int type) {
  if (type == BACKEND_SDL) {
    return sdl_backend_create();
  } else if (type == BACKEND_DUMMY) {
    return dummy_backend_create();
  }
  return NULL;
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

void backend_swap_buffers(Backend* backend) {
  if (backend && backend->iface && backend->iface->swap_buffers) {
    backend->iface->swap_buffers(backend);
  }
}

void backend_clear(Backend* backend) {
  if (backend && backend->iface && backend->iface->clear) {
    backend->iface->clear(backend);
  }
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
