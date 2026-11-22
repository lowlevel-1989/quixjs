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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "backend.h"

// Backend sin ventana: guarda el ultimo frame para poder hacer capturas
// (--screenshot) y para los tests.

typedef struct {
  int      should_close;
  uint8_t *frame;
  int      frame_w;
  int      frame_h;
} DummyPlatformData;

static void dummy_init(Backend* backend, int width, int height) {
  printf("Dummy backend initialized (headless mode)\n");
  DummyPlatformData* data = (DummyPlatformData*)backend->platform_data;
  data->should_close = 0;
}

static void dummy_process_events(Backend* backend) {
  // No hay eventos que procesar (los tests usan quix.input.pressKey)
}

static void dummy_present(Backend* backend, const uint8_t *rgba, int w, int h) {
  DummyPlatformData* data = (DummyPlatformData*)backend->platform_data;
  if (!rgba || w <= 0 || h <= 0) return;
  if (data->frame_w != w || data->frame_h != h) {
    uint8_t *frame = realloc(data->frame, (size_t)w * h * 4);
    if (!frame) return;
    data->frame = frame;
    data->frame_w = w;
    data->frame_h = h;
  }
  memcpy(data->frame, rgba, (size_t)w * h * 4);
}

static void dummy_cleanup(Backend* backend) {
  printf("Dummy backend cleaned up\n");
  DummyPlatformData* data = (DummyPlatformData*)backend->platform_data;
  if (data) {
    free(data->frame);
    free(data);
    backend->platform_data = NULL;
  }
}

static int dummy_should_close(Backend* backend) {
  DummyPlatformData* data = (DummyPlatformData*)backend->platform_data;
  return data->should_close;
}

static const BackendInterface dummy_backend_interface = {
  .init           = dummy_init,
  .process_events = dummy_process_events,
  .present        = dummy_present,
  .cleanup        = dummy_cleanup,
  .should_close   = dummy_should_close,
  .get_gamepads   = NULL,
  .name           = "dummy",
};

Backend* dummy_backend_create() {
  Backend* backend = (Backend*)calloc(1, sizeof(Backend));
  if (!backend) {
    return NULL;
  }

  DummyPlatformData* data = (DummyPlatformData*)calloc(1, sizeof(DummyPlatformData));
  if (!data) {
    free(backend);
    return NULL;
  }

  backend->iface = &dummy_backend_interface;
  backend->platform_data = data;

  return backend;
}
