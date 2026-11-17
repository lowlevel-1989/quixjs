# shaders

[English](README.md) | Español

Los shaders GLSL ES 1.0 de quixjs, uno por archivo. No se leen al arrancar:
`tools/embed_shaders.py` pasa cada archivo a una cadena de C en
`obj/gen/shaders.h` antes de compilar (lo hace el `Makefile` cuando cambia un
shader), así quixjs sigue siendo un solo programa. `gpu2d.frag` pasa a ser
`SHADER_GPU2D_FRAG`.

| archivo        | qué hace |
|----------------|----------|
| `present.vert` | el backend sdl copia una textura a la ventana: el canvas del juego, una capa, el contador de `--fps`, el aviso de un logro |
| `present.frag` | la textura tal cual, con una opacidad (`u_alpha`: el aviso de un logro se desvanece; 1 para lo demás) |
| `gpu2d.vert`   | canvas 2d en la GPU (`gfx/gpu2d.c`, hook `gpu-canvas`): la posición, y el gradiente preparado por vértice |
| `gpu2d.frag`   | el color, o el del gradiente sacado de la tabla `u_lut`, por la opacidad y la máscara del path, premultiplicado |

## gpu2d

`gfx/gpu2d.c` pone `#define MODE` y `#define MASK` delante de los dos
archivos y hace un programa por caso: `MODE` 0 color, 1 gradiente lineal,
2 gradiente radial; `MASK` 0 o 1. No hay ramas en el fragment shader, que es
lo que cuesta en una GPU pequeña (Mali-G31).

El fragment shader no comparte ningún uniform con el vertex shader: con
precisiones distintas (highp allí, mediump aquí) Mesa no enlaza el programa.
