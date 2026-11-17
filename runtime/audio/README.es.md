# audio

[English](README.md) | Español

El mezclador de audio, pensado para portátiles. `binding/audio.c` mantiene el
grafo de WebAudio en JS (fuente -> ganancia -> panner -> ...) y, en cada frame,
pasa el volumen, el paneo y la velocidad de cada voz a este mezclador, que las
mezcla en el hilo de audio de SDL.

| archivo      | de qué se encarga |
|--------------|-------------------|
| `mixer.h/.c` | clips (ogg comprimido), voces (play, set, stop, bucle), el flujo PCM de un video, mezcla en el callback de audio de SDL |
| `vorbis.c`   | compila stb_vorbis (`thirdparty/stb/`) aparte, solo con lectura desde memoria |

## Streaming

Un `AudioClip` es el ogg vorbis comprimido que da `decodeAudioData`; no se
decodifica entero. Cada voz que suena tiene su propio decodificador y su
posición, y decodifica mientras suena. Una música de 3 minutos ocupa sus
~3 MB, no ~30 MB de PCM.

## El flujo de un video

`mixer_stream_*` es un flujo de PCM ya decodificado: el audio de un
`<video>` ([video](../video/README.es.md)). El hilo principal lo va llenando
y el hilo de audio lo mezcla con las voces. Hay uno a la vez, con sitio para
2 s de estéreo a 48 kHz; lo que no cabe se pierde. `mixer_stream_time()` es
el tiempo ya sonado, el reloj que sigue la imagen del video. Sin dispositivo
de audio el flujo "suena" al ritmo del reloj del runtime.

## Sin dispositivo de audio

Con el backend dummy (tests) no hay dispositivo: las voces avanzan con el reloj
del runtime sin decodificar, así `onended` y los bucles funcionan igual.
Mientras suena una voz, el bucle principal sigue avanzando frame a frame.
