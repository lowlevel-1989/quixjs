# drivers

English | [Español](README.es.md)

All drivers have the same interface (`GpuDriver`, in `../driver.h`) and each
one fills it in its file. The common code does not look at which driver is in
use: it calls the points of the interface in their place. `../driver.c` picks
the driver when the GL context is created (`[gpu] driver: ...` in the
output, `quix.gpu.driver` in JS).

## Glossary

- **Mesa**: the free graphics drivers of Linux, one per GPU family
  ([list](https://docs.mesa3d.org/systems.html), [code](https://gitlab.freedesktop.org/mesa/mesa)).
- **panfrost**: the Mesa one for modern Mali GPUs (Mali-T, Mali-G)
  ([docs](https://docs.mesa3d.org/drivers/panfrost.html),
  [code](https://gitlab.freedesktop.org/mesa/mesa/-/tree/main/src/gallium/drivers/panfrost)).
- **libmali**: the one of ARM for its Mali GPUs, closed
  ([ARM](https://developer.arm.com/downloads/-/mali-drivers/user-space)).
- **present_ahead**: another thread presents the previous frame while the
  game draws the next one.

## Drivers

| driver     | what it is                     | tested on |
|------------|--------------------------------|-----------|
| `panfrost` | Mesa with "Panfrost" in GL_RENDERER | RG CubeXX (Mali-G31, Mesa 24.3), Powkiddy RGB30 (Mali-G52, Mesa 24.3) |
| `mali`     | "Mali" in GL_RENDERER, without Mesa | Powkiddy RGB30 (Mali-G52) |
| `mali-g31` | "Mali-G31" in GL_RENDERER, without Mesa (libmali on an RK3326: R36S and clones) | RX6S (Mali-G31, ROCKNIX) |
| `default`  | the rest (a PC); not optimized for | PC (Intel UHD 630, Mesa) |

## How the driver is picked

1. When the game starts, `gl_init` (`../gl.c`) creates the OpenGL ES
   context.
2. It asks the driver for two of its strings: `GL_RENDERER` (which GPU and
   driver) and `GL_VERSION` (which OpenGL ES version and from whom).
3. `gpu_driver_detect` (`../driver.c`) looks at them once:

   ```c
   int mesa = has(version, "mesa");
   if (mesa && has(renderer, "panfrost")) current = &gpu_driver_panfrost;
   else if (!mesa && has(renderer, "mali-g31")) current = &gpu_driver_mali_g31;
   else if (!mesa && has(renderer, "mali")) current = &gpu_driver_mali;
   else current = &gpu_driver_default;
   ```

   `has` looks for the word ignoring case: each version of a driver can
   write its name differently ("Panfrost", "panfrost"...).
4. The output says it (`[gpu] driver: panfrost`) and from then on
   `gpu_driver()` returns that driver.

What our devices say (from their logs):

| device | GL_RENDERER | GL_VERSION | driver |
|--------|-------------|------------|--------|
| RG CubeXX | `Mali-G31 (Panfrost)` | `OpenGL ES 3.1 Mesa 24.3.4` | panfrost |
| Powkiddy RGB30 (gpu.driver=panfrost) | `Mali-G52 r1 (Panfrost)` | `OpenGL ES 3.1 Mesa 24.3.4` | panfrost |
| Powkiddy RGB30 (gpu.driver=libmali) | `Mali-G52` | `OpenGL ES 3.2 v1.g13p0-01eac0...` | mali |
| RX6S (R36S clone) | `Mali-G31` | `OpenGL ES 3.2 v1.g13p0-01eac0...` | mali-g31 |
| PC | `Mesa Intel(R) UHD Graphics 630 (CFL GT2)` | `OpenGL ES 3.2 Mesa 26.0.6` | default |

## The interface

`GpuDriver` (`../driver.h`): what each driver fills in its file.

| field           | what it is | used in |
|-----------------|------------|---------|
| `thread_wait(fence)` | function: how that thread waits for the GPU to finish each frame. Without it (`0`), the driver does not use the thread and everything goes through `present` | `backend/sdl/thread.c`, the thread and `thread_start` |
| `thread_max_fps` | number: with a higher `[engine] fps` the thread is not used (`0`: no limit) | `backend/sdl/thread.c`, `thread_start` |
| `present(p, f)` | function: how a frame is presented without that thread (`present_ahead = false`, or if the compositor holds it back) | `backend/sdl/present.c`, `gpu_present()` |
| `one_context` | 1: without that thread, the frame goes to the screen with the game's context, not another one (the GPU does it in order: no waiting between contexts) | `backend/sdl/present.c`, `one_context()` |

`present` gets the frame (`GpuFrame`) and what the backend can do with it
(`Presenter`), explained below.

## The shared pieces (common.c)

The functions the drivers use are in `common.c` (and `common.h`). Each
driver picks its own; what only one driver uses goes in its own file (there
is nothing like that yet).

| piece | for | what it does |
|-------|-----|--------------|
| `drv_present_now` | `present` | waits for the GPU to finish the frame and shows it |
| `drv_present_previous` | `present` | shows the previous frame, already finished, and keeps this one for the next |
| `drv_wait_gpu` | `thread_wait` | the GPU waits (`eglWaitSyncKHR`); the thread goes on |
| `drv_wait_cpu` | `thread_wait` | the thread's CPU waits for the GPU to finish |

What each driver picks, and why (what was measured):

| | panfrost | mali | mali-g31 | default |
|-|----------|------|----------|---------|
| `thread_wait` | `drv_wait_gpu`: with `drv_wait_cpu`, on the RG CubeXX from ~70 to ~390 late frames in battle | `drv_wait_cpu`: with `drv_wait_gpu`, the menus flickered | none (no thread): with it, the game waited ~16 ms per frame; on the map from ~17 to ~27 fps | `drv_wait_gpu` |
| `present` | `drv_present_previous`: on the RGB30 with panfrost, from 353 to 76 late frames on the map and from 940 to 194 in battle | `drv_present_previous`: on the RGB30, from ~240 to ~60 on the map and from ~270 to ~85 in battle | `drv_present_previous`, like mali | `drv_present_now` |
| `thread_max_fps` | 0 (no limit) | 30: at 60 fps with the thread each frame took ~50 ms to show (HexGL on the RGB30: ~18 fps; without the thread, 60) | (no thread) | 0 |
| `one_context` | 0: it uses the thread; without it and with one context, on the RG CubeXX (Fear & Hunger at 60 fps) the map the same and battles worse (~1000 frames in 40 s against ~1400 with the thread) | 1: with two contexts, at 60 fps the windows of Fear & Hunger flickered (libmali showed the frame unfinished even after waiting for its fence) | 1, like mali (RG35XX+ with muOS) | 0 |

## Why the fps decide whether the thread is used (thread_max_fps)

The thread shows the frame ahead: the game hands it frame N and goes on with
N+1, while the thread waits for the GPU to finish N and for the screen
refresh to show it. For that it needs spare time between frames:

- At 30 fps each frame stays on screen for two refreshes (33 ms). The thread
  has a whole refresh to spare to wait for the GPU and show it, and the game
  does not notice.
- At 60 fps each frame lasts a single refresh (16 ms), so there is no room.
  While the thread has not shown frame N yet, the game already has N+1 ready
  and has to wait for it. With libmali that wait piled up (about 50 ms per
  frame), which is why HexGL dropped to 18 fps.

## How the thread waits for the GPU (thread_wait)

When the game ends a frame, the CPU has only sent drawing commands: the GPU
carries them out later, a while after. The presenting thread has to copy that
frame to the window, but not before the GPU finishes painting it: it would
copy a half-done frame. So, when the frame is handed over, a mark (fence) is
put after its commands; when the GPU reaches the mark, the frame is done.
"Waiting" means waiting for that mark, and there are two ways.

**Waiting on the GPU** (`drv_wait_gpu`):

1. The thread tells the GPU: "when you finish this frame, copy it to the
   window".
2. The thread does not wait: it goes on with its work.
3. The GPU finishes painting the frame.
4. The GPU, on its own, copies the frame to the window.

```c
// the presenting thread, every frame (simplified)
eglWaitSyncKHR(dpy, fence, 0);  // to the GPU: "do not go on until you pass the mark"; returns at once
draw_layers(...);               // command: copy the frame to the window (queued on the GPU)
SDL_GL_SwapWindow(window);      // frame change
```

**Waiting on the CPU** (`drv_wait_cpu`):

1. The thread stops.
2. The GPU finishes painting the frame and tells it.
3. The thread tells the GPU: "copy the frame to the window".
4. The GPU copies the frame to the window.

```c
// the presenting thread, every frame (simplified)
eglClientWaitSyncKHR(dpy, fence, 0, EGL_FOREVER);  // the thread stops here until the GPU passes the mark
draw_layers(...);                                  // command: copy the frame to the window
SDL_GL_SwapWindow(window);                         // frame change
```

The only difference in the code is the first line: `eglWaitSyncKHR` queues
the wait on the GPU and returns; `eglClientWaitSyncKHR` does not return
until the GPU passes the mark. They are in `drv_wait_gpu` and `drv_wait_cpu`
(`common.c`, which call `gl_fence_wait_gpu` and `gl_fence_wait_cpu` in
`../gl.c`); the thread is `present_thread` in `backend/sdl/thread.c`.

The difference: on the GPU, the copy command is already queued when the frame
ends, and the copy goes out at once. On the CPU, a little time passes between
the GPU's notice and the copy command (the thread has to wake up), but the
thread knows for sure the frame is finished.

What each driver uses, and why (we tried both ways on each):

- **panfrost uses `drv_wait_gpu`.** With `drv_wait_cpu`, the thread woke up
  late: the copy arrived after the screen refresh and the frame was seen one
  refresh later. On the RG CubeXX, late frames in battle: ~70 with
  `drv_wait_gpu`, ~390 with `drv_wait_cpu`.
- **mali (libmali) uses `drv_wait_cpu`.** With `drv_wait_gpu`, libmali
  sometimes copied the frame to the window before it was finished: a
  half-done frame showed on screen and the RPG Maker menus flickered. With
  `drv_wait_cpu` it does not happen (on the RGB30, 31 captures in a row of
  the battle menu, all complete).
- **mali-g31 does not use the thread.** With libmali on a Mali-G31 (RX6S),
  the game waited ~16 ms every frame for the thread to show the previous
  one. Without the thread, seeded, from ~400 to ~100 frames over 60 ms on
  the map and from ~510 to ~210 in battle (out of ~1000). The RG CubeXX has
  the same GPU with panfrost, and there the thread does help: it is a
  libmali-on-G31 thing.

## GpuFrame and Presenter

Both are defined in `../driver.h`. The backend (`backend/sdl.c`)
makes them and passes them to the driver's `present`.

### GpuFrame: a frame ready to present

```c
typedef struct GpuFrame {
  int later;   // 1: it can be shown later; 0: it must be shown now
} GpuFrame;
```

The driver only sees this field. The backend keeps the rest (the layers and
the GPU fence) in its own struct, `SdlFrame`, which has `GpuFrame` as its
first field.

### How quixjs knows if `later` is 1

The game draws every frame on its WebGL canvas. Inside, that canvas is a
texture: an image in GPU memory.

With `drv_present_previous` (without the thread) every frame is shown one frame
later. With a single texture that would not work: when frame 1 had to be
shown, the game would already be clearing that texture to draw frame 2 over
it.

So the canvas has three textures, called A, B and C here. All three are
always in GPU memory; none goes through the CPU. At any moment each one has a
role:

- **where the game draws**: the game's JS runs on the CPU and sends drawing
  commands (WebGL); the GPU carries them out and paints on this texture the
  frame being made;
- **waiting**: a finished frame (the GPU is done painting it), to be shown
  next;
- **on screen**: the frame seen now; the GPU copied it from this texture to
  the window (`show`, in the backend).

At the end of every frame, quixjs swaps their roles. Frame by frame:

| frame being made | where the game draws | waiting            | on screen          |
|------------------|----------------------|--------------------|--------------------|
| 1                | A                    | (none)             | (none)             |
| 2                | B                    | A, with frame 1    | (none)             |
| 3                | C                    | B, with frame 2    | A, with frame 1    |
| 4                | A                    | C, with frame 3    | B, with frame 2    |
| 5                | B                    | A, with frame 4    | C, with frame 3    |

At the end of every frame: the texture where the game drew goes to wait, the
one that was waiting goes to the screen, and the one that was on screen is
free for the game to draw the next frame. At the start, the screen takes two
frames to have something. The game never draws on a texture that is waiting
or on screen. The CPU swaps the roles without moving pixels: it only changes
which texture the canvas points to (`webgl_canvas_handoff`, `binding/webgl.c`,
called at the end of every frame from `present_screen`, `runtime/core/main/screen.c`).

`later` is 1 if everything on screen has its texture set aside like that. It
is 0 in two cases:

- the WebGL canvas has `preserveDrawingBuffer`: the game asked to go on
  drawing over what it had, so its texture cannot be changed;
- there is a 2d canvas or a video on screen: they are not textures set
  aside, they are pixels in CPU memory that the game changes in the next
  frame.

`sdl_present_layers` (`backend/sdl/present.c`) works it out. The canvases on
screen come from `document_screen_canvases` (`binding/document.c`): those of
the `body` without `display: none`, `visibility: hidden` or `opacity: 0`.

In Fear & Hunger only `GameCanvas` is seen (WebGL, without
preserveDrawingBuffer), so `later` is 1, except on the loading screen (the 2d
`UpperCanvas` of RPG Maker is seen) and with a video.

### Presenter: what the backend can do with a frame

```c
typedef struct {
  void (*wait)(GpuFrame *f);
  void (*show)(GpuFrame *f);
  void (*drop)(GpuFrame *f);
} Presenter;
```

| operation | what it does | in `backend.c` |
|-----------|--------------|----------------|
| `wait(f)` | the CPU waits for the GPU to finish drawing `f` | `frame_wait` |
| `show(f)` | draws `f` on the window and shows it (with vsync) | `frame_show` |
| `drop(f)` | `f` will not be shown: releases it | `frame_drop` |

The driver combines them in its `present`. The rules:

- every frame ends in `wait` and `show`, or in `drop`;
- if `later` is 0, the frame is shown in the same call;
- the driver can keep the frame for the next call, only that one: the
  backend alternates two and reuses the older one.

Example, `mali.c`: it shows the kept frame (the previous one) and keeps the
new one.

A new point: the field in `GpuDriver`, its function `gpu_<point>()` in
`../driver.c` (it calls the driver's one), the call in its place, its value
in each driver and its row here. If several drivers do the same thing, the
piece goes in `common.c`.

A new driver: its file, its `extern` in `../driver.h`, its case in
`gpu_driver_detect` (`../driver.c`) and its column here.
