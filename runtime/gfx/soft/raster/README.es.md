# raster

[English](README.md) | Español

El rasterizado por software del canvas 2d, partido por temas para que se pueda
leer: los paths, las masks de recorte y pintar sobre una Surface.

## Por que ../raster.c solo incluye estos archivos

`gfx/soft/raster.c` es un archivo indice: incluye las partes de esta carpeta en
orden y se compila como un solo archivo. Las partes no se compilan solas
(`cmake/sources.cmake` solo lista `raster.c`).

El orden importa: una parte usa lo que definen las anteriores. Una parte nueva
va en `raster.c` en su sitio, despues de lo que usa. Cada parte incluye
`common.h` (con `#pragma once`), para que se pueda leer y abrir en el editor
por separado.

## Partes, en el orden de raster.c

| archivo | que tiene |
|---------|-----------|
| `common.h` | lo que comparten las partes: los includes y `../raster.h` |
| `path.c` | el path: subpaths de puntos (`path_move_to`, `path_line_to`, `path_close`) y el punto actual |
| `mask.c` | la mask de recorte: el fill de un path (non-zero / even-odd), su stroke con caps y joins, y la interseccion de dos masks |
| `composite.c` | pintar en la Surface: un paint a traves de una mask (`raster_fill_mask`), `raster_fill_rect` (con sus caminos rapidos y `raster_light`), `raster_clear_rect` y `raster_draw_image` |
