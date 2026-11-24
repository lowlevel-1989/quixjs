# gpu2d

English | [Español](README.es.md)

The 2d canvas drawn on the GPU (`gpu-canvas` hook), split by topic so it can
be read: the programs, the texture of each canvas, the pending draw calls,
what the canvas calls and the canvas as a WebGL texture.

## Why ../gpu2d.c only includes these files

`gfx/gl/gpu2d.c` is an index file: it includes the parts of this folder in
order and is compiled as a single file. The parts are not compiled by
themselves (`cmake/sources.cmake` only lists `gpu2d.c`).

The order matters: a part uses what the parts before it define. A new part
goes into `gpu2d.c` in its place, after what it uses. Each part includes
`common.h` (with `#pragma once`), so that it can be read and opened in an
editor by itself.

## Parts, in the order of gpu2d.c

| file | what it has |
|------|-------------|
| `common.h` | what the parts share: `Cmd` (a draw call), `Gpu2D` (a canvas) and the list of the canvases that draw in a WebGL texture |
| `program.c` | the auxiliary context and the programs: one per paint (color, linear, radial), with or without mask |
| `target.c` | the texture and the framebuffer of each canvas: create, free and resize |
| `batch.c` | the pending draw calls: blending, the last full repaint (to skip an identical one) and the flush, in the WebGL context or the auxiliary one |
| `draw.c` | what the 2d canvas calls: `gpu2d_fill_rect`, `gpu2d_clear_rect` and `gpu2d_fill_mask` |
| `texture.c` | the canvas as a texture: read it back, its texture, draw in a WebGL texture (`gpu2d_retarget`) and forget it when WebGL deletes it |
