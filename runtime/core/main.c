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
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "quickjs.h"
#include "quickjs-libc.h"
#include "core/args.h"
#include "core/config_loader.h"
#include "core/printer.h"
#include "core/compiler.h"
#include "core/runner.h"
#include "core/timer.h"
#include "core/app_context.h"
#include "binding/timer.h"
#include "binding/document.h"
#include "binding/window.h"
#include "backend/backend.h"

static int process_events(JSContext *ctx, Backend* backend) {
  JSRuntime *rt = JS_GetRuntime(ctx);

  backend_process_events(backend);

  // Ejecutar todos los trabajos pendientes (promesas, etc.)
  JSContext *ctx1;
  int err;
  while ((err = JS_ExecutePendingJob(rt, &ctx1))) {
    if (err < 0) {
      js_std_dump_error(ctx1);
      return 1;
    }
  }

  // ejecuta procesos de los timers
  err = timer_process(ctx, backend);

  return err;
}

int main(int argc, char **argv) {
  Args args;
  parse_args(argc, argv, &args);

  if (args.show_help) {
    print_help();
    return 0;
  }

  printf("Using config: %s\n", args.config_path);
  if (args.force_recompile)
    printf("Force recompilation enabled.\n");

  Config cfg = {0};
  if (config_load(&cfg, args.config_path) != 0) {
    fprintf(stderr, "Failed to load config file: %s\n", args.config_path);
    return 1;
  }

  print_config(&cfg);

  // Crear el backend gráfico si se especifican dimensiones
  Backend* backend = NULL;
  int backend_type = args.backend;

  if (!backend_type) backend_type = BACKEND_SDL;

  // Crear el backend seleccionado
  backend = backend_create(backend_type);
  if (!backend) {
    fprintf(stderr, "Failed to create backend: %s\n", backend_type);
    return 1;
  }

  backend_init(backend, cfg.game.width, cfg.game.height);

  BytecodeBundle bundle = {0};

  JSRuntime *rt;
  JSContext *ctx;

  rt = JS_NewRuntime();
  if (!rt) {
    fprintf(stderr, "Failed to create JS runtime\n");
    if (backend) {
      backend_cleanup(backend);
      free(backend);
    }
    return 1;
  }

  js_std_set_worker_new_context_func(JS_NewContext);
  js_std_init_handlers(rt);
  // Set the module loader function so that `import` statements can resolve modules from disk.
  JS_SetModuleLoaderFunc2(rt, NULL, js_module_loader, js_module_check_attributes, NULL);

  ctx = JS_NewContext(rt);
  if (!ctx) {
    fprintf(stderr, "Failed to create JS context\n");
    JS_FreeRuntime(rt);
    if (backend) {
      backend_cleanup(backend);
      free(backend);
    }
    return 1;
  }
  js_std_add_helpers(ctx, argc, argv);

  AppContext* app = malloc(sizeof(AppContext));
  app->backend = backend;
  app->cfg = &cfg;

  JS_SetContextOpaque(ctx, app);
  js_api_add_window(ctx);
  js_api_add_timer(ctx);

  if (compile_scripts(ctx, &cfg, args.force_recompile, &bundle) != 0) {
    fprintf(stderr, "Failed to compile scripts\n");

    free_bytecode_bundle(ctx, &bundle);

    js_std_free_handlers(rt);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    if (backend) {
      backend_cleanup(backend);
      free(backend);
    }
    return 1;
  }

  if (runner_run_bundle(ctx, &bundle) != 0) {
    fprintf(stderr, "Error executing bytecode\n");

    free_bytecode_bundle(ctx, &bundle);

    js_std_free_handlers(rt);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    if (backend) {
      backend_cleanup(backend);
      free(backend);
    }
    return 1;
  }

  int should_exit;
  // Bucle principal
  while (1) {
    // Procesar eventos y trabajos pendientes
    int should_exit = process_events(ctx, backend);

    // Condición 1: Si hay backend y se solicitó cerrar
    if (backend && backend_should_close(backend)) {
      should_exit = 1;
    }

    // Condición 2: No hay más trabajo pendiente
    if (timer_count() == 0 && !JS_IsJobPending(rt)) {
      should_exit = 1;
    }

    if (should_exit) {
      break;
    }

  }

  free_bytecode_bundle(ctx, &bundle);
  timer_free(ctx);
  free_element_registry(ctx);

  js_std_free_handlers(rt);
  JS_FreeContext(ctx);
  JS_FreeRuntime(rt);

  if (backend) {
    backend_cleanup(backend);
    free(backend);
  }

  return 0;
}

