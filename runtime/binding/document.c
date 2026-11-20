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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "canvas.h"
#include "image.h"
#include "events.h"
#include "document.h"
#include "script.h"
#include "webgl.h"
#include "core/app_context.h"

// Un DOM minimo: lo justo para que las librerias encuentren body, puedan
// crear elementos, agregar el canvas y registrar eventos. Solo los canvas
// tienen comportamiento real; el resto son objetos con las propiedades
// habituales.

typedef struct {
  char     *id;
  JSValue  obj;
} ElementEntry;

static ElementEntry *element_registry       = NULL;
static int           element_registry_count = 0;

static JSValue element_proto = { 0 };
static JSValue screen_list   = { 0 };   // canvas agregados al documento (en orden)
static int     document_ready = 0;

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

  if (document_ready) {
    JS_FreeValue(ctx, element_proto);
    JS_FreeValue(ctx, screen_list);
    document_ready = 0;
  }
}

// ------------------------------------------------------------ pantalla

static uint32_t array_length(JSContext *ctx, JSValueConst arr) {
  uint32_t len = 0;
  JSValue v = JS_GetPropertyStr(ctx, arr, "length");
  JS_ToUint32(ctx, &len, v);
  JS_FreeValue(ctx, v);
  return len;
}

static int array_index_of(JSContext *ctx, JSValueConst arr, JSValueConst item) {
  uint32_t len = array_length(ctx, arr);
  for (uint32_t i = 0; i < len; i++) {
    JSValue v = JS_GetPropertyUint32(ctx, arr, i);
    int same = JS_StrictEq(ctx, v, item);
    JS_FreeValue(ctx, v);
    if (same) return (int)i;
  }
  return -1;
}

static void array_remove_at(JSContext *ctx, JSValueConst arr, int idx) {
  JSValue splice = JS_GetPropertyStr(ctx, arr, "splice");
  JSValue args[2] = { JS_NewInt32(ctx, idx), JS_NewInt32(ctx, 1) };
  JSValue r = JS_Call(ctx, splice, arr, 2, args);
  JS_FreeValue(ctx, r);
  JS_FreeValue(ctx, splice);
}

static void screen_attach(JSContext *ctx, JSValueConst canvas) {
  if (!document_ready || !js_canvas_get(ctx, canvas)) return;
  if (array_index_of(ctx, screen_list, canvas) >= 0) return;
  JS_SetPropertyUint32(ctx, screen_list, array_length(ctx, screen_list), JS_DupValue(ctx, canvas));
}

static void screen_detach(JSContext *ctx, JSValueConst canvas) {
  if (!document_ready) return;
  int idx = array_index_of(ctx, screen_list, canvas);
  if (idx >= 0) array_remove_at(ctx, screen_list, idx);
}

static int style_hidden(JSContext *ctx, JSValueConst el) {
  JSValue style = JS_GetPropertyStr(ctx, el, "style");
  int hidden = 0;
  if (JS_IsObject(style)) {
    JSValue display = JS_GetPropertyStr(ctx, style, "display");
    JSValue visibility = JS_GetPropertyStr(ctx, style, "visibility");
    JSValue opacity = JS_GetPropertyStr(ctx, style, "opacity");
    const char *d = JS_IsString(display) ? JS_ToCString(ctx, display) : NULL;
    const char *v = JS_IsString(visibility) ? JS_ToCString(ctx, visibility) : NULL;
    if (d && strcmp(d, "none") == 0) hidden = 1;
    if (v && strcmp(v, "hidden") == 0) hidden = 1;
    // opacity 0: no se ve (RPG Maker MV deja asi su UpperCanvas, encima de
    // todo, salvo mientras carga). Sin esto se dibujaba en cada imagen.
    if (JS_IsNumber(opacity) || JS_IsString(opacity)) {
      // "" (sin fijar) no cuenta
      const char *o = JS_ToCString(ctx, opacity);
      if (o) {
        char *end = NULL;
        double val = strtod(o, &end);
        if (end != o && val <= 0) hidden = 1;
        JS_FreeCString(ctx, o);
      }
    }
    if (d) JS_FreeCString(ctx, d);
    if (v) JS_FreeCString(ctx, v);
    JS_FreeValue(ctx, display);
    JS_FreeValue(ctx, visibility);
    JS_FreeValue(ctx, opacity);
  }
  JS_FreeValue(ctx, style);
  return hidden;
}

