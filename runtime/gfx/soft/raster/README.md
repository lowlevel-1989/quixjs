# raster

English | [Español](README.es.md)

The software rasterizer of the 2d canvas, split by topic so it can be read:
the paths, the clip masks and painting on a Surface.

## Why ../raster.c only includes these files

`gfx/soft/raster.c` is an index file: it includes the parts of this folder in
order and is compiled as a single file. The parts are not compiled by
themselves (`cmake/sources.cmake` only lists `raster.c`).

The order matters: a part uses what the parts before it define. A new part
goes into `raster.c` in its place, after what it uses. Each part includes
`common.h` (with `#pragma once`), so that it can be read and opened in an
editor by itself.

## Parts, in the order of raster.c

| file | what it has |
|------|-------------|
| `common.h` | what the parts share: the includes and `../raster.h` |
| `path.c` | the path: subpaths of points (`path_move_to`, `path_line_to`, `path_close`) and the current point |
| `mask.c` | the clip mask: the fill of a path (non-zero / even-odd), its stroke with caps and joins, and the intersection of two masks |
| `composite.c` | painting on the Surface: a paint through a mask (`raster_fill_mask`), `raster_fill_rect` (with its fast paths and `raster_light`), `raster_clear_rect` and `raster_draw_image` |
