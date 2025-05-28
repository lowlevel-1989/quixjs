/* File generated automatically by the QuickJS-ng compiler. */

#include "quickjs-libc.h"

extern const uint32_t qjsc_pixi_size;
extern const uint8_t  qjsc_pixi[];
extern const uint32_t qjsc_hello_size;
extern const uint8_t  qjsc_hello[];


void js_add_window(JSContext * ctx)
{
  JSValue global_obj, window, navigator;

  global_obj = JS_GetGlobalObject(ctx);
  window    = JS_NewObject(ctx);
  navigator = JS_NewObject(ctx);
  JS_SetPropertyStr(ctx, global_obj, "window",    window);
  JS_SetPropertyStr(ctx, global_obj, "navigator", navigator);

  JS_FreeValue(ctx, global_obj);
}

int main(int argc, char **argv)
{
  int r;
  JSRuntime *rt;
  JSContext *ctx;
  r = 0;
  rt = JS_NewRuntime();
  js_std_set_worker_new_context_func(JS_NewContext);
  js_std_init_handlers(rt);
  JS_SetModuleLoaderFunc(rt, NULL, js_module_loader, NULL);
  ctx = JS_NewContext(rt);
  js_std_add_helpers(ctx, argc, argv);

  js_add_window(ctx);

  js_std_eval_binary(ctx, qjsc_pixi,  qjsc_pixi_size,  0);
  js_std_eval_binary(ctx, qjsc_hello, qjsc_hello_size, 0);
  r = js_std_loop(ctx);
  if (r) {
    js_std_dump_error(ctx);
  }
  js_std_free_handlers(rt);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);
  return r;
}
