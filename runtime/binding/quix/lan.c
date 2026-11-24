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

// runtime/binding/quix/lan.c: quix.lan, la red local para juegos en JS
// (runtime/lan). Lo que da cada funcion: ver common.h.

#include "core/dispatch.h"
#include <string.h>
#include "core/app_context.h"
#include "core/config/loader.h"
#include "lan/lan.h"
#include "common.h"

static JSValue js_quix_lan_open(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val, (void)argc, (void)argv;
  AppContext *app = JS_GetContextOpaque(ctx);
  return JS_NewBool(ctx, app && app->cfg && lan_open(config_game_id(app->cfg)));
}

static JSValue js_quix_lan_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)ctx, (void)this_val, (void)argc, (void)argv;
  lan_close();
  return JS_UNDEFINED;
}

static JSValue js_quix_lan_get_id(JSContext *ctx, JSValueConst this_val) {
  (void)this_val;
  return lan_my_id() ? JS_NewString(ctx, lan_my_id()) : JS_NULL;
}

static JSValue js_quix_lan_get_name(JSContext *ctx, JSValueConst this_val) {
  (void)this_val;
  return lan_my_name() ? JS_NewString(ctx, lan_my_name()) : JS_NULL;
}

static JSValue js_quix_lan_peers(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val, (void)argc, (void)argv;
  lan_update();
  LanPeer peers[LAN_MAX_PEERS];
  int n = lan_peers(peers, LAN_MAX_PEERS);
  JSValue list = JS_NewArray(ctx);
  for (int i = 0; i < n; i++) {
    JSValue o = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, o, "id", JS_NewString(ctx, peers[i].id));
    JS_SetPropertyStr(ctx, o, "name", JS_NewString(ctx, peers[i].name));
    JS_SetPropertyUint32(ctx, list, (uint32_t)i, o);
  }
  return list;
}

// el canal lan del dispatch (core/dispatch.h): los envios salen por el, al
// instante (devuelve si se envio)
typedef struct {
  const char *to;
  const void *data;
  size_t      size;
  int        *ret;
} LanSend;

static int lan_dispatch(void *user, const void *item) {
  (void)user;
  const LanSend *m = item;
  *m->ret = lan_send(m->to, m->data, m->size);
  return 0;
}

static DispatchChannel lan_channel = { .name = "lan", .send = lan_dispatch };

static JSValue js_quix_lan_send(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val;
  if (argc < 1) return JS_FALSE;
  size_t size = 0;
  const char *text = JS_ToCStringLen(ctx, &size, argv[0]);
  if (!text) return JS_EXCEPTION;
  const char *to = argc > 1 && JS_IsString(argv[1]) ? JS_ToCString(ctx, argv[1]) : NULL;
  int sent = 0;
  LanSend m = { to, text, size, &sent };
  dispatch_push(&lan_channel, &m, sizeof m);
  if (to) JS_FreeCString(ctx, to);
  JS_FreeCString(ctx, text);
  return JS_NewBool(ctx, sent);
}

static JSValue js_quix_lan_receive(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  (void)this_val, (void)argc, (void)argv;
  lan_update();
  JSValue list = JS_NewArray(ctx);
  static uint8_t data[LAN_MAX_MESSAGE];
  char from[5];
  size_t size = sizeof data;
  for (uint32_t i = 0; lan_message(from, data, &size); i++, size = sizeof data) {
    JSValue o = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, o, "from", JS_NewString(ctx, from));
    JS_SetPropertyStr(ctx, o, "data", JS_NewStringLen(ctx, (const char *)data, size));
    JS_SetPropertyUint32(ctx, list, i, o);
  }
  return list;
}

static const JSCFunctionListEntry js_lan_funcs[] = {
  JS_CFUNC_DEF("open", 0, js_quix_lan_open),
  JS_CFUNC_DEF("close", 0, js_quix_lan_close),
  JS_CFUNC_DEF("peers", 0, js_quix_lan_peers),
  JS_CFUNC_DEF("send", 2, js_quix_lan_send),
  JS_CFUNC_DEF("receive", 0, js_quix_lan_receive),
  JS_CGETSET_DEF("id", js_quix_lan_get_id, NULL),
  JS_CGETSET_DEF("name", js_quix_lan_get_name, NULL),
};

void quix_add_lan(JSContext *ctx, JSValueConst quix) {
  quix_add_group(ctx, quix, "lan", js_lan_funcs, countof(js_lan_funcs));
}
