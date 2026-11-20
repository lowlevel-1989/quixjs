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

// EventTarget y los constructores de eventos. Ver events.h.

#include <stdio.h>
#include <string.h>
#include "core/timer.h"
#include "events.h"

#define LISTENERS_PROP "__listeners__"

static JSValue event_ctor    = { 0 };
static JSValue keyboard_ctor = { 0 };
static int     ctors_ready   = 0;

// window.addEventListener(...) llamado sin "this" (addEventListener(...))
static JSValue resolve_target(JSContext *ctx, JSValueConst this_val) {
  if (JS_IsUndefined(this_val) || JS_IsNull(this_val)) return JS_GetGlobalObject(ctx);
  return JS_DupValue(ctx, this_val);
}

static JSValue get_listener_list(JSContext *ctx, JSValueConst target, const char *type, int create) {
  JSValue all = JS_GetPropertyStr(ctx, target, LISTENERS_PROP);
  if (JS_IsUndefined(all)) {
    if (!create) return JS_UNDEFINED;
    all = JS_NewObject(ctx);
    JS_DefinePropertyValueStr(ctx, target, LISTENERS_PROP, JS_DupValue(ctx, all),
                              JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE);
  }
  JSValue list = JS_GetPropertyStr(ctx, all, type);
  if (JS_IsUndefined(list) && create) {
    list = JS_NewArray(ctx);
    JS_SetPropertyStr(ctx, all, type, JS_DupValue(ctx, list));
  }
  JS_FreeValue(ctx, all);
  return list;
}

static uint32_t array_length(JSContext *ctx, JSValueConst arr) {
  uint32_t len = 0;
  JSValue v = JS_GetPropertyStr(ctx, arr, "length");
  JS_ToUint32(ctx, &len, v);
  JS_FreeValue(ctx, v);
  return len;
}

static int listener_options_once(JSContext *ctx, JSValueConst options) {
  if (!JS_IsObject(options)) return 0;
  JSValue once = JS_GetPropertyStr(ctx, options, "once");
  int r = JS_ToBool(ctx, once);
  JS_FreeValue(ctx, once);
  return r;
}

static JSValue js_addEventListener(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 2 || !(JS_IsFunction(ctx, argv[1]) || JS_IsObject(argv[1]))) return JS_UNDEFINED;
  const char *type = JS_ToCString(ctx, argv[0]);
  if (!type) return JS_EXCEPTION;

  JSValue target = resolve_target(ctx, this_val);
  JSValue list = get_listener_list(ctx, target, type, 1);
  JS_FreeCString(ctx, type);

  // el mismo listener no se registra dos veces
  uint32_t len = array_length(ctx, list);
  for (uint32_t i = 0; i < len; i++) {
    JSValue entry = JS_GetPropertyUint32(ctx, list, i);
    JSValue fn = JS_GetPropertyStr(ctx, entry, "fn");
    int same = JS_StrictEq(ctx, fn, argv[1]);
    JS_FreeValue(ctx, fn);
    JS_FreeValue(ctx, entry);
    if (same) {
      JS_FreeValue(ctx, list);
      JS_FreeValue(ctx, target);
      return JS_UNDEFINED;
    }
  }

  JSValue entry = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, entry, "fn", JS_DupValue(ctx, argv[1]));
  JS_SetPropertyStr(ctx, entry, "once", JS_NewBool(ctx, argc > 2 && listener_options_once(ctx, argv[2])));
  JS_SetPropertyUint32(ctx, list, len, entry);

  JS_FreeValue(ctx, list);
  JS_FreeValue(ctx, target);
  return JS_UNDEFINED;
}

static void remove_listener(JSContext *ctx, JSValueConst list, JSValueConst fn) {
  uint32_t len = array_length(ctx, list);
  for (uint32_t i = 0; i < len; i++) {
    JSValue entry = JS_GetPropertyUint32(ctx, list, i);
    JSValue efn = JS_GetPropertyStr(ctx, entry, "fn");
    int same = JS_StrictEq(ctx, efn, fn);
    JS_FreeValue(ctx, efn);
    JS_FreeValue(ctx, entry);
    if (same) {
      JSValue splice = JS_GetPropertyStr(ctx, list, "splice");
      JSValue args[2] = { JS_NewUint32(ctx, i), JS_NewInt32(ctx, 1) };
      JSValue r = JS_Call(ctx, splice, list, 2, args);
      JS_FreeValue(ctx, r);
      JS_FreeValue(ctx, splice);
      return;
    }
  }
}

