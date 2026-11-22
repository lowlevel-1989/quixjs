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

// El teclado y los mandos: las teclas de SDL como KeyboardEvent.code, la
// asignacion de un mando (Controls del launcher), pantalla completa, titulo,
// refresco de la pantalla y la cola de eventos (sdl_process_events).
// Parte de backend/sdl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include <stdlib.h>
#include "common.h"

typedef struct {
  SDL_Scancode sc;
  const char  *code;
} ScancodeMap;

static const ScancodeMap scancode_map[] = {
  { SDL_SCANCODE_RETURN,    "Enter" },      { SDL_SCANCODE_KP_ENTER,  "NumpadEnter" },
  { SDL_SCANCODE_ESCAPE,    "Escape" },     { SDL_SCANCODE_SPACE,     "Space" },
  { SDL_SCANCODE_TAB,       "Tab" },        { SDL_SCANCODE_BACKSPACE, "Backspace" },
  { SDL_SCANCODE_LSHIFT,    "ShiftLeft" },  { SDL_SCANCODE_RSHIFT,    "ShiftRight" },
  { SDL_SCANCODE_LCTRL,     "ControlLeft" },{ SDL_SCANCODE_RCTRL,     "ControlRight" },
  { SDL_SCANCODE_LALT,      "AltLeft" },    { SDL_SCANCODE_RALT,      "AltRight" },
  { SDL_SCANCODE_LEFT,      "ArrowLeft" },  { SDL_SCANCODE_RIGHT,     "ArrowRight" },
  { SDL_SCANCODE_UP,        "ArrowUp" },    { SDL_SCANCODE_DOWN,      "ArrowDown" },
  { SDL_SCANCODE_PAGEUP,    "PageUp" },     { SDL_SCANCODE_PAGEDOWN,  "PageDown" },
  { SDL_SCANCODE_HOME,      "Home" },       { SDL_SCANCODE_END,       "End" },
  { SDL_SCANCODE_INSERT,    "Insert" },     { SDL_SCANCODE_DELETE,    "Delete" },
  { SDL_SCANCODE_PAUSE,     "Pause" },      { SDL_SCANCODE_CAPSLOCK,  "CapsLock" },
  { SDL_SCANCODE_MINUS,     "Minus" },      { SDL_SCANCODE_EQUALS,    "Equal" },
  { SDL_SCANCODE_LEFTBRACKET, "BracketLeft" }, { SDL_SCANCODE_RIGHTBRACKET, "BracketRight" },
  { SDL_SCANCODE_BACKSLASH, "Backslash" },  { SDL_SCANCODE_SEMICOLON, "Semicolon" },
  { SDL_SCANCODE_APOSTROPHE, "Quote" },     { SDL_SCANCODE_GRAVE,     "Backquote" },
  { SDL_SCANCODE_COMMA,     "Comma" },      { SDL_SCANCODE_PERIOD,    "Period" },
  { SDL_SCANCODE_SLASH,     "Slash" },
  { SDL_SCANCODE_UNKNOWN,   NULL }
};

static const char *sdl_scancode_to_code(SDL_Scancode sc, char *buf, size_t size) {
  if (sc >= SDL_SCANCODE_A && sc <= SDL_SCANCODE_Z) {
    snprintf(buf, size, "Key%c", 'A' + (sc - SDL_SCANCODE_A));
    return buf;
  }
  if (sc >= SDL_SCANCODE_1 && sc <= SDL_SCANCODE_0) {
    snprintf(buf, size, "Digit%d", (sc - SDL_SCANCODE_1 + 1) % 10);
    return buf;
  }
  if (sc >= SDL_SCANCODE_F1 && sc <= SDL_SCANCODE_F12) {
    snprintf(buf, size, "F%d", sc - SDL_SCANCODE_F1 + 1);
    return buf;
  }
  if (sc >= SDL_SCANCODE_KP_1 && sc <= SDL_SCANCODE_KP_0) {
    snprintf(buf, size, "Numpad%d", (sc - SDL_SCANCODE_KP_1 + 1) % 10);
    return buf;
  }
  for (const ScancodeMap *m = scancode_map; m->code; m++) {
    if (m->sc == sc) return m->code;
  }
  return NULL;
}

// Al terminar de asignar (CAPTURE_STOP), si se guardo algo, los mandos de
// SDL se reinician con la asignacion nueva (mapper_reload) y se vuelven a
// abrir. Se hace aqui y no al guardar: antes se cierra el mando crudo que
// usa mapper_capture.
static int sdl_capture_input(Backend* backend, int step, char *out, size_t room) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  int r = mapper_capture(step, out, room);
  data->pad_cache_valid = 0;
  if (step == CAPTURE_STOP && data->map_saved) {
    data->map_saved = 0;
    for (int i = 0; i < GAMEPAD_MAX; i++) {
      if (data->pads[i]) SDL_GameControllerClose(data->pads[i]);
      data->pads[i] = NULL;
    }
    mapper_reload(backend->controller_map);
    sdl_open_gamepads(data);
  }
  return r;
}

