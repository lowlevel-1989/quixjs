#pragma once

// El teclado como un mando mas: el lado SDL (ver runtime/backend/sdl/keypad.c)

#include <SDL2/SDL.h>
#include "backend/backend.h"

void keypad_load(const char *map);              // keyboard.map, junto a controller.map
int  keypad_save(const char *map, const char *pairs);   // "a:k:Return,dpup:k:Up,..."
void keypad_event(const SDL_Event *e);          // cada evento: las teclas pulsadas
int  keypad_active(void);                       // 1 si keyboard.map asigna algo
int  keypad_has(SDL_Keycode key);               // 1 si esa tecla es del mando del teclado
void keypad_merge(GamepadState *pad);           // suma las teclas al mando (pad 0)
