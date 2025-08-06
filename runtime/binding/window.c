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

#include <string.h>
#include "quickjs-libc.h"
#include "window.h"
#include "document.h"
#include "navigator.h"
#include "core/app_context.h"

void js_api_add_window(JSContext *ctx) {
  JSValue global = JS_GetGlobalObject(ctx);

  // Hacer que `window` sea igual a `globalThis`
  JS_SetPropertyStr(ctx, global, "window", JS_DupValue(ctx, global));

  // Dummy addEventListener global
  JSValue addEventListenerFunc = JS_NewCFunction(ctx, js_dummy_addEventListener, "addEventListener", 2);
  JS_SetPropertyStr(ctx, global, "addEventListener", JS_DupValue(ctx, addEventListenerFunc));

  // document
  JSValue document = js_create_document(ctx);
  JS_SetPropertyStr(ctx, global, "document", document);
                                                                //
  AppContext *app = JS_GetContextOpaque(ctx);

  JS_SetPropertyStr(ctx, global, "innerWidth",  JS_NewInt32(ctx, app->cfg->game.width));
  JS_SetPropertyStr(ctx, global, "innerHeight", JS_NewInt32(ctx, app->cfg->game.height));
  JS_SetPropertyStr(ctx, global, "width",       JS_NewInt32(ctx, app->cfg->game.width));
  JS_SetPropertyStr(ctx, global, "height",      JS_NewInt32(ctx, app->cfg->game.height));

  // navigator
  JSValue navigator = js_create_navigator(ctx);
  JS_SetPropertyStr(ctx, global, "navigator", navigator);

  // location
  JSValue location = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, location, "href", JS_NewString(ctx, "http://localhost/"));
  JS_SetPropertyStr(ctx, global, "location", JS_DupValue(ctx, location));
  JS_SetPropertyStr(ctx, global, "window.location", location);  // opcional

  JS_FreeValue(ctx, addEventListenerFunc);
  JS_FreeValue(ctx, global);
}

