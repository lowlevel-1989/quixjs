# video

[English](README.md) | Español

Video WebM para el elemento `<video>`: los "Play Movie" de los eventos de
RPG Maker MV. Está hecho con librerías, no desde cero.

| librería  | qué hace |
|-----------|----------|
| nestegg   | lee el contenedor WebM (`thirdparty/nestegg`, se compila con quixjs) |
| libvpx    | decodifica el video VP8 y VP9 |
| libopus   | decodifica el audio Opus (el audio de los WebM de RPG Maker) |

| archivo   | qué hace |
|-----------|----------|
| `video.h` | abrir, play, pausa, volumen, avanzar, fin, tiempo, imágenes decodificadas |
| `video.c` | lee los paquetes con nestegg, los decodifica con libvpx y libopus, pasa la imagen a RGBA; sin video cada función es un sustituto vacío |

La parte de JS es `binding/video.c` (`__quixVideo`) y el elemento `<video>`
de `binding/polyfill.c` ([binding](../binding/README.es.md)).

## Cómo se reproduce

- Toma la primera pista de video VP8 o VP9 y la primera pista de audio Opus.
  Un archivo sin video VP8 o VP9 no se abre. Otras pistas de audio (Vorbis)
  no se decodifican: el video se ve sin sonido.
- El audio se decodifica a 48 kHz, unos 0,5 s por delante, y va al mezclador
  como un flujo (`mixer_stream_*` en [audio](../audio/README.es.md)). El mono
  se copia a los dos canales y se quita el pre-skip de la cabecera Opus.
- La imagen sigue el reloj del audio (lo que el mezclador ha sonado). Sin
  audio, o sin dispositivo de audio, sigue el reloj del runtime.
- La imagen se decodifica en un hilo propio, que también la pasa de YUV
  (I420, I422, I444; BT.601, rango limitado) a RGBA en la CPU con enteros.
  Deja hasta 3 imágenes listas por delante; cada imagen del juego enseña la
  que ya toca.
- Un video termina cuando no quedan paquetes y el audio se ha oído.

## En pantalla

`binding/video.c` guarda hasta 4 videos. Con el backend sdl el primer video
visible que no ha terminado se dibuja como una capa encima de todos los
canvas. El elemento `<video>` dice si es visible por su `style` (`display`,
`visibility`, `opacity`). El backend dummy y las capturas no lo enseñan.

## Compilar

El `Makefile` compila con video cuando `pkg-config` encuentra `vpx` y `opus`
(`VIDEO=1`). `make VIDEO=0` compila sin él: `video_open` devuelve NULL y el
elemento `<video>` se salta (`loadeddata` y `ended` enseguida, así el juego
sigue).

```
# Debian / Ubuntu
sudo apt-get install libvpx-dev libopus-dev
```

El build de PortMaster siempre lleva video y lleva `libvpx.so.5` y
`libopus.so.0` en `libs.aarch64/` y `libs.armhf/`, con las licencias de libvpx5, libopus0 y nestegg.

El test es `games/tests/media/video`: `clip.webm` (64x48, 1 s, 10 imágenes,
hecho con ffmpeg desde `testsrc` y un tono `sine`) se reproduce hasta el
final, y un archivo que no existe se salta.
