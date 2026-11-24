# audio

English | [Español](README.es.md)

The audio mixer, made for handhelds. `binding/audio/audio.c` keeps the WebAudio
graph in JS (source -> gain -> panner -> ...) and, every frame, passes the
volume, pan and speed of each voice to this mixer, which mixes them in the SDL
audio thread.

| file        | what it does |
|-------------|--------------|
| `mixer.h/.c` | clips (the compressed file), voices (play, set, stop, loop), the PCM stream of a video, mixing in the SDL audio callback; index file of `mixer/` ([README](mixer/README.md)) |
| `decoder.h/.c` | a sound from memory without knowing its format: picks it by its first bytes and converts to 1 or 2 channels |
| `vorbis.c`  | ogg vorbis, with stb_vorbis (`thirdparty/stb/`) built here, reading from memory only; the LOOPSTART / LOOPLENGTH comments |
| `mp3.c`     | mp3, with minimp3 (`thirdparty/minimp3/`) |
| `wav.c`     | wav: PCM of 8, 16, 24 or 32 bits, or float |

## Streaming

An `AudioClip` is the compressed ogg vorbis that `decodeAudioData` returns; it
is not decoded whole. Each playing voice has its own decoder and position, and
decodes while it plays. A 3 minute song takes its ~3 MB, not ~30 MB of PCM.
A long one loaded from its file (not encrypted) is not even copied: it is
mapped (mmap), so its pages belong to the file and the kernel can drop them
when memory is short (Gunfield keeps every song it has played: 26 songs,
91 MB). A short one (up to 3 s, sound effects) is decoded whole when loaded.

## The stream of a video

`mixer_stream_*` is a stream of PCM already decoded: the audio of a `<video>`
([video](../video/README.md)). The main thread fills it and the audio thread
mixes it with the voices. There is one at a time, with room for 2 s of
stereo at 48 kHz; what does not fit is dropped. `mixer_stream_time()` is the
time already played, the clock that the picture of the video follows.
Without an audio device the stream is "played" at the pace of the clock of
the runtime.

## Without audio device

With the dummy backend (tests) there is no device: the voices advance with the
clock of the runtime without decoding, so `onended` and loops work the same.
While a voice is playing the main loop keeps advancing frame by frame.
