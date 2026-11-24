# shaders

English | [Español](README.es.md)

The GLSL ES 1.0 shaders of quixjs, one file each. They are not read at run
time: `tools/embed_shaders.py` turns each file into a C string in
`obj/cmake/<platform>/gen/shaders.h` before compiling (`cmake/generated.cmake` does it when a
shader changes), so quixjs stays a single program. `gpu2d.frag` becomes
`SHADER_GPU2D_FRAG`.

They are two pairs with different jobs:

| file           | what it does |
|----------------|--------------|
| `gpu2d.vert`   | 2d canvas on the GPU (`gfx/gl/gpu2d.c`, hook `gpu-canvas`): the position, and the gradient prepared per vertex |
| `gpu2d.frag`   | the color, or the gradient color from the table `u_lut`, by the opacity and the mask of the path, premultiplied |
| `present.vert` | the sdl backend copies a texture to the window: the game canvas, a layer, the `--fps` counter, the notice of an achievement |
| `present.frag` | the texture as it is, with an opacity (`u_alpha`), and scaled up with "sharp bilinear" when it is the game canvas |

## gpu2d: the 2d canvas on the GPU

With the hook `gpu-canvas`, some draws of a 2d canvas are done on the GPU
instead of in `gfx/soft/raster.c`: `fillRect`, `clearRect`, and `fill` and
`stroke` with a color or a gradient. The rest (text, `drawImage`,
patterns...) goes back to the CPU. `gpu2d.c` gathers the draws of a frame and
sends them at once.

### One program per case

`gpu2d.c` (`build()`) compiles both files several times, with
`#define MODE` and `#define MASK` in front:

| | `MASK 0` | `MASK 1` |
|---|---|---|
| `MODE 0`: plain color | a rectangle of color | a color cut by the shape of the path |
| `MODE 1`: linear gradient | a rectangle with a gradient | a gradient cut by the path |
| `MODE 2`: radial gradient | a rectangle with a gradient | a gradient cut by the path |

That is 6 programs, and each draw uses its own (`programs[mode][mask]`). So
the fragment shader has no `if`: on a small GPU (Mali-G31) branches per
pixel are what costs most. `clearRect` uses no shader: it is a plain OpenGL
clear (`MODE_CLEAR` in `gpu2d.c`).

### gpu2d.vert: per vertex

Each draw is a rectangle (two triangles). For each vertex:

- The position on the canvas, in pixels, goes to GPU coordinates (-1 to 1)
  with `u_size`, the size of the canvas. Row 0 of the canvas (top) ends at
  the bottom of the framebuffer: so memory stays in the order of the canvas,
  to read it (`getImageData`) or copy it to a WebGL texture without flipping.
- The gradient is prepared here and not per pixel:
  - linear (`MODE 1`): `t`, how far from the first color to the last, is the
    projection of the position on the line of the gradient (`u_g`: its two
    points). Since `t` changes linearly, the GPU interpolates it between the
    vertices and the fragment shader gets it done.
  - radial (`MODE 2`): the position relative to the center divided by the
    outer radius (`u_r.y`), and `v_k`, the inner radius on that same scale.
    They are small values, so `mediump` is enough in the fragment shader.
- `a_mt` is the coordinate of the pixel inside the mask, passed as it is.

### gpu2d.frag: per pixel

1. The color. With `MODE 0` it is `u_color`. With a gradient:
   - the linear one brings `t`; the radial one computes it from the distance
     to the center (`length(v_g)`), 0 at the inner radius and 1 at the outer;
   - it looks the color up in `u_lut`: a texture of 256 colors per gradient,
     one row each (`u_row`), that `gpu2d.c` fills on the CPU with `paint.c`.
     Reading a table is cheaper than mixing the stops of the gradient for
     each pixel, and gives the same colors as drawing by software.
2. The opacity: the alpha of the color by `globalAlpha` (`u_alpha`).
3. The mask (`MASK 1`): the opacity is also multiplied by `u_mask`, the
   coverage of the path (which pixels are inside the shape), computed on the
   CPU by `raster.c` and uploaded as an alpha only texture. So a `fill()` of
   any shape is drawn as a rectangle that is cut.
4. The result is premultiplied (`rgb * a`, `a`), what the OpenGL blending
   for `source-over` expects.

The fragment shader shares no uniform with the vertex shader: with different
precisions (`highp` there, `mediump` here) Mesa (panfrost) does not link the
program.

## present: the frame to the window

The sdl backend composes each frame copying
textures to the window with `sdl_blit` (`backend/sdl/window.c`, which loads
these two shaders; `draw.c` and `overlay.c` use it): the game canvas, the
layers, the `--fps` counter and
the notices (an achievement, "Loading...").

- `present.vert` places the rectangle of the texture in the window and
  passes the coordinate of the texture (`v_uv`).
- `present.frag` copies the texture with an opacity, `u_alpha`: the notice of
  an achievement lowers it to fade out; for everything else it is 1.

### Sharp bilinear

When the game canvas is scaled up to the screen (`u_prescale > 0`),
`present.frag` does "sharp bilinear", as emulators do:

- `u_size` are the pixels of the texture and `u_prescale` the whole part of
  the scale (256 to 640 is 2.5x: `u_prescale` is 2).
- For each pixel of the screen it looks where it falls inside the pixel of
  the game (`fract(texel) - 0.5`, from -0.5 to 0.5). In the middle it takes
  the pixel as it is; only in a thin band next to the edge it mixes with the
  neighbor. The width of that band comes from `u_prescale`.
- Result: each pixel of the game looks sharp and of the same size, and the
  part of the scale that is not whole (the 0.5 of 2.5x) is spread by
  smoothing only those edges. With the linear filter alone everything looked
  blurry; with nearest, some pixels came out wider than others.

Without scaling (`u_prescale = 0`), the texture is copied as it is.
