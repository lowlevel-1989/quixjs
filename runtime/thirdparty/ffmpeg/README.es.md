# ffmpeg

[English](README.md) | Español

Un pedazo de FFmpeg 6.1.2 (https://ffmpeg.org/releases/ffmpeg-6.1.2.tar.xz):
solo lo que decodifica H.264 y AAC, el vídeo y el audio de los MP4. quixjs
lo usa en `runtime/video/video.c` para `<video>` (los vídeos MP4 de
Felvidék); minimp4 (`../minimp4`) lee el contenedor.

Va dentro del binario con el resto de quixjs (`ffmpeg.cmake`), con vídeo
(`VIDEO=1`, lo normal si están libvpx y libopus). Suma unos 2,3 MB al
binario de PortMaster (sin símbolos).

| carpeta | qué es |
|--|--|
| `libavcodec/`, `libavutil/`, `compat/` | las fuentes de FFmpeg que necesitan esos dos decodificadores, sin cambios |
| `gen/` | archivos que escribe `configure`: la lista de decodificadores (`h264`, `aac`) y las versiones; iguales en todos los destinos |
| `config/<destino>/config.h` | lo que `configure` encontró en cada destino (`x86_64`, `aarch64`, `android-arm64`, `android-x86_64`), sin las rutas de la máquina |
| `ffmpeg.cmake` | la lista de fuentes (todas, y el ensamblador de x86_64 y el NEON de aarch64) y las opciones para compilarlas |
| `VERSION`, `LICENSE` | la versión y la licencia (LGPL 2.1 o posterior) |

## Cómo se hizo

`configure` de FFmpeg 6.1.2 con solo esos dos decodificadores, en cada
destino:

```
./configure --disable-everything --disable-programs --disable-doc \
  --disable-debug --disable-autodetect --disable-network --disable-avdevice \
  --disable-avformat --disable-swscale --disable-swresample \
  --disable-avfilter --disable-postproc --enable-decoder=h264,aac \
  --enable-static --disable-shared --disable-stripping --enable-pic
make
```

Después, las dependencias de cada objeto (los `.d`) dicen qué fuentes y
cabeceras se usan: solo esas se copiaron aquí, y la lista de fuentes pasó a
`ffmpeg.cmake`. El resto de FFmpeg no está en el repo.

## Licencia

FFmpeg es LGPL 2.1 o posterior (`LICENSE`); este pedazo no tiene partes
GPL. El port de PortMaster lleva la licencia en `licenses/ffmpeg.txt`.
