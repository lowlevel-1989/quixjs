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

#pragma once

// __quixVideo: lo nativo del elemento <video> (el resto, en JS, esta en
// polyfill.c). Los videos que suenan se decodifican una vez por imagen
// (video_tick) y el visible se muestra encima de los canvas.

#include "js/js.h"
#include "gfx/surface.h"

void js_api_add_video(JSContext *ctx);
// Una vez por imagen: avanza los videos que suenan
void video_tick(void);
// 1 si hay alguno sonando (el bucle principal no se para)
int  video_active(void);
// La imagen del video visible, o NULL. *changed: si es nueva desde la ultima vez.
const Surface *video_screen(int *changed);
void video_free_all(void);