static JSValue js_removeEventListener(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 2) return JS_UNDEFINED;
  const char *type = JS_ToCString(ctx, argv[0]);
  if (!type) return JS_EXCEPTION;

  JSValue target = resolve_target(ctx, this_val);

  JSValue list = get_listener_list(ctx, target, type, 0);
  JS_FreeCString(ctx, type);
  if (!JS_IsUndefined(list)) remove_listener(ctx, list, argv[1]);
  JS_FreeValue(ctx, list);
  JS_FreeValue(ctx, target);
  return JS_UNDEFINED;
}

static int event_flag(JSContext *ctx, JSValueConst event, const char *name) {
  JSValue v = JS_GetPropertyStr(ctx, event, name);
  int r = JS_ToBool(ctx, v);
  JS_FreeValue(ctx, v);
  return r;
}

static int call_listener(JSContext *ctx, JSValueConst fn, JSValueConst target, JSValueConst event) {
  JSValue ret;
  if (JS_IsFunction(ctx, fn)) {
    ret = JS_Call(ctx, fn, target, 1, &event);
  } else {
    JSValue handle = JS_GetPropertyStr(ctx, fn, "handleEvent");
    if (!JS_IsFunction(ctx, handle)) {
      JS_FreeValue(ctx, handle);
      return 0;
    }
    ret = JS_Call(ctx, handle, fn, 1, &event);
    JS_FreeValue(ctx, handle);
  }
  if (JS_IsException(ret)) {
    js_std_dump_error(ctx);
    return -1;
  }
  JS_FreeValue(ctx, ret);
  return 0;
}

int events_dispatch(JSContext *ctx, JSValueConst target, JSValueConst event) {
  JSValue type_val = JS_GetPropertyStr(ctx, event, "type");
  const char *type = JS_ToCString(ctx, type_val);
  JS_FreeValue(ctx, type_val);
  if (!type) return -1;

  JSValue cur_target = JS_GetPropertyStr(ctx, event, "target");
  if (JS_IsUndefined(cur_target) || JS_IsNull(cur_target)) {
    JS_SetPropertyStr(ctx, event, "target", JS_DupValue(ctx, target));
  }
  JS_FreeValue(ctx, cur_target);
  JS_SetPropertyStr(ctx, event, "currentTarget", JS_DupValue(ctx, target));

  int result = 0;

  // nombre del manejador on<type> antes de liberar type
  char on_name[64];
  snprintf(on_name, sizeof(on_name), "on%s", type);

  JSValue list = get_listener_list(ctx, target, type, 0);
  JS_FreeCString(ctx, type);

  if (!JS_IsUndefined(list)) {
    // se itera sobre una copia: un listener puede quitar o agregar otros
    JSValue slice = JS_GetPropertyStr(ctx, list, "slice");
    JSValue copy = JS_Call(ctx, slice, list, 0, NULL);
    JS_FreeValue(ctx, slice);

    uint32_t len = array_length(ctx, copy);
    for (uint32_t i = 0; i < len; i++) {
      JSValue entry = JS_GetPropertyUint32(ctx, copy, i);
      JSValue fn = JS_GetPropertyStr(ctx, entry, "fn");
      if (event_flag(ctx, entry, "once")) remove_listener(ctx, list, fn);
      if (call_listener(ctx, fn, target, event) != 0) result = -1;
      JS_FreeValue(ctx, fn);
      JS_FreeValue(ctx, entry);
      if (event_flag(ctx, event, "__stopImmediate")) break;
    }
    JS_FreeValue(ctx, copy);
  }
  JS_FreeValue(ctx, list);

  // target.onload, target.onkeydown... (despues de los listeners: lo normal
  // es asignarlo despues de addEventListener)
  if (!event_flag(ctx, event, "__stopImmediate")) {
    JSValue handler = JS_GetPropertyStr(ctx, target, on_name);
    if (JS_IsFunction(ctx, handler)) {
      if (call_listener(ctx, handler, target, event) != 0) result = -1;
    }
    JS_FreeValue(ctx, handler);
  }
  return result;
}

