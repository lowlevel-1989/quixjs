# webgl

English | [Español](README.es.md)

WebGL 1 on OpenGL ES 2.0, split by topic so it can be read.

## Why ../webgl.c only includes these files

`binding/webgl.c` is an index file: it includes the parts of this folder in
order and is compiled as a single file. The parts are not compiled by
themselves (the `Makefile` only knows `webgl.c`).

This way the compiler sees the same text as when it was a single file and
generates the same code: it puts the small functions of one part inside
another (`webgl_get`, the argument readers `U`/`I`/`F`/`B`...), and the WebGL
methods are called hundreds of times per frame. Compiled apart, each of those
calls would be a real function call. The machine code of `webgl.o` is
byte-for-byte the same as with the old single file.

The order matters: a part uses what the parts before it define (`args.h`
defines the macros the methods are written with). A new part goes into
`webgl.c` in its place, after what it uses. Each part includes `common.h`
(and `args.h` if it uses the macros), with `#pragma once`, so that it can be
read and opened in an editor by itself.

## Parts, in the order of webgl.c

| file | what it has |
|------|-------------|
| `common.h` | what the parts share: the context (`WebGLCtx`), the objects (`GLObj`, `GLLoc`) and the state |
| `canvas.c` | the WebGL canvas: its framebuffer, the textures handed over to present, the resize |
| `classes.c` | the JS classes: `WebGLRenderingContext`, `WebGLObject`, `WebGLUniformLocation` |
| `args.h` | the argument readers `U`/`I`/`F`/`B` and the macros the methods are written with (`ARGS`, `CTX`, `SIMPLE`) |
| `state.c` | the GL state (blend, depth, stencil, viewport, enable...), `clear`, `drawArrays` and `drawElements` |
| `objects.c` | `create*`, `delete*` and `is*` of every object |
| `shaders.c` | shaders and programs: compile, link (with the shader cache), attributes |
| `uniforms.c` | `uniform*` and `vertexAttrib*` |
| `buffers.c` | `bufferData` and `bufferSubData` |
| `textures.c` | `texImage2D` and `texSubImage2D` (from images, canvas and arrays), premultiply and flip, the copy cache |
| `queries.c` | `getParameter`, `getError`, `get*Parameter`... |
| `extensions.c` | `getExtension` and `getSupportedExtensions` |
| `api.c` | the list of methods of `WebGLRenderingContext`, `js_api_add_webgl` and `js_create_webgl_context` |

## Framebuffers and textures, on handhelds

- **One framebuffer per target texture** (`objects.c`, `fb_alias`): Construct
  2 draws its effects in textures with a single framebuffer whose texture it
  changes (`framebufferTexture2D`). On Mali that re-validates the framebuffer,
  ~0.3 ms each time; changing framebuffer does not. While the game's
  framebuffer has only a colour texture attached, quixjs uses one of its own
  with that texture, made once (until the texture is deleted). When the game
  attaches a renderbuffer (three.js: depth after colour), its own framebuffer
  is used again, with the colour texture attached too. The game sees no
  difference. Gunfield on the RGB30 (libmali): from ~11 to ~19 fps; RX6S
  (Panfrost): from 5.9 to 7.8.
- **Textures filled when the image is decoded** (`textures.c`,
  `upload_later`): with `[engine] lazy_images` an image is decoded when it is
  used; `texImage2D` with an image without pixels makes the texture empty, of
  its size, decodes it in the worker thread and fills it then (with the same
  premultiply / flip_y, and mipmaps again if they were asked for). A sprite
  sheet of 2048x2048 took up to 0.5 s on a handheld, in the frame where a new
  enemy appeared; now it is not shown for a moment instead. Only with
  `lazy_images`: an image released for lack of RAM (RPG Maker) is decoded
  again at once, as before.
