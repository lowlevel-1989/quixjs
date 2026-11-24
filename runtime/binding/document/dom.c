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

// runtime/binding/document/dom.c: el document: title, los constructores de HTMLElement,
// crearlo y liberarlo.
// Parte de runtime/binding/document.c (archivo indice).

#include "common.h"

void document_free(JSContext *ctx) {
  for (int i = 0; i < element_registry_count; ++i) {
    free(element_registry[i].id);  // Liberar el string
    JS_FreeValue(ctx, element_registry[i].obj);  // Liberar el JSValue
  }
  free(element_registry);
  element_registry = NULL;
  element_registry_count = 0;

  if (document_ready) {
    JS_FreeValue(ctx, element_proto);
    JS_FreeValue(ctx, screen_list);
    document_ready = 0;
  }
}

static JSValue js_document_true(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return JS_TRUE;
}

// document.title: el del juego ([game] title) hasta que el juego pone otro;
// ponerlo cambia el titulo de la ventana ("quixjs | <titulo>")
static char doc_title[256];
static int doc_title_set = 0;

static JSValue js_document_get_title(JSContext *ctx, JSValueConst this_val) {
  AppContext *app = JS_GetContextOpaque(ctx);
  if (!doc_title_set && app && app->cfg) return JS_NewString(ctx, app->cfg->launcher.title);
  return JS_NewString(ctx, doc_title);
}

static JSValue js_document_set_title(JSContext *ctx, JSValueConst this_val, JSValueConst val) {
  const char *s = JS_ToCString(ctx, val);
  if (!s) return JS_EXCEPTION;
  snprintf(doc_title, sizeof doc_title, "%s", s);
  doc_title_set = 1;
  JS_FreeCString(ctx, s);
  AppContext *app = JS_GetContextOpaque(ctx);
  if (app && app->backend) backend_set_title(app->backend, doc_title);
  return JS_UNDEFINED;
}

static const JSCFunctionListEntry js_document_funcs[] = {
  JS_CGETSET_DEF("title", js_document_get_title, js_document_set_title),
  JS_CFUNC_DEF("createElement",        1, js_document_createElement),
  JS_CFUNC_DEF("createTextNode",       1, js_document_createTextNode),
  JS_CFUNC_DEF("getElementById",       1, js_document_getElementById),
  JS_CFUNC_DEF("getElementsByTagName", 1, js_document_getElementsByTagName),
  JS_CFUNC_DEF("getElementsByClassName", 1, js_element_empty_list),
  JS_CFUNC_DEF("querySelector",        1, js_document_querySelector),
  JS_CFUNC_DEF("querySelectorAll",     1, js_element_empty_list),
  JS_CFUNC_DEF("hasFocus",             0, js_document_true),
  JS_CFUNC_DEF("exitFullscreen",       0, js_element_noop),
  JS_CFUNC_DEF("exitPointerLock",      0, js_element_noop),
};

static JSValue js_html_element_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
  return JS_ThrowTypeError(ctx, "Illegal constructor");
}

static void define_element_ctor(JSContext *ctx, JSValueConst global, const char *name, JSValueConst proto) {
  JSValue ctor = JS_NewCFunction2(ctx, js_html_element_ctor, name, 0, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, ctor, proto);
  JS_SetPropertyStr(ctx, global, name, ctor);
}

static void dom_init(JSContext *ctx) {
  if (document_ready) return;
  element_proto = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, element_proto, js_element_proto_funcs, countof(js_element_proto_funcs));
  js_event_target_setup(ctx, element_proto);
  screen_list = JS_NewArray(ctx);
  document_ready = 1;

  JSValue global = JS_GetGlobalObject(ctx);
  define_element_ctor(ctx, global, "HTMLElement", element_proto);
  define_element_ctor(ctx, global, "Element", element_proto);
  define_element_ctor(ctx, global, "Node", element_proto);
  // video y audio no estan soportados: prototipos propios para que
  // "x instanceof HTMLVideoElement" sea false con canvas e imagenes
  JSValue video_proto = JS_NewObjectProto(ctx, element_proto);
  define_element_ctor(ctx, global, "HTMLVideoElement", video_proto);
  JS_FreeValue(ctx, video_proto);
  JS_FreeValue(ctx, global);
}

JSValue js_create_document(JSContext *ctx) {
  dom_init(ctx);

  JSValue document = JS_NewObject(ctx);
  JS_SetPropertyFunctionList(ctx, document, js_document_funcs, countof(js_document_funcs));
  js_event_target_setup(ctx, document);

  JSValue html = js_create_element(ctx, "html");
  JSValue head = js_create_element(ctx, "head");
  JSValue body = js_create_element(ctx, "body");
  JSValue r;
  r = js_element_appendChild(ctx, html, 1, (JSValueConst *)&head); JS_FreeValue(ctx, r);
  r = js_element_appendChild(ctx, html, 1, (JSValueConst *)&body); JS_FreeValue(ctx, r);

  JS_SetPropertyStr(ctx, document, "documentElement", html);
  JS_SetPropertyStr(ctx, document, "head", head);
  JS_SetPropertyStr(ctx, document, "body", body);
  JS_SetPropertyStr(ctx, document, "nodeType", JS_NewInt32(ctx, 9));
  JS_SetPropertyStr(ctx, document, "readyState", JS_NewString(ctx, "loading"));
  JS_SetPropertyStr(ctx, document, "visibilityState", JS_NewString(ctx, "visible"));
  JS_SetPropertyStr(ctx, document, "hidden", JS_FALSE);
  JS_SetPropertyStr(ctx, document, "cookie", JS_NewString(ctx, ""));
  JS_SetPropertyStr(ctx, document, "currentScript", JS_NULL);

  return document;
}
