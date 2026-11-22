# webgl

[English](README.md) | Español

WebGL 1 sobre OpenGL ES 2.0, partido por temas para que se pueda leer.

## Por que ../webgl.c solo incluye estos archivos

`binding/webgl.c` es un archivo indice: incluye las partes de esta carpeta en
orden y se compila como un solo archivo. Las partes no se compilan solas (el
`Makefile` solo conoce `webgl.c`).

Asi el compilador ve el mismo texto que cuando era un solo archivo y genera el
mismo codigo: mete dentro las funciones pequenas de una parte en otra
(`webgl_get`, los lectores de argumentos `U`/`I`/`F`/`B`...), y los metodos de
WebGL se llaman cientos de veces por imagen. Compiladas aparte, cada una de
esas llamadas seria una llamada de funcion de verdad. El codigo maquina de
`webgl.o` es identico byte a byte al del archivo unico de antes.

El orden importa: una parte usa lo que definen las anteriores (`args.h` define
los macros con los que se escriben los metodos). Una parte nueva va en
`webgl.c` en su sitio, despues de lo que usa. Cada parte incluye `common.h`
(y `args.h` si usa los macros), con `#pragma once`, para que se pueda leer y
abrir en el editor por separado.

## Partes, en el orden de webgl.c

| archivo | que tiene |
|---------|-----------|
| `common.h` | lo que comparten las partes: el contexto (`WebGLCtx`), los objetos (`GLObj`, `GLLoc`) y el estado |
| `canvas.c` | el canvas de WebGL: su framebuffer, las texturas que se entregan para presentar, el cambio de tamano |
| `classes.c` | las clases de JS: `WebGLRenderingContext`, `WebGLObject`, `WebGLUniformLocation` |
| `args.h` | los lectores de argumentos `U`/`I`/`F`/`B` y los macros con los que se escriben los metodos (`ARGS`, `CTX`, `SIMPLE`) |
| `state.c` | el estado de GL (blend, depth, stencil, viewport, enable...), `clear`, `drawArrays` y `drawElements` |
| `objects.c` | `create*`, `delete*` e `is*` de cada objeto |
| `shaders.c` | shaders y programas: compilar, enlazar (con la cache de shaders), atributos |
| `uniforms.c` | `uniform*` y `vertexAttrib*` |
| `buffers.c` | `bufferData` y `bufferSubData` |
| `textures.c` | `texImage2D` y `texSubImage2D` (desde imagenes, canvas y arrays), premultiplicar y voltear, la cache de copias |
| `queries.c` | `getParameter`, `getError`, `get*Parameter`... |
| `extensions.c` | `getExtension` y `getSupportedExtensions` |
| `api.c` | la lista de metodos de `WebGLRenderingContext`, `js_api_add_webgl` y `js_create_webgl_context` |

## Framebuffers y texturas, en las handhelds

- **Un framebuffer por textura de destino** (`objects.c`, `fb_alias`):
  Construct 2 dibuja sus efectos en texturas con un solo framebuffer al que
  le cambia la textura (`framebufferTexture2D`). En Mali eso vuelve a validar
  el framebuffer, ~0,3 ms cada vez; cambiar de framebuffer, no. Mientras el
  framebuffer del juego solo tiene una textura de color enganchada, quixjs usa
  uno propio con esa textura, creado una vez (hasta que se borra la textura).
  Si el juego engancha un renderbuffer (three.js: la profundidad despues del
  color), se vuelve al suyo, con la textura de color enganchada tambien. El
  juego no nota nada. Gunfield en la RGB30 (libmali): de ~11 a ~19 fps;
  RX6S (Panfrost): de 5,9 a 7,8.
- **Texturas que se rellenan al decodificarse la imagen** (`textures.c`,
  `upload_later`): con `[engine] lazy_images` una imagen se decodifica al
  usarla; `texImage2D` con una imagen sin pixeles crea la textura vacia, de su
  tamano, la decodifica en el thread de trabajo y la rellena entonces (con el
  mismo premultiply / flip_y, y los mipmaps otra vez si se pidieron). Una hoja
  de sprites de 2048x2048 tardaba hasta 0,5 s en una handheld, en la imagen en
  que aparecia un enemigo nuevo; ahora tarda un momento en verse. Solo con
  `lazy_images`: una imagen soltada por falta de RAM (RPG Maker) se vuelve a
  decodificar al momento, como antes.
