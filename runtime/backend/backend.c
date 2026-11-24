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

// Lo comun a los backends: crear el elegido (sdl o dummy), pasar cada
// llamada a sus funciones y la cola de entrada. Ver backend.h.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "backend/dummy/backend.h"
#include "backend/sdl/sdl.h"
#include "core/config/args.h"
#include "backend.h"
#include "core/dispatch.h"

Backend* backend_create(int type) {
  Backend *backend = NULL;
  if (type == BACKEND_SDL) {
    backend = sdl_backend_create();
  } else if (type == BACKEND_DUMMY) {
    backend = dummy_backend_create();
  }
  if (backend) {
    backend->input_head = 0;
    backend->input_len = 0;
  }
  return backend;
}

void backend_init(Backend* backend, int width, int height) {
  if (backend && backend->iface && backend->iface->init) {
    backend->iface->init(backend, width, height);
  }
}

void backend_process_events(Backend* backend) {
  if (backend && backend->iface && backend->iface->process_events) {
    backend->iface->process_events(backend);
  }
  dispatch_run();   // lo pendiente del juego, al hardware (core/dispatch.h)
}

void backend_present(Backend* backend, const uint8_t *rgba, int w, int h) {
  if (backend && backend->iface && backend->iface->present) {
    backend->iface->present(backend, rgba, w, h);
  }
}

int backend_can_present_gl(Backend* backend) {
  return backend && backend->iface && backend->iface->present_gl != NULL;
}

void backend_present_gl(Backend* backend, unsigned tex, int w, int h, int premultiplied) {
  if (backend_can_present_gl(backend)) backend->iface->present_gl(backend, tex, w, h, premultiplied);
}

int backend_can_present_layers(Backend* backend) {
  return backend && backend->iface && backend->iface->present_layers != NULL;
}

void backend_present_layers(Backend* backend, const PresentLayer *layers, int n) {
  if (backend_can_present_layers(backend)) backend->iface->present_layers(backend, layers, n);
}

void backend_cleanup(Backend* backend) {
  if (backend && backend->iface && backend->iface->cleanup) {
    backend->iface->cleanup(backend);
  }
}

int backend_should_close(Backend* backend) {
  if (backend && backend->iface && backend->iface->should_close) {
    return backend->iface->should_close(backend);
  }
  return 1; // Si no hay backend, asumir que debe cerrarse
}

int backend_get_gamepads(Backend* backend, GamepadState *pads, int max) {
  if (backend && backend->iface && backend->iface->get_gamepads) {
    return backend->iface->get_gamepads(backend, pads, max);
  }
  return 0;
}

int backend_capture_input(Backend* backend, int step, char *out, size_t room) {
  if (backend && backend->iface && backend->iface->capture_input) {
    return backend->iface->capture_input(backend, step, out, room);
  }
  return -1;
}

// La vibracion pasa por el dispatch (core/dispatch.h): el juego pide y
// quixjs manda al mando una cada vez, cuando la anterior acabo, y despues una
// pausa (VIBRATE_GAP_MS). En la RG CubeXX (ROCKNIX, H700) muchas seguidas
// colgaban la consola entera: Gunfield con el mod rumble y los enemigos a x3
// (explosiones sin parar) la congelaba en uno a tres minutos; sin vibrar, no.
// Si llegan varias mientras tanto, se queda la mas fuerte.
#define VIBRATE_GAP_MS 150

typedef struct {
  Backend *backend;
  float strong, weak;
  int ms;
  double at;   // cuando se pidio (ms): lo que ya paso no se vibra
} VibrateItem;

static double vib_now(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec * 1000.0 + t.tv_nsec / 1e6;
}

static int vib_send(void *user, const void *item) {
  (void)user;
  const VibrateItem *v = item;
  Backend *b = v->backend;
  if (b->no_vibration || !b->iface || !b->iface->vibrate) return 0;
  int left = v->ms - (int)(vib_now() - v->at);
  if (v->ms > 0 && left <= 0) return 0;   // ya habria acabado
  b->iface->vibrate(b, v->strong, v->weak, v->ms > 0 ? left : 0);
  return v->ms > 0 ? left : 0;
}

// la nueva gana si es al menos igual de fuerte
static int vib_merge(const void *a, const void *b) {
  const VibrateItem *x = a, *y = b;
  float fx = x->strong > x->weak ? x->strong : x->weak;
  float fy = y->strong > y->weak ? y->strong : y->weak;
  return fy >= fx;
}

static DispatchChannel vibrate_channel = { .name = "vibrate", .gap_ms = VIBRATE_GAP_MS,
                                           .send = vib_send, .merge = vib_merge };

void backend_vibrate(Backend* backend, float strong, float weak, int ms) {
  if (!backend || backend->no_vibration || !backend->iface || !backend->iface->vibrate) return;
  VibrateItem v = { backend, strong, weak, ms > 0 ? ms : 0, vib_now() };
  dispatch_push(&vibrate_channel, &v, sizeof v);
}

int backend_test_pad(Backend* backend, int button, int type) {
  if (backend && backend->iface && backend->iface->test_pad) {
    return backend->iface->test_pad(backend, button, type);
  }
  return -1;
}

int backend_save_controller(Backend* backend, const char *pairs) {
  if (backend && backend->iface && backend->iface->save_controller) {
    return backend->iface->save_controller(backend, pairs);
  }
  return -1;
}

void backend_set_title(Backend* backend, const char *game_title) {
  if (!backend) return;
  if (game_title && game_title[0]) snprintf(backend->title, sizeof backend->title, "quixjs | %s", game_title);
  else snprintf(backend->title, sizeof backend->title, "quixjs");
  if (backend->iface && backend->iface->set_title) backend->iface->set_title(backend);
}

