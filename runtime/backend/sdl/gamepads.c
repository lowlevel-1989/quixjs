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

// getGamepads: el estado de los mandos en el mapeo "standard". Y vibrar.
// Parte de backend/sdl.c, que une las partes en orden y se compila como
// un solo archivo (ver README.md): usa lo de las partes anteriores.

#include "common.h"
#include "platform/platform.h"

// Orden de botones del mapeo "standard" de la Gamepad API
static const SDL_GameControllerButton standard_buttons[GAMEPAD_BUTTONS] = {
  SDL_CONTROLLER_BUTTON_A,             // 0
  SDL_CONTROLLER_BUTTON_B,             // 1
  SDL_CONTROLLER_BUTTON_X,             // 2
  SDL_CONTROLLER_BUTTON_Y,             // 3
  SDL_CONTROLLER_BUTTON_LEFTSHOULDER,  // 4
  SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, // 5
  SDL_CONTROLLER_BUTTON_INVALID,       // 6 gatillo izquierdo (eje)
  SDL_CONTROLLER_BUTTON_INVALID,       // 7 gatillo derecho (eje)
  SDL_CONTROLLER_BUTTON_BACK,          // 8
  SDL_CONTROLLER_BUTTON_START,         // 9
  SDL_CONTROLLER_BUTTON_LEFTSTICK,     // 10
  SDL_CONTROLLER_BUTTON_RIGHTSTICK,    // 11
  SDL_CONTROLLER_BUTTON_DPAD_UP,       // 12
  SDL_CONTROLLER_BUTTON_DPAD_DOWN,     // 13
  SDL_CONTROLLER_BUTTON_DPAD_LEFT,     // 14
  SDL_CONTROLLER_BUTTON_DPAD_RIGHT,    // 15
  SDL_CONTROLLER_BUTTON_GUIDE,         // 16
};

static float axis_value(SDL_GameController *gc, SDL_GameControllerAxis axis) {
  return SDL_GameControllerGetAxis(gc, axis) / 32767.0f;
}

// Lee los mandos de SDL y lo guarda en pad_cache. El estado de SDL solo
// cambia al procesar los eventos (SDL_PollEvent): se lee una vez por imagen.
// latch (un boton pulsado y soltado antes de leerlo) se ve en todas las
// lecturas de esa imagen y se borra para la siguiente.
static void sdl_read_gamepads(SDLPlatformData* data) {
  int count = 0;
  int select_held[GAMEPAD_MAX] = {0};   // Select pulsado ahora (sin latch): los combos
  for (int i = 0; i < GAMEPAD_MAX; i++) {
    GamepadState *p = &data->pad_cache[i];
    memset(p, 0, sizeof(*p));
    SDL_GameController *gc = data->pads[i];
    if (!gc) continue;
    p->connected = 1;
    select_held[i] = SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_BACK);
    snprintf(p->id, sizeof(p->id), "%s", SDL_GameControllerName(gc) ? SDL_GameControllerName(gc) : "SDL gamepad");
    p->axes[0] = axis_value(gc, SDL_CONTROLLER_AXIS_LEFTX);
    p->axes[1] = axis_value(gc, SDL_CONTROLLER_AXIS_LEFTY);
    p->axes[2] = axis_value(gc, SDL_CONTROLLER_AXIS_RIGHTX);
    p->axes[3] = axis_value(gc, SDL_CONTROLLER_AXIS_RIGHTY);
    for (int b = 0; b < GAMEPAD_BUTTONS; b++) {
      if (standard_buttons[b] == SDL_CONTROLLER_BUTTON_INVALID) continue;
      p->pressed[b] = SDL_GameControllerGetButton(gc, standard_buttons[b]) || data->latch[i][standard_buttons[b]];
      data->latch[i][standard_buttons[b]] = 0;
      p->value[b] = p->pressed[b] ? 1.0f : 0.0f;
    }
    p->value[6] = axis_value(gc, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    p->value[7] = axis_value(gc, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
    p->pressed[6] = p->value[6] > 0.5f;
    p->pressed[7] = p->value[7] > 0.5f;
    count = i + 1;
  }
  // los combos de quixjs (Select + otro boton): con Select pulsado, el
  // juego no ve ningun otro boton, ni despues, hasta que se suelte (en
  // Firework, X abre el menu y Select + X lo abria al guardar). Select solo,
  // como siempre
  static unsigned char combo_mask[GAMEPAD_MAX][GAMEPAD_BUTTONS];
  for (int i = 0; i < GAMEPAD_MAX; i++) {
    GamepadState *p = &data->pad_cache[i];
    for (int b = 0; b < GAMEPAD_BUTTONS; b++) {
      if (b == 8) continue;   // Select
      if (select_held[i] && p->pressed[b]) combo_mask[i][b] = 1;
      if (!combo_mask[i][b]) continue;
      if (!p->pressed[b]) combo_mask[i][b] = 0;
      p->pressed[b] = 0;
      p->value[b] = 0;
    }
  }
  // el teclado (keyboard.map): por el mismo canal, el primer mando
  if (keypad_active()) {
    keypad_merge(&data->pad_cache[0]);
    if (count == 0) count = 1;
  }
  data->pad_count = count;
  data->pad_cache_valid = 1;
}

static int sdl_get_gamepads(Backend* backend, GamepadState *pads, int max) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (!data->pad_cache_valid) sdl_read_gamepads(data);
  int n = max < GAMEPAD_MAX ? max : GAMEPAD_MAX;
  if (n > 0) memcpy(pads, data->pad_cache, sizeof(GamepadState) * n);
  magnifier_hide_pads(backend, pads, n);
  return data->pad_count < n ? data->pad_count : n;
}