int document_screen_canvases(JSContext *ctx, CanvasData **out, int max) {
  if (!document_ready) return 0;
  int n = 0;
  uint32_t len = array_length(ctx, screen_list);
  for (uint32_t i = 0; i < len && n < max; i++) {
    JSValue c = JS_GetPropertyUint32(ctx, screen_list, i);
    CanvasData *cd = js_canvas_get(ctx, c);
    if (cd && !style_hidden(ctx, c)) out[n++] = cd;
    JS_FreeValue(ctx, c);
  }
  return n;
}

// ------------------------------------------------------------ elementos

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
        register_element_with_id(ctx, (JSValue)this_val, id);
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

  JSValue style = JS_NewObject(ctx);
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

static JSValue js_createElement(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1) return JS_ThrowTypeError(ctx, "createElement: tag required");
  const char *tag = JS_ToCString(ctx, argv[0]);
  if (!tag) return JS_EXCEPTION;
  JSValue elem = js_create_element(ctx, tag);
  JS_FreeCString(ctx, tag);
  return elem;
}

static JSValue js_createTextNode(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  JSValue node = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, node, "nodeType", JS_NewInt32(ctx, 3));
  JS_SetPropertyStr(ctx, node, "textContent", argc > 0 ? JS_ToString(ctx, argv[0]) : JS_NewString(ctx, ""));
  return node;
}

JSValue js_get_element_by_id(JSContext *ctx, JSValueConst this_val,
    int argc, JSValueConst *argv) {

  const char *id = JS_ToCString(ctx, argv[0]);
  if (!id) return JS_EXCEPTION;
  for (int i = element_registry_count - 1; i >= 0; i--) {
    if (strcmp(element_registry[i].id, id) == 0) {
      JS_FreeCString(ctx, id);
      return JS_DupValue(ctx, element_registry[i].obj);
    }
  }

  JS_FreeCString(ctx, id);
  return JS_NULL;
}

// el elemento de id dentro de node (sus childNodes, en orden), o JS_NULL:
// para los que recibieron el id con el.id = "..." (no pasan por
// setAttribute, que es lo que los registra)
static JSValue find_by_id(JSContext *ctx, JSValueConst node, const char *id, int depth) {
  JSValue kids = JS_GetPropertyStr(ctx, node, "childNodes");
  uint32_t len = JS_IsObject(kids) && depth < 64 ? array_length(ctx, kids) : 0;
  JSValue found = JS_NULL;
  for (uint32_t i = 0; i < len && JS_IsNull(found); i++) {
    JSValue kid = JS_GetPropertyUint32(ctx, kids, i);
    JSValue kid_id = JS_GetPropertyStr(ctx, kid, "id");
    const char *s = JS_IsString(kid_id) ? JS_ToCString(ctx, kid_id) : NULL;
    if (s && strcmp(s, id) == 0) found = JS_DupValue(ctx, kid);
    else if (JS_IsObject(kid)) found = find_by_id(ctx, kid, id, depth + 1);
    if (s) JS_FreeCString(ctx, s);
    JS_FreeValue(ctx, kid_id);
    JS_FreeValue(ctx, kid);
  }
  JS_FreeValue(ctx, kids);
  return found;
}