void backend_set_fullscreen(Backend* backend, int on) {
  if (backend && backend->iface && backend->iface->set_fullscreen) backend->iface->set_fullscreen(backend, on);
}

int backend_screen_size(Backend* backend, int *w, int *h) {
  if (backend && backend->iface && backend->iface->screen_size) return backend->iface->screen_size(backend, w, h);
  return -1;
}

double backend_refresh_rate(Backend* backend) {
  if (backend && backend->iface && backend->iface->refresh_rate) return backend->iface->refresh_rate(backend);
  return 0;
}

const char *backend_name(Backend* backend) {
  if (backend && backend->iface && backend->iface->name) return backend->iface->name;
  return "none";
}

void backend_push_input(Backend* backend, const InputEvent *ev) {
  if (!backend || backend->input_len >= INPUT_QUEUE_SIZE) return;  // cola llena: se pierde
  int idx = (backend->input_head + backend->input_len) % INPUT_QUEUE_SIZE;
  backend->input[idx] = *ev;
  backend->input_len++;
}

int backend_pop_input(Backend* backend, InputEvent *ev) {
  if (!backend || backend->input_len == 0) return 0;
  *ev = backend->input[backend->input_head];
  backend->input_head = (backend->input_head + 1) % INPUT_QUEUE_SIZE;
  backend->input_len--;
  return 1;
}

void backend_present_wait(Backend* backend) {
  if (backend && backend->iface && backend->iface->present_wait) backend->iface->present_wait(backend);
}

int backend_presented(Backend* backend, double *ms, int max) {
  if (backend && backend->iface && backend->iface->presented) return backend->iface->presented(backend, ms, max);
  return -1;
}

// ------------------------------------------------ el aviso de carga
//
// Al cargar una partida, el juego se para en imagenes de medio segundo o mas
// (crear el mapa, subir sus texturas): parece colgado. quix.loading(true) lo
// pone el hook de la carga; quix.loading(false), el del mapa ya creado. Tras
// la carga suele quedar algo pesado (el mapa al que lleva un evento): el aviso
// sigue hasta LOADING_CALM_MS de imagenes normales, o como mucho
// LOADING_MAX_MS desde que empezo.
#define LOADING_CALM_MS 1000.0
// una imagen mas larga es un paron de la carga; un juego lento (6 fps en una
// portatil: ~170 ms) no lo es, y con 150 el aviso no se iba hasta LOADING_MAX_MS
#define LOADING_SLOW_MS 400.0
#define LOADING_MAX_MS  30000.0

static double loading_now(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

// el backend con el aviso (para backend_loading_poke, que no lo recibe)
static Backend *loading_backend;
static double loading_repainted;

// Si ya habia una carga (Continue y luego su mapa), sigue la que habia, con el
// retardo mas corto de las dos
void backend_set_loading(Backend* backend, int on, double after_ms) {
  if (!backend) return;
  double now = loading_now();
  loading_backend = backend;
  if (on) {
    if (!backend->loading) {
      printf("[loading] on%s\n", after_ms > 0 ? " (shown if it takes long)" : "");
      backend->loading_since = now;
      backend->loading_tick = now;
      backend->loading_after = after_ms;
      backend->loading_shown = 0;
    } else if (after_ms < backend->loading_after) {
      backend->loading_after = after_ms;
    }
    backend->loading = 1;
  } else if (backend->loading == 1) {
    backend->loading_calm = now;
    backend->loading = 2;
  }
}

// Con retardo, solo un parón de verdad (after ms sin mostrar una imagen) lo
// enseña: una carga rapida no lo ve nunca. Una vez visto, sigue hasta el final
// (sin parpadear entre una imagen lenta y otra)
int backend_loading_visible(Backend* backend) {
  if (!backend || !backend->loading) return 0;
  if (backend->loading_shown || backend->loading_after <= 0) return 1;
  if (loading_now() - backend->loading_tick < backend->loading_after) return 0;
  backend->loading_shown = 1;
  printf("[loading] shown\n");
  return 1;
}

#define LOADING_REPAINT_MS 250.0

void backend_loading_poke(void) {
  Backend *b = loading_backend;
  if (!b || !b->loading || !b->iface || !b->iface->repaint) return;
  double now = loading_now();
  if (now - b->loading_tick < LOADING_REPAINT_MS || now - loading_repainted < LOADING_REPAINT_MS) return;
  if (!backend_loading_visible(b)) return;
  loading_repainted = now;
  b->iface->repaint(b);
}

void backend_loading_now(Backend* backend) {
  if (!backend) return;
  backend_set_loading(backend, 1, 0);
  backend->loading_shown = 1;
  if (!backend->iface || !backend->iface->repaint) return;
  loading_repainted = loading_now();
  backend->iface->repaint(backend);
}

void backend_loading_hold(Backend* backend) {
  if (backend && backend->loading == 2) backend->loading_calm = loading_now();
}

void backend_loading_tick(Backend* backend) {
  if (!backend || !backend->loading) return;
  double now = loading_now();
  if (now - backend->loading_tick > LOADING_SLOW_MS) backend->loading_calm = now;
  backend->loading_tick = now;
  if ((backend->loading == 2 && now - backend->loading_calm >= LOADING_CALM_MS) ||
      now - backend->loading_since > LOADING_MAX_MS) {
    backend->loading = 0;
    printf("[loading] off (%.1f s)\n", (now - backend->loading_since) / 1000.0);
  }
}