static JSValue js_dispatchEvent(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  if (argc < 1 || !JS_IsObject(argv[0])) return JS_ThrowTypeError(ctx, "dispatchEvent: event required");
  JSValue target = resolve_target(ctx, this_val);
  events_dispatch(ctx, target, argv[0]);
  JS_FreeValue(ctx, target);
  return JS_NewBool(ctx, !event_flag(ctx, argv[0], "defaultPrevented"));
}

static const JSCFunctionListEntry js_event_target_funcs[] = {
  JS_CFUNC_DEF("addEventListener",    2, js_addEventListener),
  JS_CFUNC_DEF("removeEventListener", 2, js_removeEventListener),
  JS_CFUNC_DEF("dispatchEvent",       1, js_dispatchEvent),
};

void js_event_target_setup(JSContext *ctx, JSValueConst obj) {
  JS_SetPropertyFunctionList(ctx, obj, js_event_target_funcs, countof(js_event_target_funcs));
}

// ---------------------------------------------------------------- Event

static JSValue js_event_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
  if (argc < 1) return JS_ThrowTypeError(ctx, "Event: type required");

  JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  JSValue ev = JS_NewObjectProto(ctx, proto);
  JS_FreeValue(ctx, proto);
  if (JS_IsException(ev)) return ev;

  JSValue type = JS_ToString(ctx, argv[0]);
  JS_SetPropertyStr(ctx, ev, "type", type);
  JS_SetPropertyStr(ctx, ev, "bubbles", JS_FALSE);
  JS_SetPropertyStr(ctx, ev, "cancelable", JS_FALSE);
  JS_SetPropertyStr(ctx, ev, "defaultPrevented", JS_FALSE);
  JS_SetPropertyStr(ctx, ev, "target", JS_NULL);
  JS_SetPropertyStr(ctx, ev, "currentTarget", JS_NULL);
  JS_SetPropertyStr(ctx, ev, "timeStamp", JS_NewFloat64(ctx, clock_now_ms()));

  // los campos del init (key, code, keyCode, detail, bubbles...) se copian
  if (argc > 1 && JS_IsObject(argv[1])) {
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue object = JS_GetPropertyStr(ctx, global, "Object");
    JSValue assign = JS_GetPropertyStr(ctx, object, "assign");
    JSValue assign_args[2] = { ev, argv[1] };
    JSValue r = JS_Call(ctx, assign, object, 2, assign_args);
    JS_FreeValue(ctx, r);
    JS_FreeValue(ctx, assign);
    JS_FreeValue(ctx, object);
    JS_FreeValue(ctx, global);
  }
  return ev;
}

static JSValue js_event_preventDefault(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  JS_SetPropertyStr(ctx, this_val, "defaultPrevented", JS_TRUE);
  return JS_UNDEFINED;
}

static JSValue js_event_stopPropagation(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  JS_SetPropertyStr(ctx, this_val, "__stop", JS_TRUE);
  return JS_UNDEFINED;
}

static JSValue js_event_stopImmediatePropagation(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
  JS_SetPropertyStr(ctx, this_val, "__stop", JS_TRUE);
  JS_SetPropertyStr(ctx, this_val, "__stopImmediate", JS_TRUE);
  return JS_UNDEFINED;
}

static JSValue js_event_target_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
  JSValue proto = JS_GetPropertyStr(ctx, new_target, "prototype");
  JSValue obj = JS_NewObjectProto(ctx, proto);
  JS_FreeValue(ctx, proto);
  return obj;
}

static const JSCFunctionListEntry js_event_proto_funcs[] = {
  JS_CFUNC_DEF("preventDefault",           0, js_event_preventDefault),
  JS_CFUNC_DEF("stopPropagation",          0, js_event_stopPropagation),
  JS_CFUNC_DEF("stopImmediatePropagation", 0, js_event_stopImmediatePropagation),
};

