# sdl backend

English | [Español](README.es.md)

The sdl backend, split by topic so it can be read: the window (SDL2), each
frame shown with OpenGL ES 2, the thread that presents, the overlays, the
keyboard and the gamepads.

## Why ../sdl.c only includes these files

`backend/sdl.c` is an index file: it includes the parts of this folder in
order and is compiled as a single file. The parts are not compiled by
themselves (`cmake/sources.cmake` only lists `sdl.c`); `mapper.c` is not a part:
it is compiled apart, as always.

The order matters: a part uses what the parts before it define. A new part
goes into `sdl.c` in its place, after what it uses. Each part includes
`common.h` (with `#pragma once`), so that it can be read and opened in an
editor by itself.

## Parts, in the order of sdl.c

| file | what it has |
|------|-------------|
| `common.h` | what the parts share: `SDLPlatformData` (window, context, layers, the thread that presents) and its types |
| `window.c` | the window and OpenGL ES: open the gamepads, the shaders that present, draw a texture (`sdl_blit`), `sdl_init` |
| `input.c` | keyboard and gamepads: SDL keys as `KeyboardEvent.code`, the mapping of a gamepad (Controls of the launcher), fullscreen, title, refresh rate and the event queue (`sdl_process_events`) |
| `overlay.c` | what is drawn over the game at the resolution of the window: the `--fps` counter, the notice of an achievement and the loading notice |
| `draw.c` | showing a frame: the canvas layers scaled to the window and the buffer swap |
| `magnifier.c` | the magnifier (Select + L1): open, pin and close it, its buttons, moving it, hiding the gamepad from the game (pinned, only the buttons that pinned it), the box and its icon (pinned, with a padlock) |
| `thread.c` | the thread that presents (`[engine] present_ahead`): it gets each frame with its time and shows it then, without the game waiting |
| `present.c` | handing over a frame: with the thread or without it (`gpu_driver_present` and the `Presenter` of the driver), repainting during a load, and the three entries of the backend (`present_layers`, `present`, `present_gl`) |
| `gamepads.c` | `getGamepads`: the state of the gamepads in the "standard" mapping, read from SDL once per frame (the same until the events of the next one; a press shorter than a frame is seen in all the reads of that frame), and the test gamepad (`quix.input.pressPad`) |
| `api.c` | close, the `BackendInterface` table and `sdl_backend_create` |

Compiled apart (not parts): `mapper.c`, the mapping of a gamepad or of the
keyboard to the buttons of the game (Controls of the launcher), and
`keypad.c`, the keyboard as a gamepad (`keyboard.map`: the assigned keys come
in through the same standard gamepad as the buttons). `backend.h` is what the rest
of quixjs sees: `sdl_backend_create`.
