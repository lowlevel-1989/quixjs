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

// runtime/binding/document/element.c: los elementos: sus metodos (appendChild,
// setAttribute...), su prototipo y crearlos (createElement, createTextNode).
// Parte de runtime/binding/document.c (archivo indice).

#include "common.h"

static JSValue js_element_appendChild(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1 || !JS_IsObject(argv[0])) return JS_UNDEFINED;
  JSValue child = argv[0];

  // si ya tenia padre, se mueve
  JSValue old_parent = JS_GetPropertyStr(ctx, child, "parentNode");
  if (JS_IsObject(old_parent)) {
    JSValue kids = JS_GetPropertyStr(ctx, old_parent, "childNodes");
    if (JS_IsObject(kids)) {
      int idx = array_index_of(ctx, kids, child);
      if (idx >= 0) array_remove_at(ctx, kids, idx);
    }
    JS_FreeValue(ctx, kids);
  }
  JS_FreeValue(ctx, old_parent);

  JSValue kids = JS_GetPropertyStr(ctx, this_val, "childNodes");
  if (JS_IsObject(kids)) {
    JS_SetPropertyUint32(ctx, kids, array_length(ctx, kids), JS_DupValue(ctx, child));
  }
  JS_FreeValue(ctx, kids);
  JS_SetPropertyStr(ctx, child, "parentNode", JS_DupValue(ctx, this_val));
  JS_SetPropertyStr(ctx, child, "parentElement", JS_DupValue(ctx, this_val));

  screen_attach(ctx, child);
  script_maybe_run(ctx, child);
  return JS_DupValue(ctx, child);
}

static JSValue js_element_removeChild(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1 || !JS_IsObject(argv[0])) return JS_UNDEFINED;
  JSValue kids = JS_GetPropertyStr(ctx, this_val, "childNodes");
  if (JS_IsObject(kids)) {
    int idx = array_index_of(ctx, kids, argv[0]);
    if (idx >= 0) array_remove_at(ctx, kids, idx);
  }
  JS_FreeValue(ctx, kids);
  JS_SetPropertyStr(ctx, argv[0], "parentNode", JS_NULL);
  JS_SetPropertyStr(ctx, argv[0], "parentElement", JS_NULL);
  screen_detach(ctx, argv[0]);
  return JS_DupValue(ctx, argv[0]);
}

static JSValue js_element_insertBefore(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  // el orden no importa para pintar: se agrega al final
  return js_element_appendChild(ctx, this_val, argc, argv);
}

static JSValue js_element_setAttribute(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 2) return JS_UNDEFINED;
  JSValue attrs = JS_GetPropertyStr(ctx, this_val, "__attributes__");
  if (!JS_IsObject(attrs)) {
    JS_FreeValue(ctx, attrs);
    attrs = JS_NewObject(ctx);
    JS_DefinePropertyValueStr(ctx, this_val, "__attributes__", JS_DupValue(ctx, attrs),
                              JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE);
  }
  JSAtom name = JS_ValueToAtom(ctx, argv[0]);
  JS_SetProperty(ctx, attrs, name, JS_ToString(ctx, argv[1]));

  // id, width y height tambien son propiedades
  const char *n = JS_AtomToCString(ctx, name);
  if (n && (strcmp(n, "id") == 0 || strcmp(n, "width") == 0 || strcmp(n, "height") == 0 ||
            strcmp(n, "class") == 0)) {
    const char *prop = strcmp(n, "class") == 0 ? "className" : n;
    JS_SetPropertyStr(ctx, this_val, prop, JS_DupValue(ctx, argv[1]));
    if (strcmp(n, "id") == 0) {
      const char *id = JS_ToCString(ctx, argv[1]);
      if (id) {
        document_register_id(ctx, (JSValue)this_val, id);
        JS_FreeCString(ctx, id);
      }
    }
  }
  if (n) JS_FreeCString(ctx, n);
  JS_FreeAtom(ctx, name);
  JS_FreeValue(ctx, attrs);
  return JS_UNDEFINED;
}

