# video

English | [Español](README.es.md)

WebM video for the `<video>` element: the "Play Movie" of the events of RPG
Maker MV. It is made with libraries, not from scratch.

| library   | what it does |
|-----------|--------------|
| nestegg   | reads the WebM container (`thirdparty/nestegg`, built with quixjs) |
| libvpx    | decodes the VP8 and VP9 video |
| libopus   | decodes the Opus audio (the audio of the WebM of RPG Maker) |

| file      | what it does |
|-----------|--------------|
| `video.h` | open, play, pause, volume, update, ended, time, decoded frames |
| `video.c` | reads the packets with nestegg, decodes them with libvpx and libopus, converts the picture to RGBA; without video support every function is a stub |

The JS side is `binding/video.c` (`__quixVideo`) and the `<video>` element of
`binding/polyfill.c` ([binding](../binding/README.md)).

## How it plays

- It takes the first VP8 or VP9 video track and the first Opus audio track.
  A file without VP8 or VP9 video does not open. Other audio tracks (Vorbis)
  are not decoded: the video plays without sound.
- The audio is decoded at 48 kHz, about 0.5 s ahead, and goes to the mixer as
  a stream (`mixer_stream_*` in [audio](../audio/README.md)). Mono is copied
  to both channels and the pre-skip of the Opus header is dropped.
- The picture follows the clock of the audio (what the mixer has played).
  Without audio, or without an audio device, it follows the clock of the
  runtime.
- The picture is decoded in a thread of its own, which also converts it from
  YUV (I420, I422, I444; BT.601, limited range) to RGBA on the CPU with
  integers. It keeps up to 3 pictures ready ahead; each frame of the game
  shows the one whose time has come.
- A video ends when there are no packets left and the audio has been
  heard.

## On screen

`binding/video.c` keeps up to 4 videos. With the sdl backend the first
visible video that has not ended is drawn as a layer on top of every canvas.
The `<video>` element says if it is visible from its `style` (`display`,
`visibility`, `opacity`). The dummy backend and the screenshots do not show
it.

## Build

The `Makefile` builds with video when `pkg-config` finds `vpx` and `opus`
(`VIDEO=1`). `make VIDEO=0` builds without it: `video_open` returns NULL and
the `<video>` element is skipped (`loadeddata` and `ended` at once, so the
game goes on).

```
# Debian / Ubuntu
sudo apt-get install libvpx-dev libopus-dev
```

The PortMaster build always has video and carries `libvpx.so.5` and
`libopus.so.0` in `libs.aarch64/` and `libs.armhf/`, with the licenses of libvpx5, libopus0 and nestegg.

The test is `games/tests/media/video`: `clip.webm` (64x48, 1 s, 10 frames,
made with ffmpeg from `testsrc` and a `sine` tone) plays to the end, and a
missing file is skipped.
