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
#pragma once

// Lo que comparten las partes del backend sdl: SDLPlatformData (la ventana,
// el contexto, las capas y el thread que presenta) y sus tipos. Ver README.md.

#include "backend/sdl/mapper.h"
#include "backend/sdl/keypad.h"
#include <SDL2/SDL.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "core/keymap.h"
#include "core/trace.h"
#include "gfx/font.h"
#include "gfx/gl.h"
#include "gfx/gl_state.h"
#include "gfx/driver.h"
#include "backend/sdl/backend.h"
#include "backend/toast.h"
#include "platform/platform.h"
#include "shaders.h"   // runtime/shaders, generado por tools/embed_shaders.py

#define RESIZABLE_DELAY 30
// Ventana mas grande que la pantalla (sway: el juego de 816x624 en una
// pantalla de 640x480, hasta que el script del port la pone a pantalla
// completa): se muestra negro hasta que quepa, como mucho FIT_WAIT_MS.
#define FIT_WAIT_MS 3000

// Un texto ya dibujado en una textura (se rehace solo si cambia)
typedef struct {
  GLuint tex;
  char   text[128];
  int    w, h;
  int    px;       // tamano de letra con el que se hizo
} TextTex;

#define PRESENT_MAX_LAYERS 8
#define SHOWN_MAX 64

// Una imagen para el thread que presenta (o, sin el, para gpu_present)
typedef struct {
  PresentLayer layers[PRESENT_MAX_LAYERS];
  int          n, ww, wh;
  double       at;          // cuando mostrarla (CLOCK_MONOTONIC, ms; 0: ya)
  void        *fence;       // gl_fence de lo dibujado
  double       handed;      // cuando se entrego (para la traza "gpu frame")
  char         overlay[32];
  Toast        toast;
} PresentJob;

// Sin el thread: la imagen que el driver presenta (gfx/driver.h, gpu_present)
typedef struct {
  GpuFrame    base;         // primero: el driver solo ve esto
  PresentJob  job;
  Backend    *backend;
  struct SDLPlatformData *data;
} SdlFrame;

typedef struct SDLPlatformData {
  SDL_Window*         window;
  SDL_GLContext       gl;          // contexto para presentar (el de WebGL comparte texturas con el)
  GLuint              program;     // shader que copia una textura a la ventana
  GLint               u_tex;
  GLuint              screen_tex;  // frame de un canvas 2d (se sube cada frame)
  int                 screen_w;
  int                 screen_h;
  TextTex             overlay;     // backend->overlay (--fps)
  TextTex             loading;     // backend->loading: el aviso de carga
  struct {                         // backend->toast: la tarjeta de un logro
    GLuint   tex;
    unsigned serial;               // el aviso que tiene dibujado
    int      px, w, h;
    Uint32   age;                  // cuanto lleva en pantalla (ms, por imagenes dibujadas)
    Uint32   last;                 // SDL_GetTicks de la ultima imagen con el aviso
  } toast;
  GLint               u_alpha;
  GLint               u_size, u_prescale;   // present.frag: el canvas ampliado (sharp bilinear)
  float               blit_size[2], blit_prescale[2];   // los del siguiente sdl_blit (prescale 0: tal cual)
  float               blit_alpha;  // la opacidad del siguiente sdl_blit (1 casi siempre)
  int                 frames_until_resizable;
  Uint32              fit_until;   // SDL_GetTicks limite para esperar a que quepa (0: ya cabe)
  struct {                         // texturas de las capas 2d (una por canvas)
    const void *key;
    GLuint      tex;
    int         w, h;
    int         used;
  } layer_tex[8];
  SDL_GameController* pads[GAMEPAD_MAX];
  // botones pulsados desde la ultima lectura: una pulsacion mas corta que
  // una imagen tambien la ve el juego (RPG Maker mira el mando al actualizar)
  Uint8               latch[GAMEPAD_MAX][SDL_CONTROLLER_BUTTON_MAX];
  // Select y Start segun los eventos (no el estado de ahora): el atajo de
  // salir funciona aunque el juego vaya tan lento que se procesen tarde
  Uint8               held[GAMEPAD_MAX][SDL_CONTROLLER_BUTTON_MAX];
  Uint8               trigger_down[GAMEPAD_MAX][2];   // L2 y R2 pasados de la mitad
  // [controls.keys]: cuantos mandos tienen pulsada cada accion (la tecla
  // baja con el primero y sube con el ultimo)
  Uint8               pad_key_down[8];
  // lo que leyo getGamepads en esta imagen: se devuelve igual hasta que se
  // procesan los eventos de la siguiente (sdl_process_events) o cambian los
  // mandos. El juego lo lee varias veces por imagen (RPG Maker, ~4) y cada
  // lectura de SDL costaba ~95 us en la RX6S
  GamepadState        pad_cache[GAMEPAD_MAX];
  int                 pad_count;
  int                 pad_cache_valid;
  // el mando virtual de los tests (backend_test_pad)
  SDL_JoystickID      test_pad;
  int                 test_pad_on;
  int                 should_close;
  // el thread que presenta ([engine] present_ahead)
  SDL_Thread         *thr;
  SDL_mutex          *mx;
  SDL_cond           *cv;
  int                 thr_tried, thr_ok, thr_quit;
  // el compositor frena el thread (thread_throttled)
  double              thr_vper;     // ms entre refrescos de la pantalla
  int                 thr_slow;     // cambios de imagen seguidos que tardaron de mas
  int                 thr_throttled;
  PresentJob          job;          // la imagen entregada
  // sin thread: las imagenes para gpu_present (el driver puede guardar la
  // anterior: se alternan dos) y la marca de la GPU de la ultima que se
  // mostro (su textura vuelve al juego)
  SdlFrame            frames[2];
  int                 frame_i;
  void               *shown_fence;
  PresentJob          shown_job;    // sin thread: la ultima mostrada (sdl_repaint)
  int                 have_shown;
  int                 one;              // la ultima imagen, con el contexto del juego (present.c)
  int                 map_saved;   // Controls guardo una asignacion: se aplica en CAPTURE_STOP
  int                 job_pending;  // entregada y sin recoger
  int                 job_busy;     // el thread la esta mostrando
  unsigned            job_done;
  // cuando se mostraron las imagenes (para la consola de debug, pacing): las
  // apunta swap_window en shown. Sin thread, sdl_presented lee shown; con el
  // thread, shown es suyo y lo copia a pub bajo el cerrojo (mx), y
  // sdl_presented lee pub
  double              shown[SHOWN_MAX];
  int                 shown_n;
  double              pub[SHOWN_MAX];
  int                 pub_n;
} SDLPlatformData;