static JSValue js_element_getAttribute(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1) return JS_NULL;
  JSValue attrs = JS_GetPropertyStr(ctx, this_val, "__attributes__");
  JSValue r = JS_NULL;
  if (JS_IsObject(attrs)) {
    JSAtom name = JS_ValueToAtom(ctx, argv[0]);
    JSValue v = JS_GetProperty(ctx, attrs, name);
    JS_FreeAtom(ctx, name);
    if (!JS_IsUndefined(v)) r = v;
  }
  JS_FreeValue(ctx, attrs);
  return r;
}

static JSValue js_element_getBoundingClientRect(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  AppContext *app = JS_GetContextOpaque(ctx);
  double w = app->cfg->game.width, h = app->cfg->game.height;
  CanvasData *cd = js_canvas_get(ctx, this_val);
  if (cd) {
    w = cd->surface.w;
    h = cd->surface.h;
  }
  JSValue r = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, r, "x", JS_NewInt32(ctx, 0));
  JS_SetPropertyStr(ctx, r, "y", JS_NewInt32(ctx, 0));
  JS_SetPropertyStr(ctx, r, "left", JS_NewInt32(ctx, 0));
  JS_SetPropertyStr(ctx, r, "top", JS_NewInt32(ctx, 0));
  JS_SetPropertyStr(ctx, r, "width", JS_NewFloat64(ctx, w));
  JS_SetPropertyStr(ctx, r, "height", JS_NewFloat64(ctx, h));
  JS_SetPropertyStr(ctx, r, "right", JS_NewFloat64(ctx, w));
  JS_SetPropertyStr(ctx, r, "bottom", JS_NewFloat64(ctx, h));
  return r;
}

static JSValue js_element_getElementsByTagName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

static JSValue js_element_noop(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return JS_UNDEFINED;
}

static JSValue js_element_empty_list(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return JS_NewArray(ctx);
}

static JSValue js_element_null(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return JS_NULL;
}

static JSValue js_element_false(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  return JS_FALSE;
}

static const JSCFunctionListEntry js_element_proto_funcs[] = {
  JS_CFUNC_DEF("appendChild",           1, js_element_appendChild),
  JS_CFUNC_DEF("removeChild",           1, js_element_removeChild),
  JS_CFUNC_DEF("insertBefore",          2, js_element_insertBefore),
  JS_CFUNC_DEF("setAttribute",          2, js_element_setAttribute),
  JS_CFUNC_DEF("getAttribute",          1, js_element_getAttribute),
  JS_CFUNC_DEF("removeAttribute",       1, js_element_noop),
  JS_CFUNC_DEF("getBoundingClientRect", 0, js_element_getBoundingClientRect),
  JS_CFUNC_DEF("getElementsByTagName",  1, js_element_getElementsByTagName),
  JS_CFUNC_DEF("getElementsByClassName", 1, js_element_empty_list),
  JS_CFUNC_DEF("querySelectorAll",      1, js_element_empty_list),
  JS_CFUNC_DEF("querySelector",         1, js_element_null),
  JS_CFUNC_DEF("contains",              1, js_element_false),
  JS_CFUNC_DEF("focus",                 0, js_element_noop),
  JS_CFUNC_DEF("blur",                  0, js_element_noop),
  JS_CFUNC_DEF("click",                 0, js_element_noop),
  JS_CFUNC_DEF("requestFullscreen",     0, js_element_noop),
  JS_CFUNC_DEF("requestPointerLock",    0, js_element_noop),
};

static void dom_init(JSContext *ctx);

JSValue js_element_proto(JSContext *ctx) {
  dom_init(ctx);
  return JS_DupValue(ctx, element_proto);
}