// El vibrador que no es de un mando ni de la plataforma (SDL_Haptic: en
// Android, el VIBRATOR_SERVICE del telefono). Se abre la primera vez; NULL si
// no hay
static SDL_Haptic *device_haptic(void) {
  static SDL_Haptic *haptic;
  static int tried;
  if (tried) return haptic;
  tried = 1;
  if (SDL_InitSubSystem(SDL_INIT_HAPTIC) < 0) return NULL;
  for (int i = 0; i < SDL_NumHaptics() && !haptic; i++) {
    SDL_Haptic *candidate = SDL_HapticOpen(i);
    if (!candidate) continue;
    if (SDL_HapticRumbleInit(candidate) == 0) {
      haptic = candidate;
      printf("[vibrate] vibrator: %s\n", SDL_HapticName(i));
    } else {
      SDL_HapticClose(candidate);
    }
  }
  return haptic;
}

// Vibran los mandos abiertos que puedan, cada motor con su fuerza; si
// ninguno, el aparato, con la mayor: el de la plataforma (platform_vibrate)
// o, si no, el de SDL. Adaptado de tamatama (src/backend/sdl/backend.c, MIT;
// alli siempre a toda fuerza):
// https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/src/backend/sdl/backend.c#L519-L556
// En el log, la primera vez: que vibro (o que nada puede)
static Uint16 motor(float v) {
  return v <= 0 ? 0 : v >= 1 ? 0xffff : (Uint16)(v * 0xffff);
}

static void sdl_vibrate(Backend* backend, float strong, float weak, int ms) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  static int said;
  if (ms < 0) ms = 0;
  float most = strong > weak ? strong : weak;
  if (most <= 0) ms = 0;   // sin fuerza: parar
  if (most > 1) most = 1;
  int done = 0;
  for (int i = 0; i < GAMEPAD_MAX; i++) {
    if (data->pads[i] && SDL_GameControllerRumble(data->pads[i], motor(strong), motor(weak), (Uint32)ms) == 0) {
      if (!done && !said) printf("[vibrate] the controller: %s\n", SDL_GameControllerName(data->pads[i]));
      done = 1;
    }
  }
  if (!done && platform_vibrate(ms, (int)(most * 255 + 0.5f))) {
    if (!said) printf("[vibrate] the device\n");
    done = 1;
  }
  SDL_Haptic *haptic = done ? NULL : device_haptic();
  if (haptic) {
    if (ms == 0) SDL_HapticRumbleStop(haptic);
    else if (SDL_HapticRumblePlay(haptic, most, (Uint32)ms) == 0) done = 1;
    else fprintf(stderr, "[vibrate] %s\n", SDL_GetError());
  }
  if (!done && !said) printf("[vibrate] nothing can vibrate\n");
  said = 1;
}

// El mando de prueba de los tests (backend_test_pad): un mando virtual de
// SDL, que llega como uno de verdad (SDL_CONTROLLERDEVICEADDED, eventos de
// botones). Los botones del mando virtual siguen el orden de
// SDL_GameControllerButton (su asignacion es a:b0, b:b1... dpright:b14).
#if SDL_VERSION_ATLEAST(2, 0, 14)
static int sdl_test_pad(Backend* backend, int button, int type) {
  SDLPlatformData* data = (SDLPlatformData*)backend->platform_data;
  if (button < 0 || button >= GAMEPAD_BUTTONS || standard_buttons[button] == SDL_CONTROLLER_BUTTON_INVALID) return -1;
  SDL_GameControllerButton sb = standard_buttons[button];
  if (!data->test_pad_on) {
    int idx = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX,
                                        SDL_CONTROLLER_BUTTON_MAX, 0);
    if (idx < 0) {
      fprintf(stderr, "[test pad] %s\n", SDL_GetError());
      return -1;
    }
    data->test_pad = SDL_JoystickGetDeviceInstanceID(idx);
    data->test_pad_on = 1;
    sdl_open_gamepads(data);
  }
  for (int i = 0; i < GAMEPAD_MAX; i++) {
    SDL_Joystick *joy = data->pads[i] ? SDL_GameControllerGetJoystick(data->pads[i]) : NULL;
    if (!joy || SDL_JoystickInstanceID(joy) != data->test_pad) continue;
    if (type == TEST_PAD_PRESS) {
      // pulsado y soltado entre dos imagenes: el estado de SDL no cambia,
      // solo llegan los dos eventos (lo que ve data->latch)
      SDL_Event ev;
      memset(&ev, 0, sizeof(ev));
      ev.cbutton.which = data->test_pad;
      ev.cbutton.button = (Uint8)sb;
      ev.type = SDL_CONTROLLERBUTTONDOWN;
      ev.cbutton.state = SDL_PRESSED;
      SDL_PushEvent(&ev);
      ev.type = SDL_CONTROLLERBUTTONUP;
      ev.cbutton.state = SDL_RELEASED;
      SDL_PushEvent(&ev);
    } else {
      // el estado cambia al procesar los eventos de la siguiente imagen
      SDL_JoystickSetVirtualButton(joy, sb, type == TEST_PAD_DOWN ? SDL_PRESSED : SDL_RELEASED);
    }
    return i;
  }
  return -1;
}
#else
#define sdl_test_pad NULL
#endif
