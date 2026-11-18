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

// La tabla de teclas. Ver keymap.h.

#include <string.h>
#include <stdio.h>
#include "keymap.h"

static const KeyInfo keymap[] = {
  { "KeyA",          "a",           "A",    65 },
  { "KeyB",          "b",           "B",    66 },
  { "KeyC",          "c",           "C",    67 },
  { "KeyD",          "d",           "D",    68 },
  { "KeyE",          "e",           "E",    69 },
  { "KeyF",          "f",           "F",    70 },
  { "KeyG",          "g",           "G",    71 },
  { "KeyH",          "h",           "H",    72 },
  { "KeyI",          "i",           "I",    73 },
  { "KeyJ",          "j",           "J",    74 },
  { "KeyK",          "k",           "K",    75 },
  { "KeyL",          "l",           "L",    76 },
  { "KeyM",          "m",           "M",    77 },
  { "KeyN",          "n",           "N",    78 },
  { "KeyO",          "o",           "O",    79 },
  { "KeyP",          "p",           "P",    80 },
  { "KeyQ",          "q",           "Q",    81 },
  { "KeyR",          "r",           "R",    82 },
  { "KeyS",          "s",           "S",    83 },
  { "KeyT",          "t",           "T",    84 },
  { "KeyU",          "u",           "U",    85 },
  { "KeyV",          "v",           "V",    86 },
  { "KeyW",          "w",           "W",    87 },
  { "KeyX",          "x",           "X",    88 },
  { "KeyY",          "y",           "Y",    89 },
  { "KeyZ",          "z",           "Z",    90 },
  { "Digit0",        "0",           ")",    48 },
  { "Digit1",        "1",           "!",    49 },
  { "Digit2",        "2",           "@",    50 },
  { "Digit3",        "3",           "#",    51 },
  { "Digit4",        "4",           "$",    52 },
  { "Digit5",        "5",           "%",    53 },
  { "Digit6",        "6",           "^",    54 },
  { "Digit7",        "7",           "&",    55 },
  { "Digit8",        "8",           "*",    56 },
  { "Digit9",        "9",           "(",    57 },
  { "F1",            "F1",          NULL,  112 },
  { "F2",            "F2",          NULL,  113 },
  { "F3",            "F3",          NULL,  114 },
  { "F4",            "F4",          NULL,  115 },
  { "F5",            "F5",          NULL,  116 },
  { "F6",            "F6",          NULL,  117 },
  { "F7",            "F7",          NULL,  118 },
  { "F8",            "F8",          NULL,  119 },
  { "F9",            "F9",          NULL,  120 },
  { "F10",           "F10",         NULL,  121 },
  { "F11",           "F11",         NULL,  122 },
  { "F12",           "F12",         NULL,  123 },
  { "Enter",         "Enter",       NULL,   13 },
  { "NumpadEnter",   "Enter",       NULL,   13 },
  { "Escape",        "Escape",      NULL,   27 },
  { "Space",         " ",           NULL,   32 },
  { "Tab",           "Tab",         NULL,    9 },
  { "Backspace",     "Backspace",   NULL,    8 },
  { "ShiftLeft",     "Shift",       NULL,   16 },
  { "ShiftRight",    "Shift",       NULL,   16 },
  { "ControlLeft",   "Control",     NULL,   17 },
  { "ControlRight",  "Control",     NULL,   17 },
  { "AltLeft",       "Alt",         NULL,   18 },
  { "AltRight",      "Alt",         NULL,   18 },
  { "ArrowLeft",     "ArrowLeft",   NULL,   37 },
  { "ArrowUp",       "ArrowUp",     NULL,   38 },
  { "ArrowRight",    "ArrowRight",  NULL,   39 },
  { "ArrowDown",     "ArrowDown",   NULL,   40 },
  { "PageUp",        "PageUp",      NULL,   33 },
  { "PageDown",      "PageDown",    NULL,   34 },
  { "End",           "End",         NULL,   35 },
  { "Home",          "Home",        NULL,   36 },
  { "Insert",        "Insert",      NULL,   45 },
  { "Delete",        "Delete",      NULL,   46 },
  { "Pause",         "Pause",       NULL,   19 },
  { "CapsLock",      "CapsLock",    NULL,   20 },
  { "Minus",         "-",           "_",   189 },
  { "Equal",         "=",           "+",   187 },
  { "BracketLeft",   "[",           "{",   219 },
  { "BracketRight",  "]",           "}",   221 },
  { "Backslash",     "\\",          "|",   220 },
  { "Semicolon",     ";",           ":",   186 },
  { "Quote",         "'",           "\"",  222 },
  { "Backquote",     "`",           "~",   192 },
  { "Comma",         ",",           "<",   188 },
  { "Period",        ".",           ">",   190 },
  { "Slash",         "/",           "?",   191 },
  { "Numpad0",       "0",           NULL,   96 },
  { "Numpad1",       "1",           NULL,   97 },
  { "Numpad2",       "2",           NULL,   98 },
  { "Numpad3",       "3",           NULL,   99 },
  { "Numpad4",       "4",           NULL,  100 },
  { "Numpad5",       "5",           NULL,  101 },
  { "Numpad6",       "6",           NULL,  102 },
  { "Numpad7",       "7",           NULL,  103 },
  { "Numpad8",       "8",           NULL,  104 },
  { "Numpad9",       "9",           NULL,  105 },
  { NULL, NULL, NULL, 0 }
};

const KeyInfo *keymap_find(const char *code) {
  if (!code) return NULL;
  for (const KeyInfo *k = keymap; k->code; k++) {
    if (strcmp(k->code, code) == 0) return k;
  }
  return NULL;
}

int keymap_make_event(InputEvent *ev, InputType type, const char *code, int shift, int repeat) {
  const KeyInfo *k = keymap_find(code);
  if (!k) return -1;
  memset(ev, 0, sizeof(*ev));
  ev->type = type;
  ev->key_code = k->key_code;
  ev->repeat = repeat;
  snprintf(ev->code, sizeof(ev->code), "%s", k->code);
  snprintf(ev->key, sizeof(ev->key), "%s", (shift && k->shift_key) ? k->shift_key : k->key);
  return 0;
}
