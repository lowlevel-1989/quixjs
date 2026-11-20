# gfx

[English](README.md) | Español

Dibujo sin JS. El canvas 2d de `binding/canvas/2d/` dibuja aquí por software
sobre una `Surface` RGBA en memoria, igual con el backend sdl que con el dummy.
`gl.c` es el contexto OpenGL ES 2.0 que usan WebGL (`binding/webgl.c`) y el
backend sdl para presentar el frame. `gpu2d.c` dibuja un canvas 2d en la GPU
cuando lo pide un hook `gpu-canvas`.

| archivo          | de qué se encarga |
|------------------|-------------------|
| `surface.h/.c`   | superficie RGBA (sin premultiplicar, mismo orden que ImageData), matriz afín, mezcla de los modos de `globalCompositeOperation` |
| `color.c`        | colores CSS: `#rgb`, `rgb()`, `hsl()`, nombres...; se escriben como el navegador (`#rrggbb` o `rgba(...)`) |
| `paint.c`        | lo que va en fillStyle / strokeStyle: color, gradiente lineal y radial, patrón; se muestrea por pixel |
| `raster.c`       | paths, relleno (nonzero / evenodd), trazo, máscaras de cobertura (también para `clip()`), fillRect, clearRect, drawImage |
| `image_loader.c` | decodifica imágenes (PNG, JPG...) con SDL_image y guarda PNG para las capturas |
| `font.c`         | fuentes TrueType con SDL_ttf: familias registradas, valor CSS de `font`, medir y dibujar texto; un thread a la vez (el juego y los avisos encima de el, que dibuja el thread que presenta) |
| `gl.h/.c`        | contexto OpenGL ES 2.0 para WebGL (ventana oculta de SDL, u offscreen sin ventana); funciones cargadas con `SDL_GL_GetProcAddress`; un segundo contexto que comparte texturas (`gl_aux_begin` / `gl_aux_end`) para `gpu2d.c` |
| `gpu2d.h/.c`     | canvas 2d dibujado en la GPU (hook `gpu-canvas`): fillRect, clearRect, fill y stroke con color o gradiente; lo demás vuelve a la CPU; un repintado entero igual al anterior (una máscara de luces con las luces y la pantalla quietas) no se vuelve a dibujar; pinta en el contexto de WebGL, guardando y devolviendo su estado (`QUIXJS_GPU2D_MAIN=0`: el contexto auxiliar) |
| `shader_cache.h/.c` | los programas de WebGL ya enlazados, guardados en `<cache_dir>/shaders` (`GL_OES_get_program_binary`): la siguiente vez se cargan en unos ms en vez de compilarlos; si el driver ya no acepta uno, se enlaza y se guarda de nuevo |
| `driver.h/.c`    | que driver de OpenGL ES se usa (panfrost, mali o default), leido una vez de `GL_RENDERER` y `GL_VERSION`; el resto del codigo pregunta a `gpu_driver()` en vez de mirar el nombre |
| `drivers/*.c`    | un archivo por driver con lo que quixjs hace distinto con el, y por que (lo medido) |
| `gl_funcs.h`     | lista de funciones de GLES 2.0, generada por `tools/gen_gl.py` (no editar a mano) |

## Sin antialiasing

`raster.c` pinta un pixel cuando su centro queda dentro de la figura, como con
`imageSmoothingEnabled = false`. La cobertura es 0 o 255 por pixel, y la misma
máscara sirve para `clip()`.

## Caminos rápidos

`blend_pixel` (en `surface.h`) resuelve `source-over` y `lighter` en línea y
con enteros, porque los usan casi todos los dibujos; el resto de modos pasa por
`blend_pixel_slow`. `lighter` sobre un destino opaco (una máscara de luces de
los plugins de iluminación de RPG Maker MV) es una suma con tope sin divisiones.
`raster_fill_rect` tiene un camino rápido cuando la matriz solo traslada y
escala, y los gradientes se precalculan en una tabla de 256 colores
(`PaintSampler`).

`raster_light` es la luz de los plugins de iluminación: un fillRect con un
gradiente radial concéntrico en modo `lighter`, sin clip ni giro. Su color
sale de una tabla indexada por la distancia al cuadrado (sin raíz), cada fila
solo recorre el tramo dentro del círculo cuando el último color no suma nada,
y sobre un destino opaco son tres sumas con tope por pixel. Un relleno
`lighter` con negro solo toca los pixeles que no son opacos.

## GL sin GLES en el sistema

Las cabeceras de GL son las que trae SDL2 y las funciones se cargan al
arrancar, así que para compilar no hace falta GLES instalado (tampoco para
PortMaster). El contexto de WebGL se crea en la ventana del backend sdl y
comparte texturas con él: un canvas WebGL se muestra sin leerlo de la GPU.

## Canvas 2d en la GPU

`gpu2d.c` dibuja los canvas que marca un hook `gpu-canvas`
([hooks](../hooks/README.es.md)): las máscaras de luces de RPG Maker, que se
repintan enteras en cada actualización, son el dibujo por software más caro
en una portátil.

- El canvas vive en una textura (alfa premultiplicado). Se dibuja con un
  segundo contexto GL que comparte texturas con el de WebGL
  (`gl_aux_begin` / `gl_aux_end` en `gl.c`), así que no toca el estado GL
  del juego.
- Los comandos de dibujo se apuntan y se pintan de una vez cuando hace
  falta: cuando WebGL sube el canvas o cuando se lee.
- Un relleno opaco de todo el canvas tira los comandos apuntados (no se
  verían) y pasa a ser un `glClear`. Un relleno `lighter` con negro sobre un
  canvas opaco se salta.
- En la GPU: fillRect, clearRect, fill y stroke (la cobertura del path se
  calcula por software y se sube como máscara), con un color o un gradiente
  (lineal, o radial con un solo centro), globalAlpha, la transformación y los
  modos source-over, lighter, destination-out, destination-over, source-atop,
  xor y screen.
- Un programa de shaders por pintura (color, lineal, radial) y con o sin
  máscara: sin ramas en el fragment shader. Los shaders están en
  [shaders](../shaders/README.es.md).
- Todo lo demás (clip, patrones, otros modos, texto, imágenes,
  getImageData...) lee el canvas y el canvas se queda en la CPU.
- `texImage2D` de WebGL con ese canvas lo copia en la GPU con
  `glCopyTexSubImage2D` (`glCopyTexImage2D` la primera vez o si cambia el
  tamaño), cuando el juego pide alfa premultiplicado y sin dar la vuelta, como
  hace PIXI. Si no, se lee como cualquier canvas.
- `drawImage` de ese canvas lo lee, y el canvas sigue en la GPU.

## Fuentes

Una familia se registra desde JS (FontFace, `@font-face`) o desde `[fonts]` en
el toml del juego. Si una familia no está registrada se usa la fuente por defecto:
la variable de entorno `QUIXJS_FONT` o la primera que exista de una lista de
fuentes habituales de Linux (DejaVu, Noto, Liberation).

## Drivers

Lo que quixjs hace distinto con cada driver de OpenGL ES: [drivers](drivers/README.es.md).
