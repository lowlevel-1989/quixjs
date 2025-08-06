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

#include <stdlib.h>
#include <stdio.h>
#include "backend.h"

typedef struct {
    int should_close;
} DummyPlatformData;

static void dummy_init(Backend* backend, int width, int height) {
    printf("Dummy backend initialized (headless mode)\n");
    DummyPlatformData* data = (DummyPlatformData*)backend->platform_data;
    data->should_close = 0;
}

static void dummy_process_events(Backend* backend) {
    // No hay eventos que procesar
}

static void dummy_swap_buffers(Backend* backend) {
    // No hay buffers que intercambiar
}

static void dummy_cleanup(Backend* backend) {
    printf("Dummy backend cleaned up\n");
}

static int dummy_should_close(Backend* backend) {
    DummyPlatformData* data = (DummyPlatformData*)backend->platform_data;
    return data->should_close;
}

static const BackendInterface dummy_backend_interface = {
    .init = dummy_init,
    .process_events = dummy_process_events,
    .swap_buffers = dummy_swap_buffers,
    .cleanup = dummy_cleanup,
    .should_close = dummy_should_close
};

Backend* dummy_backend_create() {
    Backend* backend = (Backend*)malloc(sizeof(Backend));
    if (!backend) {
        return NULL;
    }
    
    DummyPlatformData* data = (DummyPlatformData*)malloc(sizeof(DummyPlatformData));
    if (!data) {
        free(backend);
        return NULL;
    }
    
    data->should_close = 0;
    
    backend->iface = &dummy_backend_interface;
    backend->platform_data = data;
    
    return backend;
}
