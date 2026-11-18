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

// El backend: la ventana, mostrar cada imagen, la entrada (teclado y mandos)
// y los avisos en pantalla. Hay dos: sdl (ventana con OpenGL ES 2) y dummy
// (sin ventana, para los tests y las capturas). El resto de quixjs solo usa
// las funciones de este archivo.

#include <stddef.h>
#include <stdint.h>

typedef struct Backend Backend;

// Eventos de entrada que el backend deja en la cola y el runtime pasa a JS
// como KeyboardEvent (keydown / keyup en document y window).
typedef enum {
  INPUT_KEY_DOWN = 1,
  INPUT_KEY_UP
} InputType;

typedef struct {
  InputType type;
  int       key_code;   // KeyboardEvent.keyCode
  char      key[24];    // KeyboardEvent.key  ("a", "Enter", "ArrowLeft"...)
  char      code[24];   // KeyboardEvent.code ("KeyA", "Enter", "ArrowLeft"...)
  int       repeat;
  int       synthetic;  // 1 si viene de quix.pressKey (no del teclado real)
} InputEvent;

#define INPUT_QUEUE_SIZE 128

// Gamepad con el mapeo "standard" del navegador
#define GAMEPAD_MAX         4
#define GAMEPAD_BUTTONS     17
#define GAMEPAD_AXES        4

typedef struct {
  int   connected;
  char  id[64];
  float axes[GAMEPAD_AXES];
  int   pressed[GAMEPAD_BUTTONS];
  float value[GAMEPAD_BUTTONS];
} GamepadState;

// Una capa de la pantalla (un canvas del documento, de abajo a arriba)
typedef struct {
  const void    *key;          // identifica la capa entre frames (su canvas)
  unsigned       gl_tex;       // canvas WebGL: su textura (0 si es 2d)
  const uint8_t *rgba;         // canvas 2d: sus pixeles
  int            w, h;
  int            premultiplied;
  int            dirty;        // canvas 2d: cambio desde el frame anterior
} PresentLayer;

// Un aviso como los de RetroAchievements: una tarjeta arriba a la derecha con
// un distintivo de color y su inicial, la cabecera, el titulo y el texto.
// Quien lo pone cambia serial; el backend la muestra TOAST_MS y la quita.
#define TOAST_MS 5000

typedef struct {
  unsigned serial;       // otro valor: otro aviso (0: ninguno todavia)
  char     header[64];   // "Achievement unlocked  3/17"
  char     title[96];
  char     text[160];    // la descripcion; puede ir vacia
  char     badge[8];     // lo que va dentro del distintivo (la inicial)
  unsigned color;        // 0xRRGGBB del distintivo y del borde
} Toast;

typedef struct {
  void (*init)(Backend* backend, int width, int height);
  void (*process_events)(Backend* backend);
  // Muestra un frame RGBA (orden r, g, b, a) de w x h
  void (*present)(Backend* backend, const uint8_t *rgba, int w, int h);
  // Muestra una textura GL (canvas WebGL) sin leerla de la GPU (NULL si no se puede)
  void (*present_gl)(Backend* backend, unsigned tex, int w, int h, int premultiplied);
  // Varias capas compuestas en la GPU (NULL si no se puede)
  void (*present_layers)(Backend* backend, const PresentLayer *layers, int n);
  void (*cleanup)(Backend* backend);
  int  (*should_close)(Backend* backend);
  int  (*get_gamepads)(Backend* backend, GamepadState *pads, int max);
  // Asignar el mando (Controls del launcher): ver backend_capture_input
  int  (*capture_input)(Backend* backend, int step, char *out, size_t room);
  int  (*save_controller)(Backend* backend, const char *pairs);
  // Pantalla completa (Options del launcher, en el PC), sin reiniciar
  void (*set_fullscreen)(Backend* backend, int on);
  // El titulo de la ventana (backend->title), ya
  void (*set_title)(Backend* backend);
  // Los refrescos por segundo de la pantalla de la ventana (0 si no se sabe)
  double (*refresh_rate)(Backend* backend);
  // Con el thread que presenta: espera a que se haya mostrado la imagen
  // entregada (NULL si no hay thread)
  void (*present_wait)(Backend* backend);
  // Cuando se mostraron de verdad las imagenes desde la ultima llamada
  // (CLOCK_MONOTONIC, ms); NULL si el backend no lo sabe
  int  (*presented)(Backend* backend, double *ms, int max);
  // La ultima imagen mostrada otra vez, con lo de encima (el aviso de carga),
  // desde el thread del juego a mitad de un trabajo largo; NULL si no sabe
  void (*repaint)(Backend* backend);
  const char *name;
} BackendInterface;