static JSValue define_event_class(JSContext *ctx, JSValueConst global, const char *name, JSValueConst parent_proto) {
  JSValue proto = JS_NewObject(ctx);
  if (JS_IsObject(parent_proto)) JS_SetPrototype(ctx, proto, parent_proto);
  JSValue ctor = JS_NewCFunction2(ctx, js_event_ctor, name, 2, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, ctor, proto);
  JS_SetPropertyStr(ctx, global, name, JS_DupValue(ctx, ctor));
  JS_FreeValue(ctx, proto);
  return ctor;
}

void js_api_add_events(JSContext *ctx) {
  JSValue global = JS_GetGlobalObject(ctx);

  event_ctor = define_event_class(ctx, global, "Event", JS_UNDEFINED);
  JSValue event_proto = JS_GetPropertyStr(ctx, event_ctor, "prototype");
  JS_SetPropertyFunctionList(ctx, event_proto, js_event_proto_funcs, countof(js_event_proto_funcs));

  keyboard_ctor = define_event_class(ctx, global, "KeyboardEvent", event_proto);
  JSValue c;
  c = define_event_class(ctx, global, "MouseEvent", event_proto);   JS_FreeValue(ctx, c);
  c = define_event_class(ctx, global, "PointerEvent", event_proto); JS_FreeValue(ctx, c);
  c = define_event_class(ctx, global, "TouchEvent", event_proto);   JS_FreeValue(ctx, c);
  c = define_event_class(ctx, global, "CustomEvent", event_proto);  JS_FreeValue(ctx, c);
  ctors_ready = 1;

  // EventTarget como clase base (algunas librerias heredan de ella)
  JSValue target_proto = JS_NewObject(ctx);
  js_event_target_setup(ctx, target_proto);
  JSValue target_ctor = JS_NewCFunction2(ctx, js_event_target_ctor, "EventTarget", 0, JS_CFUNC_constructor, 0);
  JS_SetConstructor(ctx, target_ctor, target_proto);
  JS_SetPropertyStr(ctx, global, "EventTarget", target_ctor);
  JS_FreeValue(ctx, target_proto);

  // window es un EventTarget
  js_event_target_setup(ctx, global);

  JS_FreeValue(ctx, event_proto);
  JS_FreeValue(ctx, global);
}

void events_free(JSContext *ctx) {
  if (!ctors_ready) return;
  JS_FreeValue(ctx, event_ctor);
  JS_FreeValue(ctx, keyboard_ctor);
  ctors_ready = 0;
}

JSValue events_new(JSContext *ctx, const char *type) {
  JSValue t = JS_NewString(ctx, type);
  JSValue ev = JS_CallConstructor(ctx, event_ctor, 1, &t);
  JS_FreeValue(ctx, t);
  return ev;
}

JSValue events_new_keyboard(JSContext *ctx, const char *type, JSValueConst init) {
  JSValue args[2] = { JS_NewString(ctx, type), JS_DupValue(ctx, init) };
  JSValue ev = JS_CallConstructor(ctx, keyboard_ctor, 2, args);
  JS_FreeValue(ctx, args[0]);
  JS_FreeValue(ctx, args[1]);
  return ev;
}

static JSValue js_deferred_dispatch(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv,
                                    int magic, JSValue *data) {
  // data[0] = target, data[1] = type
  const char *type = JS_ToCString(ctx, data[1]);
  if (!type) return JS_EXCEPTION;
  JSValue ev = events_new(ctx, type);
  JS_FreeCString(ctx, type);
  events_dispatch(ctx, data[0], ev);
  JS_FreeValue(ctx, ev);
  return JS_UNDEFINED;
}

void events_dispatch_later(JSContext *ctx, JSValueConst target, const char *type) {
  JSValue data[2] = { JS_DupValue(ctx, target), JS_NewString(ctx, type) };
  JSValue fn = JS_NewCFunctionData(ctx, js_deferred_dispatch, 0, 0, 2, data);
  timer_set(ctx, fn, 0, 0, 0, NULL);
  JS_FreeValue(ctx, fn);
  JS_FreeValue(ctx, data[0]);
  JS_FreeValue(ctx, data[1]);
}
