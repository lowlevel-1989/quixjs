# shaders

[English](README.md) | Español

Los shaders GLSL ES 1.0 de quixjs, uno por archivo. No se leen al arrancar:
`tools/embed_shaders.py` pasa cada archivo a un string de C en
`obj/cmake/<plataforma>/gen/shaders.h` antes de compilar (lo hace `cmake/generated.cmake`
cuando cambia un shader), así quixjs sigue siendo un solo binario. `gpu2d.frag` pasa a ser
`SHADER_GPU2D_FRAG`.

Son dos parejas con trabajos distintos:

| archivo        | qué hace |
|----------------|----------|
| `gpu2d.vert`   | canvas 2d en la GPU (`gfx/gl/gpu2d.c`, hook `gpu-canvas`): la posición, y el gradient preparado por vertex |
| `gpu2d.frag`   | el color, o el del gradient sacado de la LUT `u_lut`, por el alpha y la mask del path, en premultiplied alpha |
| `present.vert` | el backend sdl copia una texture a la ventana: el canvas del juego, un layer, el contador de `--fps`, el aviso de un logro |
| `present.frag` | la texture tal cual, con un alpha (`u_alpha`), y escalada con sharp bilinear si es el canvas del juego |

## gpu2d: el canvas 2d en la GPU

Con el hook `gpu-canvas`, algunos draw calls de un canvas 2d se hacen en la
GPU en vez de en `gfx/soft/raster.c`: `fillRect`, `clearRect`, y `fill` y
`stroke` con un solid color o un gradient. Lo demás (texto, `drawImage`,
patterns...) vuelve a la CPU. `gpu2d.c` junta los draw calls de un frame y
los manda en un batch.

### Un shader program por caso

`gpu2d.c` (`build()`) compila los dos archivos varias veces, con
`#define MODE` y `#define MASK` delante:

| | `MASK 0` (sin mask) | `MASK 1` (con mask) |
|---|---|---|
| `MODE 0`: solid color | un rect de solid color | un solid color recortado por la mask del path |
| `MODE 1`: linear gradient | un rect con linear gradient | un linear gradient recortado por la mask del path |
| `MODE 2`: radial gradient | un rect con radial gradient | un radial gradient recortado por la mask del path |

Son 6 shader programs, y cada draw call usa el suyo
(`programs[mode][mask]`). Así el fragment shader no tiene `if`: en una GPU
pequeña (Mali-G31) los branches por pixel son lo que más cuesta.
`clearRect` no usa shader: es un clear directo de OpenGL (`MODE_CLEAR` en
`gpu2d.c`).

### gpu2d.vert: el vertex shader

Cada draw call es un rect (dos triangles). Para cada vertex:

- La posición en el canvas, en pixels, pasa a clip space (de -1 a 1) con
  `u_size`, el tamaño del canvas. La fila 0 del canvas (arriba) queda abajo
  del framebuffer: así la memoria queda en el orden del canvas, para leerla
  (`getImageData`) o copiarla a una texture de WebGL sin hacer flip.
- El gradient se prepara aquí, por vertex, y no en cada pixel:
  - linear gradient (`MODE 1`): `t` (de 0 en el primer color stop a 1 en el
    último) es la proyección de la posición sobre la línea del gradient
    (`u_g`: sus dos puntos). Como `t` cambia de forma lineal, la GPU lo
    interpola entre los vertices y el fragment shader lo recibe hecho.
  - radial gradient (`MODE 2`): la posición relativa al centro dividida por
    el radio exterior (`u_r.y`), y `v_k`, el radio interior en esa misma
    escala. Son valores pequeños, y por eso `mediump` basta en el fragment
    shader.
- `a_mt`, la coordenada de la mask (texture coordinate), pasa tal cual.

### gpu2d.frag: el fragment shader

Por cada pixel:

1. El color. Con `MODE 0` es `u_color`. Con gradient:
   - el linear gradient ya trae `t`; el radial gradient lo calcula con la
     distancia al centro (`length(v_g)`), de 0 en el radio interior a 1 en
     el exterior;
   - busca el color en `u_lut`, una LUT (lookup table): una texture de 256
     colores por gradient, una fila cada uno (`u_row`), que `gpu2d.c`
     rellena en la CPU con `paint.c`. Leer una LUT es más barato que mezclar
     los color stops en cada pixel, y da los mismos colores que el render
     por software.
2. El alpha: el del color por `globalAlpha` (`u_alpha`).
3. La mask (`MASK 1`): el alpha se multiplica también por `u_mask`, el
   coverage del path (qué pixels quedan dentro de la forma), calculado en la
   CPU por `raster.c` y subido como una texture de solo alpha. Así un
   `fill()` de cualquier forma se dibuja como un rect recortado por la mask.
4. El resultado sale en premultiplied alpha (`rgb * a`, `a`), lo que espera
   el blending de OpenGL para `source-over`.

El fragment shader no comparte ningún uniform con el vertex shader: con
precisiones distintas (`highp` allí, `mediump` aquí) Mesa (panfrost) no
hace el link del shader program.

## present: el frame a la ventana

El backend sdl compone cada frame copiando textures a la ventana con
`sdl_blit` (`backend/sdl/window.c`, que carga estos dos shaders; lo usan
`draw.c` y `overlay.c`): el canvas del juego, los layers, el contador de
`--fps` y los avisos (un logro, "Loading...").

- `present.vert` coloca el rect de la texture en la ventana y pasa su texture
  coordinate (`v_uv`).
- `present.frag` copia la texture con un alpha, `u_alpha`: el aviso de un
  logro lo baja para hacer fade out; para todo lo demás es 1.

### Sharp bilinear

Cuando el canvas del juego se escala a la pantalla (`u_prescale > 0`),
`present.frag` hace sharp bilinear, como los emuladores:

- `u_size` son los texels de la texture y `u_prescale` la parte entera del
  scale (de 256 a 640 es 2,5x: `u_prescale` es 2).
- En cada pixel de la pantalla mira en qué parte del texel cae
  (`fract(texel) - 0.5`, de -0.5 a 0.5). En el centro toma el texel tal
  cual; solo en una franja estrecha junto al borde hace la interpolación
  bilinear con el vecino. El ancho de esa franja sale de `u_prescale`.
- Resultado: cada pixel del juego se ve nítido y del mismo tamaño, y lo que
  no es entero del scale (el 0,5 de 2,5x) se reparte suavizando solo esos
  bordes. Con el linear filter solo, todo se veía borroso; con nearest,
  unos pixels salían más anchos que otros.

Sin scale (`u_prescale = 0`), la texture se copia tal cual.
