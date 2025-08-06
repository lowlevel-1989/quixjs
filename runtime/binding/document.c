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
#include <stdlib.h>
#include "canvas.h"
#include "document.h"
#include "core/app_context.h"

typedef struct {
  char     *id;
  JSValue  obj;
} ElementEntry;

static ElementEntry *element_registry       = NULL;
static int           element_registry_count = 0;

void register_element_with_id(JSContext *ctx, JSValue obj, const char *id) {
  element_registry = realloc(element_registry, sizeof(ElementEntry) * (element_registry_count + 1));
  element_registry[element_registry_count].id = strdup(id);
  element_registry[element_registry_count].obj = JS_DupValue(ctx, obj);
  element_registry_count++;
}

void free_element_registry(JSContext *ctx) {
  for (int i = 0; i < element_registry_count; ++i) {
    free(element_registry[i].id);  // Liberar el string
    JS_FreeValue(ctx, element_registry[i].obj);  // Liberar el JSValue
  }
  free(element_registry);
  element_registry = NULL;
  element_registry_count = 0;
}

// Dummy function para addEventListener
JSValue js_dummy_addEventListener(JSContext *ctx, JSValueConst this_val,
    int argc, JSValueConst *argv) {
  printf("[addEventListener] called\n");

  // No hace nada
  return JS_UNDEFINED;
}

JSValue js_dummy_appendChild(JSContext *ctx, JSValueConst this_val,
    int argc, JSValueConst *argv) {
  printf("[appendChild] called\n");

  // Simplemente devuelve el elemento que se intentó añadir
  if (argc > 0) {
    return JS_DupValue(ctx, argv[0]);
  }
  return JS_UNDEFINED;
}

JSValue js_create_element(JSContext *ctx, const char *tag) {
  printf("[createElement] called with tag: <%s>\n", tag);

  AppContext *app = JS_GetContextOpaque(ctx);

  JSValue elem;
  if (tag && strcmp(tag, "canvas") == 0) {
    elem = js_create_canvas(ctx);
  } else {
    elem = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, elem, "tagName", JS_NewString(ctx, tag));
  }

  JS_SetPropertyStr(ctx, elem, "id", JS_NewString(ctx, ""));

  JSValue style = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, style, "width",    JS_NewInt32(ctx, app->cfg->game.width));
  JS_SetPropertyStr(ctx, style, "height",   JS_NewInt32(ctx, app->cfg->game.height));
  JS_SetPropertyStr(ctx, style, "position", JS_NewInt32(ctx,  0));
  JS_SetPropertyStr(ctx, style, "top",      JS_NewInt32(ctx,  0));
  JS_SetPropertyStr(ctx, style, "left",     JS_NewInt32(ctx,  0));
  JS_SetPropertyStr(ctx, style, "zIndex",   JS_NewInt32(ctx,  0));

  JS_SetPropertyStr(ctx, elem, "style", style);

  // Añadir addEventListener dummy
  JS_SetPropertyStr(ctx, elem, "addEventListener",
      JS_NewCFunction(ctx, js_dummy_addEventListener, "addEventListener", 2));

  // Añadir appendChild dummy
  JS_SetPropertyStr(ctx, elem, "appendChild",
      JS_NewCFunction(ctx, js_dummy_appendChild, "appendChild", 1));

  return elem;
}

// Dummy createElement
static JSValue js_createElement(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  const char *tag = JS_ToCString(ctx, argv[0]);
  JSValue elem = js_create_element(ctx, tag);
  JS_FreeCString(ctx, tag);
  return elem;
}

JSValue js_get_element_by_id(JSContext *ctx, JSValueConst this_val,
    int argc, JSValueConst *argv) {

  const char *id = JS_ToCString(ctx, argv[0]);
  for (int i = 0; i < element_registry_count; i++) {
    if (strcmp(element_registry[i].id, id) == 0) {
      printf("[getElementById] found element with id: %s\n", id);
      JS_FreeCString(ctx, id);
      return JS_DupValue(ctx, element_registry[i].obj);
    }
  }

  JS_FreeCString(ctx, id);
  return JS_NULL;
}

static JSValue js_getElementById(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

  if (argc < 1) {
    return JS_UNDEFINED;
  }
  const char *id = JS_ToCString(ctx, argv[0]);
  if (!id) {
    return JS_UNDEFINED;
  }

  AppContext *app = JS_GetContextOpaque(ctx);

  printf("[getElementById] called with id: %s\n", id);

  JSValue elem = js_get_element_by_id(ctx, this_val, argc, argv);

  if (JS_IsNull(elem) && strcmp(id, app->cfg->engine.canvas_id) == 0) {
    printf("[getElementById] create element with id: %s\n", id);
    elem = js_create_element(ctx, "canvas");
    JS_SetPropertyStr(ctx, elem, "id", JS_NewString(ctx, id));
    register_element_with_id(ctx, elem, id);
  }

  return elem;
}

JSValue js_create_document(JSContext *ctx) {
  JSValue document = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, document, "createElement",
      JS_NewCFunction(ctx, js_createElement, "createElement", 1));

  // Crear body
  JSValue body = js_create_element(ctx, "body");
  JS_SetPropertyStr(ctx, document, "body", body);

  // Añadir addEventListener al documento mismo
  JS_SetPropertyStr(ctx, document, "addEventListener",
      JS_NewCFunction(ctx, js_dummy_addEventListener, "addEventListener", 2));

  JS_SetPropertyStr(ctx, document, "getElementById",
      JS_NewCFunction(ctx, js_getElementById, "getElementById", 1));

  return document;
}
