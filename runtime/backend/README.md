# backend

English | [Español](README.es.md)

Window, input and presentation of the frame. `backend.h` is the common
interface (a table of functions) and `backend.c` calls the backend chosen with
`-b`. The backends put the keys in an input queue; `core/main.c` takes them
out and turns them into KeyboardEvent. `quix.pressKey()` uses the same queue.

| file              | what it does |
|-------------------|--------------|
| `backend.h/.c`    | `BackendInterface`, `InputEvent`, `GamepadState`, `PresentLayer`; creates the backend and keeps the input queue |
| `sdl.c`           | SDL window presented with OpenGL ES 2, keyboard, gamepads, `--fps` overlay. An index file: it joins the parts of `sdl/` and is compiled as one ([README](sdl/README.md)) |
| `sdl/mapper.c`    | Controls of the launcher: reads the raw gamepad and saves a SDL_GAMECONTROLLERCONFIG line in `controller.map` (inherited from tamatama) |
| `dummy/backend.c` | no window: keeps the last frame for `--screenshot` and the tests |

`sdl.c` is an index file (`sdl/` has its parts, [README](sdl/README.md));
`dummy` is a single small file and needs none.

## sdl

- Presents with OpenGL ES 2, as on handhelds with KMSDRM (SDL drawing
  straight to the screen, without a desktop). Each canvas on
  screen is a layer: a 2d canvas is uploaded to a texture only when it
  changed, a WebGL canvas goes straight (it shares textures with the WebGL
  context). All the layers are scaled to the rectangle of the first one (the
  game canvas), keeping the aspect ratio with black bars. A visible `<video>`
  is one more layer, on top. The shaders are in `runtime/shaders/present.*`.
- Keys: the SDL scancode becomes a KeyboardEvent.code and `core/keymap.c`
  gives its key and keyCode.
- Gamepads: up to 4 with SDL_GameController, given to `navigator.getGamepads()`
  with the "standard" mapping of the browser. Select + Start at the same time
  exits (on a handheld there is no window to close). A button pressed and
  released between two reads is latched: the next `getGamepads()` reports it
  pressed once.
- Vsync: `SDL_GL_SetSwapInterval(1)` unless `[engine] vsync = false`.
- Frame ahead (`[engine] present_ahead`): with the fps at half the refresh
  or less, another thread presents each frame on its refresh and the game
  goes on with the next one. Only with Mesa, the refreshes of the screen
  (DRM) and `EGL_KHR_surfaceless_context`. With `EGL_KHR_fence_sync` the
  context that presents waits until the GPU has finished the WebGL canvas.
- `--fps`: the text is drawn at the top right at the resolution of the window,
  not of the game, so it does not touch the game canvas.
- Very small games (some tests) get the window scaled by an integer; the game
  canvas does not change.

## Tiling window managers

The window is created with a fixed size so that tiling window managers (sway...)
leave it floating, and it becomes resizable only once it is on screen
(exposed) and some frames later, as in tamatama. `--resizable` makes it
resizable from the start, and `--fullscreen` uses the whole screen with black
bars.
