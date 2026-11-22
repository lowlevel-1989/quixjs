# audio

[English](README.md) | Español

El mezclador de audio, pensado para portátiles. `binding/audio.c` mantiene el
grafo de WebAudio en JS (fuente -> ganancia -> panner -> ...) y, en cada frame,
pasa el volumen, el paneo y la velocidad de cada voz a este mezclador, que las
mezcla en el thread de audio de SDL.

| archivo      | de qué se encarga |
|--------------|-------------------|
| `mixer.h/.c` | clips (el archivo comprimido), voces (play, set, stop, loop), el flujo PCM de un video, mezcla en el callback de audio de SDL |
| `decoder.h/.c` | un sonido desde memoria sin saber su formato: lo elige por sus primeros bytes y pasa a 1 o 2 canales |
| `vorbis.c`   | ogg vorbis, con stb_vorbis (`thirdparty/stb/`) compilado aqui, solo con lectura desde memoria; los comentarios LOOPSTART / LOOPLENGTH |
| `mp3.c`      | mp3, con minimp3 (`thirdparty/minimp3/`) |
| `wav.c`      | wav: PCM de 8, 16, 24 o 32 bits, o float |

## Streaming

Un `AudioClip` es el ogg vorbis comprimido que da `decodeAudioData`; no se
decodifica entero. Cada voz que suena tiene su propio decodificador y su
posición, y decodifica mientras suena. Una música de 3 minutos ocupa sus
~3 MB, no ~30 MB de PCM.
Una larga cargada desde su archivo (sin cifrar) ni se copia: se mapea
(mmap), así sus páginas son del archivo y el kernel las puede soltar si falta
memoria (Gunfield guarda cada música que ha sonado: 26 músicas, 91 MB). Una
corta (hasta 3 s, los efectos) se decodifica entera al cargarla.

## El flujo de un video

`mixer_stream_*` es un flujo de PCM ya decodificado: el audio de un
`<video>` ([video](../video/README.es.md)). El thread principal lo va llenando
y el thread de audio lo mezcla con las voces. Hay uno a la vez, con sitio para
2 s de estéreo a 48 kHz; lo que no cabe se pierde. `mixer_stream_time()` es
el tiempo ya sonado, el reloj que sigue la imagen del video. Sin dispositivo
de audio el flujo "suena" al ritmo del reloj del runtime.

## Sin dispositivo de audio

Con el backend dummy (tests) no hay dispositivo: las voces avanzan con el reloj
del runtime sin decodificar, así `onended` y los loops funcionan igual.
Mientras suena una voz, el loop principal sigue avanzando frame a frame.