static JSValue js_getElementById(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

  if (argc < 1) {
    return JS_NULL;
  }
  const char *id = JS_ToCString(ctx, argv[0]);
  if (!id) {
    return JS_EXCEPTION;
  }

  AppContext *app = JS_GetContextOpaque(ctx);

  JSValue elem = js_get_element_by_id(ctx, this_val, argc, argv);
  // con el.id = "...": se busca en el documento (y se registra)
  if (JS_IsNull(elem)) {
    JSValue body = JS_GetPropertyStr(ctx, this_val, "body");
    if (JS_IsObject(body)) elem = find_by_id(ctx, body, id, 0);
    JS_FreeValue(ctx, body);
    if (!JS_IsNull(elem)) register_element_with_id(ctx, elem, id);
  }

  // El canvas del juego (engine.canvas_id) existe siempre, como si estuviera
  // en el index.html
  if (JS_IsNull(elem) && strcmp(id, app->cfg->engine.canvas_id) == 0) {
    elem = js_create_element(ctx, "canvas");
    JS_SetPropertyStr(ctx, elem, "id", JS_NewString(ctx, id));
    register_element_with_id(ctx, elem, id);
    screen_attach(ctx, elem);
  }

  JS_FreeCString(ctx, id);
  return elem;
}

// Recorre el arbol (en orden de documento) y agrega a list los elementos con
// ese tagName ("*" = todos)
static void collect_by_tag(JSContext *ctx, JSValueConst node, const char *tag, JSValueConst list, uint32_t *n, int depth) {
  if (depth > 64) return;
  JSValue kids = JS_GetPropertyStr(ctx, node, "childNodes");
  if (JS_IsObject(kids)) {
    uint32_t len = array_length(ctx, kids);
    for (uint32_t i = 0; i < len; i++) {
      JSValue k = JS_GetPropertyUint32(ctx, kids, i);
      if (JS_IsObject(k)) {
        JSValue t = JS_GetPropertyStr(ctx, k, "tagName");
        const char *ts = JS_IsString(t) ? JS_ToCString(ctx, t) : NULL;
        if (ts && (strcmp(tag, "*") == 0 || strcasecmp(ts, tag) == 0)) {
          JS_SetPropertyUint32(ctx, list, (*n)++, JS_DupValue(ctx, k));
        }
        if (ts) JS_FreeCString(ctx, ts);
        JS_FreeValue(ctx, t);
        collect_by_tag(ctx, k, tag, list, n, depth + 1);
      }
      JS_FreeValue(ctx, k);
    }
  }
  JS_FreeValue(ctx, kids);
}

static JSValue js_element_getElementsByTagName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  JSValue list = JS_NewArray(ctx);
  if (argc < 1) return list;
  const char *tag = JS_ToCString(ctx, argv[0]);
  if (!tag) return list;
  uint32_t n = 0;
  collect_by_tag(ctx, this_val, tag, list, &n, 0);
  JS_FreeCString(ctx, tag);
  return list;
}

static JSValue js_document_getElementsByTagName(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  JSValue list = JS_NewArray(ctx);
  if (argc < 1) return list;
  const char *tag = JS_ToCString(ctx, argv[0]);
  if (!tag) return list;
  uint32_t n = 0;
  JSValue html = JS_GetPropertyStr(ctx, this_val, "documentElement");
  if (strcasecmp(tag, "html") == 0 || strcmp(tag, "*") == 0) JS_SetPropertyUint32(ctx, list, n++, JS_DupValue(ctx, html));
  collect_by_tag(ctx, html, tag, list, &n, 0);
  JS_FreeValue(ctx, html);
  JS_FreeCString(ctx, tag);
  return list;
}

static JSValue js_document_querySelector(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1) return JS_NULL;
  const char *sel = JS_ToCString(ctx, argv[0]);
  if (!sel) return JS_EXCEPTION;
  JSValue r = JS_NULL;
  if (sel[0] == '#') {
    JSValue id = JS_NewString(ctx, sel + 1);
    r = js_getElementById(ctx, this_val, 1, &id);
    JS_FreeValue(ctx, id);
  } else if (strcmp(sel, "body") == 0 || strcmp(sel, "head") == 0) {
    r = JS_GetPropertyStr(ctx, this_val, sel);
  }
  JS_FreeCString(ctx, sel);
  return r;
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
  JS_CFUNC_DEF("createElement",        1, js_createElement),
  JS_CFUNC_DEF("createTextNode",       1, js_createTextNode),
  JS_CFUNC_DEF("getElementById",       1, js_getElementById),
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
