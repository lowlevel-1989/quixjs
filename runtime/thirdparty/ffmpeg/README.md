# ffmpeg

English | [Español](README.es.md)

A piece of FFmpeg 6.1.2 (https://ffmpeg.org/releases/ffmpeg-6.1.2.tar.xz):
only what decodes H.264 and AAC, the video and the audio of MP4 files.
quixjs uses it in `runtime/video/video.c` for `<video>` (the MP4 videos of
Felvidék); minimp4 (`../minimp4`) reads the container.

It is built into the binary with the rest of quixjs (`ffmpeg.cmake`), with
video (`VIDEO=1`, the default when libvpx and libopus are installed). It
adds about 2.3 MB to the stripped binary of PortMaster.

| folder | what it is |
|--|--|
| `libavcodec/`, `libavutil/`, `compat/` | the sources of FFmpeg that those two decoders need, not modified |
| `gen/` | files that `configure` writes: the list of decoders (`h264`, `aac`) and versions; the same for every target |
| `config/<target>/config.h` | what `configure` found on each target (`x86_64`, `aarch64`, `android-arm64`, `android-x86_64`), with the paths of the machine removed |
| `ffmpeg.cmake` | the list of sources (all targets, and the assembler of x86_64 and the NEON of aarch64) and the options to compile them |
| `VERSION`, `LICENSE` | the version and the license (LGPL 2.1 or later) |

## How it was made

`configure` of FFmpeg 6.1.2 with only those two decoders, on each target:

```
./configure --disable-everything --disable-programs --disable-doc \
  --disable-debug --disable-autodetect --disable-network --disable-avdevice \
  --disable-avformat --disable-swscale --disable-swresample \
  --disable-avfilter --disable-postproc --enable-decoder=h264,aac \
  --enable-static --disable-shared --disable-stripping --enable-pic
make
```

Then the dependencies of each object (the `.d` files) say which sources and
headers are used: only those were copied here, and the list of sources went
into `ffmpeg.cmake`. The rest of FFmpeg is not in the repo.

## License

FFmpeg is LGPL 2.1 or later (`LICENSE`); this piece has no GPL parts. The
PortMaster port carries the license in `licenses/ffmpeg.txt`.
