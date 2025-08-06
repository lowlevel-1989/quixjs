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

#pragma once

#include <stdint.h>

typedef void* Texture;

typedef struct Backend Backend;

typedef struct {
    void (*init)(Backend* backend, int width, int height);
    void (*process_events)(Backend* backend);
    void (*swap_buffers)(Backend* backend);
    void (*clear)(Backend* backend);
    void (*cleanup)(Backend* backend);
    int (*should_close)(Backend* backend);
    void (*fill_rect_color)(struct Backend* backend, int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
    void (*clear_rect)(Backend* backend, int x, int y, int w, int h);
} BackendInterface;

struct Backend {
    const BackendInterface* iface;
    void* platform_data;
};

Backend* backend_create(int type);
void backend_init(Backend* backend, int width, int height);
void backend_process_events(Backend* backend);
void backend_swap_buffers(Backend* backend);
void backend_cleanup(Backend* backend);
void backend_clear(Backend* backend);
int backend_should_close(Backend* backend);
