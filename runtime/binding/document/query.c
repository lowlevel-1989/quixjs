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

// runtime/binding/document/query.c: buscar elementos: los registrados por id,
// getElementById, getElementsByTagName y querySelector.
// Parte de runtime/binding/document.c (archivo indice).

#include "common.h"

void document_register_id(JSContext *ctx, JSValue obj, const char *id) {
  element_registry = realloc(element_registry, sizeof(ElementEntry) * (element_registry_count + 1));
  element_registry[element_registry_count].id = strdup(id);
  element_registry[element_registry_count].obj = JS_DupValue(ctx, obj);
  element_registry_count++;
}

static JSValue document_registered(JSContext *ctx, JSValueConst this_val,
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

// [html] elements del toml: el elemento de ese id, o NULL
static const HtmlElement *html_element(AppContext *app, const char *id) {
  for (int i = 0; i < app->cfg->html_count; i++)
    if (strcmp(app->cfg->html[i].id, id) == 0) return &app->cfg->html[i];
  return NULL;
}

static JSValue js_document_getElementById(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {

  if (argc < 1) {
    return JS_NULL;
  }
  const char *id = JS_ToCString(ctx, argv[0]);
  if (!id) {
    return JS_EXCEPTION;
  }

  AppContext *app = JS_GetContextOpaque(ctx);

  JSValue elem = document_registered(ctx, this_val, argc, argv);
  // con el.id = "...": se busca en el documento (y se registra)
  if (JS_IsNull(elem)) {
    JSValue body = JS_GetPropertyStr(ctx, this_val, "body");
    if (JS_IsObject(body)) elem = find_by_id(ctx, body, id, 0);
    JS_FreeValue(ctx, body);
    if (!JS_IsNull(elem)) document_register_id(ctx, elem, id);
  }

  // El canvas del juego (engine.canvas_id) existe siempre, como si estuviera
  // en el index.html, y los de [html] elements del toml: se crean al pedirlos,
  // dentro del body
  const HtmlElement *h = html_element(app, id);
  int is_canvas = strcmp(id, app->cfg->engine.canvas_id) == 0;
  if (JS_IsNull(elem) && (is_canvas || h)) {
    elem = js_create_element(ctx, is_canvas ? "canvas" : h->tag);
    JS_SetPropertyStr(ctx, elem, "id", JS_NewString(ctx, id));
    if (!is_canvas && h->type[0]) JS_SetPropertyStr(ctx, elem, "type", JS_NewString(ctx, h->type));
    document_register_id(ctx, elem, id);
    if (is_canvas) screen_attach(ctx, elem);
    JSValue body = JS_GetPropertyStr(ctx, this_val, "body");
    if (JS_IsObject(body)) JS_FreeValue(ctx, js_element_appendChild(ctx, body, 1, &elem));
    JS_FreeValue(ctx, body);
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
    r = js_document_getElementById(ctx, this_val, 1, &id);
    JS_FreeValue(ctx, id);
  } else if (strcmp(sel, "body") == 0 || strcmp(sel, "head") == 0) {
    r = JS_GetPropertyStr(ctx, this_val, sel);
  }
  JS_FreeCString(ctx, sel);
  return r;
}
