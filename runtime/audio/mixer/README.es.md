# mixer

[English](README.md) | Español

El mezclador de audio, partido por temas para que se pueda leer: los clips,
decodificar y mezclar en el thread de audio de SDL, las voces y el flujo de
PCM de un video.

## Por que ../mixer.c solo incluye estos archivos

`audio/mixer.c` es un archivo indice: incluye las partes de esta carpeta en
orden y se compila como un solo archivo. Las partes no se compilan solas
(`cmake/sources.cmake` solo lista `mixer.c`).

El orden importa: una parte usa lo que definen las anteriores. Una parte nueva
va en `mixer.c` en su sitio, despues de lo que usa. Cada parte incluye
`common.h` (con `#pragma once`), para que se pueda leer y abrir en el editor
por separado.

## Partes, en el orden de mixer.c

| archivo | que tiene |
|---------|-----------|
| `common.h` | lo que comparten las partes: `AudioClip`, `Voice`, el flujo de PCM y el estado del mezclador |
| `clip.c` | los clips: los sonidos cargados de un archivo, de memoria o de PCM (los cortos, decodificados enteros), y cuantos los usan |
| `decode.c` | decodificar y mezclar cada voz (con su pan, velocidad y loop), en el callback de audio de SDL |
| `voice.c` | las voces: play, set (volumen, pan, velocidad, loop), stop y el tiempo de cada una |
| `device.c` | el dispositivo de audio: abrirlo (o seguir sin el, por el reloj), cada frame (`mixer_update`) y cerrarlo |
| `stream.c` | el flujo de PCM (el audio de un video): una cola que mezcla el callback, y su tiempo |
