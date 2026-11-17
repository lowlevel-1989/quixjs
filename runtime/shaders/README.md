# shaders

English | [Español](README.es.md)

The GLSL ES 1.0 shaders of quixjs, one file each. They are not read at run
time: `tools/embed_shaders.py` turns each file into a C string in
`obj/gen/shaders.h` before compiling (the `Makefile` does it when a shader
changes), so quixjs stays a single program. `gpu2d.frag` becomes
`SHADER_GPU2D_FRAG`.

| file           | what it does |
|----------------|--------------|
| `present.vert` | the sdl backend copies a texture to the window: the game canvas, a layer, the `--fps` counter, the notice of an achievement |
| `present.frag` | the texture as it is, with an opacity (`u_alpha`: the notice of an achievement fades out; 1 for the rest) |
| `gpu2d.vert`   | 2d canvas on the GPU (`gfx/gpu2d.c`, hook `gpu-canvas`): position, and the gradient prepared per vertex |
| `gpu2d.frag`   | the color, or the gradient color from the table `u_lut`, by the opacity and the mask of the path, premultiplied |

## gpu2d

`gfx/gpu2d.c` puts `#define MODE` and `#define MASK` in front of both files
and builds one program for each case: `MODE` 0 color, 1 linear gradient,
2 radial gradient; `MASK` 0 or 1. There are no branches in the fragment
shader, which is what costs on a small GPU (Mali-G31).

The fragment shader shares no uniform with the vertex shader: with different
precisions (highp there, mediump here) Mesa does not link the program.
