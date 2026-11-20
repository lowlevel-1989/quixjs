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