// Guarda la asignacion en controller.map (se aplica en CAPTURE_STOP)
static int sdl_save_controller(Backend* backend, const char *pairs) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (mapper_save(backend->controller_map, pairs) != 0) return -1;
  data->map_saved = 1;
  return 0;
}

static void sdl_set_fullscreen(Backend* backend, int on) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (!data->window) return;
  if (SDL_SetWindowFullscreen(data->window, on ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) == 0) backend->fullscreen = on != 0;
}

static void sdl_set_title(Backend* backend) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (data && data->window) SDL_SetWindowTitle(data->window, backend->title);
}

static double sdl_refresh_rate(Backend* backend) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  SDL_DisplayMode mode;
  if (!data->window || SDL_GetWindowDisplayMode(data->window, &mode) != 0) return 0;
  return mode.refresh_rate;
}

static int sdl_screen_size(Backend* backend, int *w, int *h) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (!data || !data->window) return -1;
  int display = SDL_GetWindowDisplayIndex(data->window);
  SDL_Rect r;
  if (display < 0 || SDL_GetDisplayBounds(display, &r) != 0 || r.w <= 0 || r.h <= 0) return -1;
  *w = r.w;
  *h = r.h;
  return 0;
}

// [controls.keys]: la accion name pulsa o suelta su tecla, como si viniera
// del teclado (con varios mandos, sube con el ultimo que la suelta)
static void pad_key(Backend* backend, SDLPlatformData* data, const char *name, int down) {
  for (int k = 0; k < backend->pad_keys_count && k < 8; k++) {
    if (strcmp(backend->pad_keys[k].name, name) != 0) continue;
    if (down) {
      if (data->pad_key_down[k]++ != 0) continue;
    } else {
      if (data->pad_key_down[k] == 0 || --data->pad_key_down[k] != 0) continue;
    }
    // las de quixjs ([controls] screenshot y magnifier): su boton suelto,
    // ademas del combo
    const char *code = backend->pad_keys[k].code;
    if (strcmp(code, "quix:screenshot") == 0) {
      if (down) backend->want_screenshot = 1;
      continue;
    }
    if (strcmp(code, "quix:magnifier") == 0) {
      if (down) magnifier_toggle(backend);
      continue;
    }
    InputEvent ev;
    if (keymap_make_event(&ev, down ? INPUT_KEY_DOWN : INPUT_KEY_UP, backend->pad_keys[k].code, 0, 0) == 0) {
      backend_push_input(backend, &ev);
    }
  }
}

static void pad_key_cb(void *ctx, const char *name, int down) {
  Backend *backend = ctx;
  pad_key(backend, (SDLPlatformData*)backend->platform_data, name, down);
}





// Select + R2: cerrar el juego y volver al launcher (main.c lo vuelve a
// abrir). Select + Start no sirve: en PortMaster gptokeyb lo usa para matar
// el juego (la salida de emergencia). Solo en el juego (menu_back)
static void back_to_menu(Backend* backend, SDLPlatformData* data) {
  if (!backend->menu_back) return;
  printf("Select + R2: back to the menu\n");
  data->should_close = 1;
  backend->back_to_menu = 1;
}

