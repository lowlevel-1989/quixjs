# Que soporta

[English](features.md) | Español

Lo que quixjs sabe hacer hoy, en resumen. El estado de cada API del navegador
esta en [compatibilidad](compatibility.es.md).

- JavaScriptCore con su JIT (en aarch64 y x86_64)
- WebGL 1 sobre OpenGL ES 2: PixiJS usa su renderer WebGL, y un canvas WebGL va a
  la pantalla directo desde la GPU, sin leerlo de vuelta. `--no-webgl` o
  `[engine] webgl = false` lo apagan
- canvas 2d por software, igual con ventana y sin ella: rectangulos, paths, arcos,
  curvas, trazos, clip, transformaciones, save/restore, globalAlpha, operaciones de
  composicion (multiply, screen, lighter...), drawImage, ImageData, gradientes,
  patrones
- texto con SDL_ttf: fillText, strokeText, measureText, `@font-face`, `FontFace` y
  `[fonts]` en el toml del juego
- audio: WebAudio (AudioContext, decodeAudioData, nodos de ganancia y paneo, loops,
  rampas) sobre un mezclador propio (`runtime/audio`); ogg vorbis decodificado con
  stb_vorbis mientras suena. `new Audio()` existe pero solo responde a
  `canPlayType` (no suena)
- video: `<video>` reproduce WebM con VP8 o VP9 y audio Opus (nestegg, libvpx,
  libopus); la imagen sigue al audio y se ve encima del juego
- hooks en el toml del juego (`[hooks]`): cambian una funcion del juego sin tocar sus
  archivos. `gpu-canvas` dibuja un canvas 2d en la GPU (las mascaras de luces de
  RPG Maker), `skip` y `once-per-frame`
  ([runtime/hooks](../runtime/hooks/README.es.md))
- `[engine] fps` (de 10 a 60) y `vsync`: un port puede dibujar a 30 fps mientras
  RPG Maker sigue con 60 actualizaciones por segundo. En una portatil la bateria
  es una prioridad: por eso los ports van a 30 fps por defecto (la mitad de
  imagenes que dibujar, la CPU y la GPU a menos frecuencia), y quixjs no sube
  las frecuencias del sistema; lo que haga falta se gana abaratando el trabajo
- Image (PNG, JPG... data URIs y urls `blob:`), Blob y `URL.createObjectURL`,
  XMLHttpRequest para archivos locales (texto, json, arraybuffer)
- modo nw.js (`[engine] nwjs = true`): `require('fs')`, `path`, `events`, `os`,
  `nw.gui`, `process`, y `require('./x')` de los archivos del juego
- `<script>` anadidos con appendChild (plugins), DOMContentLoaded, window.onload, y
  JS antiguo (`RegExp.$1..$9`, que JavaScriptCore trae)
- setTimeout, setInterval, requestAnimationFrame, performance.now
- DOM minimo: document, body, createElement, appendChild, getElementById, eventos
- teclado (KeyboardEvent con key, code y keyCode) y mandos (Gamepad API, mapeo
  standard); Select + Start sale. Una pulsacion mas corta que una imagen el juego
  la ve igual
- `--launcher`: un menu antes del juego (el launcher) con el titulo del juego y
  una pequena firma "quixjs": Play, Achievements, Mods, Options y Cheats, y
  debajo, siempre a la vista, Transfer Save, Controls y Exit (cada opcion
  solo si el juego la usa). Viene de
  tamatama (https://github.com/lowlevel-1989/tamatama), un proyecto anterior
  del mismo autor ([runtime/launcher](../runtime/launcher/README.es.md))
- Transfer Save: pasa las partidas a otro dispositivo de la misma red, sin abrir
  puertos, con los logros ([runtime/transfer](../runtime/transfer/README.es.md))
- logros, definidos en el toml del juego, con un aviso al conseguir uno y, en
  RPG Maker MV, una opcion Achievements en el propio menu del juego
  ([runtime/achievements](../runtime/achievements/README.es.md))
- trucos (`[[cheat]]`), que se activan en el launcher; la partida jugada con uno
  no consigue logros ([runtime/cheats](../runtime/cheats/README.es.md))
- `--debug` y `tools/console.py`: una consola de debug para pulsar teclas, hacer
  capturas, ejecutar JS y pausar el juego (heredada de tamatama)
- `--script`: teclas y capturas en frames dados, para automatizar pruebas
- modo sin ventana (`dummy`) con reloj fijo y capturas, para los tests
- 46 juegos de test en `games/tests`, la mayoria se pueden abrir tambien en un
  navegador
- PortMaster: `make PLATFORM=portmaster` construye un port para portatiles aarch64

Todavia no: raton y pantalla tactil, guardar localStorage en disco,
antialiasing en canvas 2d, `toDataURL`. Estado de cada API:
[docs/compatibility.es.md](compatibility.es.md).
