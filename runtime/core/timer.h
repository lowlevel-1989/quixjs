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
#include "quickjs-libc.h"
#include "backend/backend.h"

int timer_process(JSContext *ctx, Backend *backend);
int timer_set(JSContext *ctx, JSValueConst func, int64_t delay);
void timer_clear(JSContext *ctx, int id);
int timer_count();
void timer_free(JSContext *ctx);
int timer_request_animation_frame(JSContext *ctx, JSValueConst func);