static void sdl_process_events(Backend* backend) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  SDL_Event event;
  data->pad_cache_valid = 0;   // getGamepads vuelve a leer los mandos (gamepads.c)
  while (SDL_PollEvent(&event)) {
    mapper_event(&event);   // solo hace algo mientras se asigna el mando
    keypad_event(&event);   // el teclado como mando (keyboard.map)
    if (backend->pad_keys_count) mapper_key_event(&event, pad_key_cb, backend);
    switch (event.type) {
      case SDL_QUIT:
        data->should_close = 1;
        break;

      case SDL_KEYDOWN:
      case SDL_KEYUP: {
        char buf[24];
        const char *code = sdl_scancode_to_code(event.key.keysym.scancode, buf, sizeof(buf));
        if (!code || keypad_has(event.key.keysym.sym)) break;   // del mando del teclado (keypad.c)
        InputEvent ev;
        int shift = (event.key.keysym.mod & KMOD_SHIFT) != 0;
        InputType type = event.type == SDL_KEYDOWN ? INPUT_KEY_DOWN : INPUT_KEY_UP;
        if (keymap_make_event(&ev, type, code, shift, event.key.repeat) == 0) {
          backend_push_input(backend, &ev);
        }
        break;
      }

      // Select + Start a la vez: salir (en la portatil no hay ventana que cerrar)
      case SDL_CONTROLLERBUTTONDOWN: {
        SDL_GameController *gc = SDL_GameControllerFromInstanceID(event.cbutton.which);
        int quit = 0, b = event.cbutton.button;
        for (int i = 0; i < GAMEPAD_MAX; i++) {
          if (gc && data->pads[i] == gc && b < SDL_CONTROLLER_BUTTON_MAX) {
            Uint8 *held = data->held[i];
            data->latch[i][b] = 1;
            held[b] = 1;
            quit = held[SDL_CONTROLLER_BUTTON_BACK] && held[SDL_CONTROLLER_BUTTON_START];
            // Select + R1: una captura de pantalla
            if (held[SDL_CONTROLLER_BUTTON_BACK] && held[SDL_CONTROLLER_BUTTON_RIGHTSHOULDER] &&
                (b == SDL_CONTROLLER_BUTTON_BACK || b == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER))
              backend->want_screenshot = 1;
            // Select + L1: abrir o cerrar la lupa; abierta, sus botones
            if (held[SDL_CONTROLLER_BUTTON_BACK] && held[SDL_CONTROLLER_BUTTON_LEFTSHOULDER] &&
                (b == SDL_CONTROLLER_BUTTON_BACK || b == SDL_CONTROLLER_BUTTON_LEFTSHOULDER))
              magnifier_toggle(backend);
            else if (backend->magnify > 1)
              magnifier_button(backend, b);
          }
        }
        if (quit) {
          printf("Select + Start: exit\n");
          data->should_close = 1;
        }
        // Select + R2 (en orden cualquiera): volver al launcher
        if (b == SDL_CONTROLLER_BUTTON_BACK && gc &&
            SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 16000)
          back_to_menu(backend, data);
        break;
      }

      case SDL_WINDOWEVENT:
        // Redimensionable solo cuando ya se ve: los gestores en mosaico dejan
        // flotando y con su tamano las ventanas que aparecen con tamano fijo,
        // y siguen asi aunque luego se puedan redimensionar. Justo tras el
        // primer frame es pronto: se espera a que este en pantalla (exposed)
        // y RESIZABLE_DELAY frames mas.
        if (event.window.event == SDL_WINDOWEVENT_EXPOSED && data->frames_until_resizable == 0) {
          data->frames_until_resizable = RESIZABLE_DELAY;
        }
        // a pantalla completa o no, la haya puesto quixjs o el sistema (en
        // ROCKNIX, el script de PortMaster): Select + R2 la recrea igual
        backend->fullscreen = (SDL_GetWindowFlags(data->window) & SDL_WINDOW_FULLSCREEN) != 0;
        break;

      case SDL_CONTROLLERBUTTONUP: {
        SDL_GameController *gc = SDL_GameControllerFromInstanceID(event.cbutton.which);
        for (int i = 0; i < GAMEPAD_MAX; i++) {
          if (gc && data->pads[i] == gc && event.cbutton.button < SDL_CONTROLLER_BUTTON_MAX) {
            data->held[i][event.cbutton.button] = 0;
          }
        }
        break;
      }

      // L2 y R2 son ejes: cuentan al pasar de la mitad (Select + R2, la lupa)
      case SDL_CONTROLLERAXISMOTION: {
        int t = event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT ? 0
              : event.caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT ? 1 : -1;
        if (t < 0) break;
        SDL_GameController *gc = SDL_GameControllerFromInstanceID(event.caxis.which);
        for (int i = 0; i < GAMEPAD_MAX; i++) {
          if (!gc || data->pads[i] != gc) continue;
          int down = event.caxis.value > 16000;
          if (down && !data->trigger_down[i][t]) {
            if (t == 1 && data->held[i][SDL_CONTROLLER_BUTTON_BACK]) back_to_menu(backend, data);
            else if (backend->magnify > 1) magnifier_zoom(backend, t == 1 ? 1 : -1);   // la lupa: R2 acerca, L2 aleja
          }
          data->trigger_down[i][t] = down;
        }
        break;
      }

      case SDL_CONTROLLERDEVICEADDED:
        sdl_open_gamepads(data);
        break;

      case SDL_CONTROLLERDEVICEREMOVED:
        for (int i = 0; i < GAMEPAD_MAX; i++) {
          if (data->pads[i] && !SDL_GameControllerGetAttached(data->pads[i])) {
            SDL_GameControllerClose(data->pads[i]);
            data->pads[i] = NULL;
          }
        }
        break;
    }
  }
  magnifier_pan(backend, data);
}
