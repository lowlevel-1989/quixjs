# mixer

English | [Español](README.es.md)

The audio mixer, split by topic so it can be read: the clips, decoding and
mixing in the SDL audio thread, the voices and the PCM stream of a video.

## Why ../mixer.c only includes these files

`audio/mixer.c` is an index file: it includes the parts of this folder in
order and is compiled as a single file. The parts are not compiled by
themselves (`cmake/sources.cmake` only lists `mixer.c`).

The order matters: a part uses what the parts before it define. A new part
goes into `mixer.c` in its place, after what it uses. Each part includes
`common.h` (with `#pragma once`), so that it can be read and opened in an
editor by itself.

## Parts, in the order of mixer.c

| file | what it has |
|------|-------------|
| `common.h` | what the parts share: `AudioClip`, `Voice`, the PCM stream and the state of the mixer |
| `clip.c` | the clips: the sounds loaded from a file, from memory or from PCM (short ones decoded whole), and how many use them |
| `decode.c` | decoding and mixing each voice (with its pan, speed and loop), in the SDL audio callback |
| `voice.c` | the voices: play, set (volume, pan, speed, loop), stop and the time of each one |
| `device.c` | the audio device: opening it (or going on without one, by the clock), each frame (`mixer_update`) and closing it |
| `stream.c` | the PCM stream (the audio of a video): a queue that the callback mixes, and its time |
