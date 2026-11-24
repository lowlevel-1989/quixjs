# gpu2d

[English](README.md) | Español

El canvas 2d dibujado en la GPU (hook `gpu-canvas`), partido por temas para que
se pueda leer: los programs, la texture de cada canvas, los draw calls
pendientes, lo que llama el canvas y el canvas como texture de WebGL.

## Por que ../gpu2d.c solo incluye estos archivos

`gfx/gl/gpu2d.c` es un archivo indice: incluye las partes de esta carpeta en
orden y se compila como un solo archivo. Las partes no se compilan solas
(`cmake/sources.cmake` solo lista `gpu2d.c`).

El orden importa: una parte usa lo que definen las anteriores. Una parte nueva
va en `gpu2d.c` en su sitio, despues de lo que usa. Cada parte incluye
`common.h` (con `#pragma once`), para que se pueda leer y abrir en el editor
por separado.

## Partes, en el orden de gpu2d.c

| archivo | que tiene |
|---------|-----------|
| `common.h` | lo que comparten las partes: `Cmd` (un draw call), `Gpu2D` (un canvas) y la lista de los canvas que dibujan en una texture de WebGL |
| `program.c` | el contexto auxiliar y los programs: uno por paint (color, lineal, radial), con o sin mask |
| `target.c` | la texture y el framebuffer de cada canvas: crearlo, liberarlo y cambiar su tamaño |
| `batch.c` | los draw calls pendientes: el blending, el ultimo repintado entero (para saltarse uno igual) y el flush, en el contexto de WebGL o en el auxiliar |
| `draw.c` | lo que llama el canvas 2d: `gpu2d_fill_rect`, `gpu2d_clear_rect` y `gpu2d_fill_mask` |
| `texture.c` | el canvas como texture: leerlo, su texture, dibujar en una texture de WebGL (`gpu2d_retarget`) y olvidarla cuando WebGL la borra |