void element_setup(JSContext *ctx, JSValueConst elem, const char *tag) {
  AppContext *app = JS_GetContextOpaque(ctx);

  char upper[64];
  size_t i = 0;
  for (; tag && tag[i] && i < sizeof(upper) - 1; i++) upper[i] = (char)toupper((unsigned char)tag[i]);
  upper[i] = 0;

  JS_SetPropertyStr(ctx, elem, "tagName",  JS_NewString(ctx, upper));
  JS_SetPropertyStr(ctx, elem, "nodeName", JS_NewString(ctx, upper));
  JS_SetPropertyStr(ctx, elem, "nodeType", JS_NewInt32(ctx, 1));
  JS_SetPropertyStr(ctx, elem, "id", JS_NewString(ctx, ""));
  JS_SetPropertyStr(ctx, elem, "className", JS_NewString(ctx, ""));
  JS_SetPropertyStr(ctx, elem, "innerHTML", JS_NewString(ctx, ""));
  JS_SetPropertyStr(ctx, elem, "textContent", JS_NewString(ctx, ""));
  JS_SetPropertyStr(ctx, elem, "parentNode", JS_NULL);
  JS_SetPropertyStr(ctx, elem, "parentElement", JS_NULL);
  JS_SetPropertyStr(ctx, elem, "dataset", JS_NewObject(ctx));

  JSValue kids = JS_NewArray(ctx);
  JS_SetPropertyStr(ctx, elem, "children", JS_DupValue(ctx, kids));
  JS_SetPropertyStr(ctx, elem, "childNodes", kids);

  // el style, con los metodos de CSSStyleDeclaration (setProperty...: los
  // pone polyfill/dom.js; los elementos de antes de el no los tienen)
  JSValue global = JS_GetGlobalObject(ctx);
  JSValue css = JS_GetPropertyStr(ctx, global, "CSSStyleDeclaration");
  JSValue css_proto = JS_IsObject(css) ? JS_GetPropertyStr(ctx, css, "prototype") : JS_UNDEFINED;
  JSValue style = JS_IsObject(css_proto) ? JS_NewObjectProto(ctx, css_proto) : JS_NewObject(ctx);
  JS_FreeValue(ctx, css_proto);
  JS_FreeValue(ctx, css);
  JS_FreeValue(ctx, global);
  JS_SetPropertyStr(ctx, style, "width",    JS_NewInt32(ctx, app->cfg->game.width));
  JS_SetPropertyStr(ctx, style, "height",   JS_NewInt32(ctx, app->cfg->game.height));
  JS_SetPropertyStr(ctx, style, "position", JS_NewInt32(ctx,  0));
  JS_SetPropertyStr(ctx, style, "top",      JS_NewInt32(ctx,  0));
  JS_SetPropertyStr(ctx, style, "left",     JS_NewInt32(ctx,  0));
  JS_SetPropertyStr(ctx, style, "zIndex",   JS_NewInt32(ctx,  0));
  JS_SetPropertyStr(ctx, elem, "style", style);

  JS_SetPropertyStr(ctx, elem, "clientWidth",  JS_NewInt32(ctx, app->cfg->game.width));
  JS_SetPropertyStr(ctx, elem, "clientHeight", JS_NewInt32(ctx, app->cfg->game.height));
  JS_SetPropertyStr(ctx, elem, "offsetWidth",  JS_NewInt32(ctx, app->cfg->game.width));
  JS_SetPropertyStr(ctx, elem, "offsetHeight", JS_NewInt32(ctx, app->cfg->game.height));
}

JSValue js_create_element(JSContext *ctx, const char *tag) {
  JSValue elem;
  if (tag && strcasecmp(tag, "canvas") == 0) {
    elem = js_create_canvas(ctx);
  } else if (tag && strcasecmp(tag, "img") == 0) {
    elem = js_create_image(ctx);
  } else {
    elem = JS_NewObjectProto(ctx, element_proto);
    element_setup(ctx, elem, tag ? tag : "div");
  }
  return elem;
}

static JSValue js_document_createElement(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1) return JS_ThrowTypeError(ctx, "createElement: tag required");
  const char *tag = JS_ToCString(ctx, argv[0]);
  if (!tag) return JS_EXCEPTION;
  JSValue elem = js_create_element(ctx, tag);
  JS_FreeCString(ctx, tag);
  return elem;
}

static JSValue js_document_createTextNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  JSValue node = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, node, "nodeType", JS_NewInt32(ctx, 3));
  JS_SetPropertyStr(ctx, node, "textContent", argc > 0 ? JS_ToString(ctx, argv[0]) : JS_NewString(ctx, ""));
  return node;
}