struct Backend {
  const BackendInterface* iface;
  void* platform_data;
  int   fullscreen;   // se fija antes de backend_init
  int   resizable;    // ventana redimensionable (en un tiling WM no flota)
  int   no_vsync;     // presentar sin esperar al refresco (se fija antes de backend_init)
  char  overlay[32];  // texto encima de la ventana, arriba a la derecha (--fps)
  // controller.map: las asignaciones del mando hechas en el launcher (lineas
  // de SDL_GAMECONTROLLERCONFIG, como las de PortMaster). Se fija antes de
  // backend_init, que las carga
  char  controller_map[1024];
  char  title[256];   // el titulo de la ventana: "quixjs | <juego>" (backend_set_title)
  Toast toast;        // aviso de un logro (el backend lo anima y lo quita solo)
  // Una imagen de adelanto ([engine] present_ahead): las muestra otro thread y
  // el juego sigue con la siguiente sin esperar al refresco.
  int    present_ahead;    // permitido (se fija antes de backend_init)
  int    present_thread;   // el backend lo pone a 1 cuando su thread va
  int    present_async;    // la siguiente imagen se entrega sin esperar a que se vea
  double present_at;       // y se muestra en ese momento (CLOCK_MONOTONIC, ms; 0: ya)
  // El aviso de carga ("Loading...", abajo a la derecha) mientras el juego
  // carga una partida (quix.loading). 1: cargando; 2: la carga termino y el
  // aviso sigue hasta que las imagenes vayan normales (backend_loading_tick);
  // 0: sin aviso. Lo lee tambien el thread que presenta.
  volatile int loading;
  double loading_since;    // cuando empezo (CLOCK_MONOTONIC, ms)
  double loading_calm;     // desde cuando van normales las imagenes (con 2)
  double loading_tick;     // la ultima imagen (backend_loading_tick)
  double loading_after;    // con retardo: se ve si el juego lleva estos ms sin
                           // mostrar una imagen (0: enseguida)
  volatile int loading_shown;   // ya se vio (sigue hasta que acabe la carga)

  InputEvent input[INPUT_QUEUE_SIZE];
  int        input_head;
  int        input_len;
};

Backend* backend_create(int type);
void backend_init(Backend* backend, int width, int height);
void backend_process_events(Backend* backend);
void backend_present(Backend* backend, const uint8_t *rgba, int w, int h);
int  backend_can_present_gl(Backend* backend);
void backend_present_gl(Backend* backend, unsigned tex, int w, int h, int premultiplied);
int  backend_can_present_layers(Backend* backend);
void backend_present_layers(Backend* backend, const PresentLayer *layers, int n);
void backend_cleanup(Backend* backend);
int  backend_should_close(Backend* backend);
int  backend_get_gamepads(Backend* backend, GamepadState *pads, int max);
const char *backend_name(Backend* backend);

// Asignar el mando, entrada a entrada (Controls del launcher):
//   CAPTURE_START  abre el primer mando sin mapeo (lo crudo); -1 si no hay
//   CAPTURE_POLL   1 cuando se ha pulsado y soltado una entrada nueva: out es
//                  "b3" (boton), "h0.1" (cruceta, con su direccion) o "-a1"
//                  (eje, con su direccion); -1 si se cancela (Escape, se
//                  desconecta el mando); 0 mientras espera
//   CAPTURE_STOP   lo cierra
// Con lo pedido, backend_save_controller guarda la linea del mando en
// controller_map ("dpup:h0.1,a:b0,-leftx:-a0,...": se sustituye la de ese
// mando) y la aplica ya, sin reiniciar. 0 si va bien.
enum { CAPTURE_START, CAPTURE_POLL, CAPTURE_STOP };
int  backend_capture_input(Backend* backend, int step, char *out, size_t room);
int  backend_save_controller(Backend* backend, const char *pairs);
// La ventana a pantalla completa o no, ya (backend->fullscreen lo dice)
void backend_set_fullscreen(Backend* backend, int on);
// El titulo de la ventana: "quixjs | <juego>", o "quixjs" sin titulo. Antes de
// backend_init vale para crear la ventana; despues la cambia ya
void backend_set_title(Backend* backend, const char *game_title);
// Los refrescos por segundo de la pantalla donde esta la ventana (0 si no se sabe)
double backend_refresh_rate(Backend* backend);
void   backend_present_wait(Backend* backend);
int    backend_presented(Backend* backend, double *ms, int max);   // -1 si no lo sabe
// El aviso de carga: on 1 al empezar a cargar (con after_ms, se ve solo si el
// juego se para mas de after_ms sin mostrar una imagen; 0: enseguida), 0 al
// terminar (backend->loading)
void   backend_set_loading(Backend* backend, int on, double after_ms);
// Si el aviso se ve ya (cargando y, con retardo, el juego parado ese rato)
int    backend_loading_visible(Backend* backend);
// Desde un trabajo largo del juego (subir texturas, compilar shaders...): sin
// el thread que presenta, nadie repinta mientras el juego esta parado; si
// el aviso se ve y hace rato que no se muestra nada, la ultima imagen otra
// vez con el aviso (iface->repaint). Casi gratis si no hay aviso.
void   backend_loading_poke(void);
// Una vez por imagen presentada: quita el aviso cuando toca
void   backend_loading_tick(Backend* backend);

// Cola de entrada (la llenan los backends y quix.pressKey en los tests)
void backend_push_input(Backend* backend, const InputEvent *ev);
int  backend_pop_input(Backend* backend, InputEvent *ev);
