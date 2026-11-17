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

// EventTarget (addEventListener / removeEventListener / dispatchEvent) y los
// constructores Event, KeyboardEvent, MouseEvent y CustomEvent.

#include "js/js.h"

void js_api_add_events(JSContext *ctx);
void events_free(JSContext *ctx);

// Pone addEventListener, removeEventListener y dispatchEvent en obj
// (normalmente un prototipo).
void js_event_target_setup(JSContext *ctx, JSValueConst obj);

// new Event(type) / new KeyboardEvent(type, init)
JSValue events_new(JSContext *ctx, const char *type);
JSValue events_new_keyboard(JSContext *ctx, const char *type, JSValueConst init);

// Llama a target.on<type> y a los listeners. Devuelve -1 si alguno lanza
// una excepcion (ya se ha mostrado por stderr).
int events_dispatch(JSContext *ctx, JSValueConst target, JSValueConst event);

// Despacha un Event(type) en el siguiente ciclo del bucle (setTimeout 0),
// como los eventos load/error de Image y XMLHttpRequest.
void events_dispatch_later(JSContext *ctx, JSValueConst target, const char *type);
