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

// [engine] scaling y Options > Scaling
enum {
  SCALING_SHARP = 0,   // llena la pantalla, nitido (sharp bilinear: present.frag)
  SCALING_SMOOTH,      // llena la pantalla, suavizado (el filtro lineal solo)
  SCALING_PIXEL        // solo 1x, 2x... (pixel perfect), con bordes
};

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
  int       synthetic;  // 1 si viene de quix.input.pressKey (no del teclado real)
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
enum { WANT_STATE_SAVE = 1, WANT_STATE_LOAD, WANT_STATE_PREV, WANT_STATE_NEXT };

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
  int      locked;       // aun sin conseguir (el progreso, "3/12"): el
                         // distintivo hueco y el borde gris
  int      done, goal;   // goal > 0: una barra de progreso, done de goal
  int      star;         // una estrella dorada despues del titulo (un logro
                         // conseguido en hardcore)
  char     group[16];    // un grupo ("savestate"): uno nuevo sustituye al de su
                         // grupo que se ve o que espera, sin hacer cola
  int      replaces;     // (lo pone la cola) sustituye al que se veia: no
                         // vuelve a entrar desde la derecha
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
  // Un mando de prueba (tests): ver backend_test_pad. NULL si no hay
  int  (*test_pad)(Backend* backend, int button, int type);
  // Vibrar: ver backend_vibrate. NULL si no puede
  void (*vibrate)(Backend* backend, float strong, float weak, int ms);
  // Pantalla completa (Options del launcher, en el PC), sin reiniciar
  void (*set_fullscreen)(Backend* backend, int on);
  // El titulo de la ventana (backend->title), ya
  void (*set_title)(Backend* backend);
  // Los refrescos por segundo de la pantalla de la ventana (0 si no se sabe)
  double (*refresh_rate)(Backend* backend);
  // El tamano de la pantalla de la ventana, en pixeles (0 si bien; NULL o -1
  // si no se sabe: sin ventana)
  int  (*screen_size)(Backend* backend, int *w, int *h);
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
  int   no_vibration; // Options > Vibration: Off (backend_vibrate no hace nada)
  int   scaling;       // como se amplia el juego a la pantalla: SCALING_SHARP, _SMOOTH o _PIXEL
  int   back_to_menu;  // Select + R2: cerrar el juego y volver al launcher
  int   menu_back;     // Select + R2 permitido: el juego corre y se abrio desde el launcher
  int   want_screenshot; // Select + R1: el loop guarda una captura (y lo pone a 0)
  // los savestates: Select + X guarda, Select + Y carga, Select + izquierda o
  // derecha cambia de ranura; el loop lo hace entre dos frames
  // (core/main/state.c) y lo pone a 0
  int   want_state;      // WANT_STATE_SAVE, _LOAD, _PREV o _NEXT
  // La lupa, en todos los juegos: Select + L1 (o su boton de Controls, con
  // [controls] magnifier = true) abre un cuadro que se mueve con la cruceta
  // o las palancas y muestra ampliado lo que tiene debajo; R1 acerca, L1
  // aleja, A la vuelve al centro, Y la agranda, X la achica, B (o el combo)
  // la cierra; arriba a la izquierda, su icono. El juego no recibe el mando
  // mientras, salvo con la lupa fija (Start con ella abierta): se queda
  // quieta y el mando es del juego; el combo la desfija
  float magnify;       // 0: sin lupa; si no, cuanto amplia (1.25 a 6)
  float mag_size;      // el tamano del cuadro: la parte del juego (1/3 a 1; 2, toda la pantalla)
  float mag_x, mag_y;  // el centro del cuadro (0 a 1 del juego)
  int   mag_pinned;    // la lupa fija: el mando es del juego
  char  overlay[32];  // texto encima de la ventana, arriba a la derecha (--fps)
  // controller.map: las asignaciones del mando hechas en el launcher (lineas
  // de SDL_GAMECONTROLLERCONFIG, como las de PortMaster). Se fija antes de
  // backend_init, que las carga
  char  controller_map[1024];
  // [controls.keys]: acciones con nombre que pulsan una tecla, para lo que
  // el juego solo escucha en el teclado (la antorcha de Fear & Hunger, la T).
  // El boton de cada una se asigna en Controls y se guarda en
  // controller.keys, junto a controller.map. code es KeyboardEvent.code. Se
  // fija antes de backend_init
  int   pad_keys_count;
  struct { char name[48]; char code[24]; } pad_keys[8];
  char  title[256];   // el titulo de la ventana: "quixjs | <juego>" (backend_set_title)
  Toast toast;        // aviso de un logro (el backend lo anima y lo quita solo)
  // Una imagen de adelanto ([engine] present_ahead): las muestra otro thread y
  // el juego sigue con la siguiente sin esperar al refresco.
  int    present_ahead;    // permitido (se fija antes de backend_init)
  int    fps;              // [engine] fps, el tope de imagenes por segundo (0: 60)
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
//   CAPTURE_START  abre el primer mando sin mapeo (lo crudo); sin mando, se
//                  asigna el teclado
//   CAPTURE_POLL   1 cuando se ha pulsado y soltado una entrada nueva: out es
//                  "b3" (boton), "h0.1" (cruceta, con su direccion), "-a1"
//                  (eje, con su direccion) o "k:Return" (una tecla: la
//                  primera entrada decide si se asigna el mando o el
//                  teclado); -1 si se cancela (Escape, se desconecta el
//                  mando); 0 mientras espera
//   CAPTURE_STOP   lo cierra
// Con lo pedido, backend_save_controller guarda la linea del mando en
// controller_map ("dpup:h0.1,a:b0,-leftx:-a0,...": se sustituye la de ese
// mando) y la aplica ya, sin reiniciar; si se asigno el teclado, en
// keyboard.map, junto a controller_map (runtime/backend/sdl/keypad.c). 0 si va bien.
enum { CAPTURE_START, CAPTURE_POLL, CAPTURE_STOP };
int  backend_capture_input(Backend* backend, int step, char *out, size_t room);
int  backend_save_controller(Backend* backend, const char *pairs);
// Un mando de prueba para los tests (quix.input.pressPad): la primera vez conecta
// un mando virtual (SDL_JoystickAttachVirtual) y le pulsa (TEST_PAD_DOWN) o
// suelta (TEST_PAD_UP) el boton (indice del mapeo "standard"); con
// TEST_PAD_PRESS lo pulsa y lo suelta antes de la siguiente imagen (sus dos
// eventos llegan juntos). Devuelve la posicion del mando (navigator.getGamepads)
// o -1 si el backend no puede (dummy, SDL de antes de 2.0.14).
enum { TEST_PAD_UP, TEST_PAD_DOWN, TEST_PAD_PRESS };
int  backend_test_pad(Backend* backend, int button, int type);
// Vibra ms milisegundos (0 para) con la fuerza de cada motor, de 0 a 1:
// strong el grande (golpe, grave), weak el pequeno (zumbido, agudo). Los
// mandos que puedan y, si ninguno, el aparato (platform_vibrate, el telefono
// en Android, con la mayor de las dos). Sin vibrador, o con no_vibration,
// no hace nada. Lo usan
// navigator.vibrate y gamepad.vibrationActuator (runtime/binding/navigator/gamepads.js)
void backend_vibrate(Backend* backend, float strong, float weak, int ms);
// La ventana a pantalla completa o no, ya (backend->fullscreen lo dice)
void backend_set_fullscreen(Backend* backend, int on);
// El titulo de la ventana: "quixjs | <juego>", o "quixjs" sin titulo. Antes de
// backend_init vale para crear la ventana; despues la cambia ya
void backend_set_title(Backend* backend, const char *game_title);
// Los refrescos por segundo de la pantalla donde esta la ventana (0 si no se sabe)
double backend_refresh_rate(Backend* backend);
int    backend_screen_size(Backend* backend, int *w, int *h);   // -1 si no se sabe
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
// El aviso ya, sin esperar (al pulsar Play en el launcher: el juego tarda en
// arrancar y aun no ha dibujado nada); se quita como los demas
void   backend_loading_now(Backend* backend);
// Una vez por imagen presentada: quita el aviso cuando toca
void   backend_loading_tick(Backend* backend);
// el juego aun carga (archivos, imagenes, sonidos): el aviso que se esta
// yendo espera
void   backend_loading_hold(Backend* backend);

// Cola de entrada (la llenan los backends y quix.input.pressKey en los tests)
void backend_push_input(Backend* backend, const InputEvent *ev);
int  backend_pop_input(Backend* backend, InputEvent *ev);
