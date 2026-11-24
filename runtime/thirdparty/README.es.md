# thirdparty

[English](README.md) | Español

Codigo de otros proyectos, compilado con quixjs.

| carpeta | biblioteca | para que la usa quixjs |
|---------|------------|------------------------|
| `tomlc17/` | tomlc17 (https://github.com/cktan/tomlc17) | lee todos los toml: la configuracion del juego (`core/config/config_loader.c`), las opciones del launcher (`launcher/settings.c`), `[hooks]`, `[symbols]`, logros y trucos |
| `stb/stb_vorbis.c` | stb_vorbis (https://github.com/nothings/stb) | decodifica el ogg vorbis del audio mientras suena (`audio/vorbis.c`, `audio/mixer.c`) |
| `minimp3/` | minimp3 (https://github.com/lieff/minimp3, CC0), commit [1428237](https://github.com/lieff/minimp3/tree/142823796ab8efa55e1bf11406028f3be77ecceb) | decodifica el mp3 del audio mientras suena (`audio/mp3.c`) |
| `stb/stb_easy_font.h` | stb_easy_font (https://github.com/nothings/stb) | el texto del menu del launcher y de los avisos en pantalla (`launcher/ui/draw.c`, `backend/toast.c`) |
| `nestegg/` | nestegg de Mozilla (https://github.com/mozilla/nestegg) | lee archivos WebM para `<video>` (`video/video.c`); solo se compila con video |
| `minimp4/` | minimp4 (https://github.com/lieff/minimp4, CC0) | lee archivos MP4 para `<video>` (`video/video.c`); solo se compila con video |
| `ffmpeg/` | un pedazo de FFmpeg 6.1.2 (https://ffmpeg.org, LGPL 2.1+): los decodificadores H.264 y AAC | decodifica los videos MP4 (`video/video.c`); solo se compila con video ([README](ffmpeg/README.es.md)) |
| `webkit/` | JavaScriptCore, WTF y bmalloc de WebKit (https://github.com/WebKit/WebKit) | ejecuta el JavaScript del juego; se compila aparte con `cmake/jsc.cmake` ([README](webkit/README.es.md)) |
| `webkit/unifdef/` | unifdef (https://dotat.at/prog/unifdef/) | una herramienta de la compilacion de WebKit, no va en el binario |

tomlc17, stb, minimp3, nestegg, minimp4 y las fuentes de FFmpeg no estan modificados. Los cambios en `webkit/` estan en
su [README](webkit/README.es.md).

## Bibliotecas del sistema

No estan en el repo: se enlazan las del sistema (el port lleva algunas en
`libs/`).

| biblioteca | para que la usa quixjs |
|------------|------------------------|
| SDL2 | ventana, mandos, contexto OpenGL ES y salida de audio |
| SDL2_ttf (con freetype) | las fuentes del juego (`gfx/soft/font.c`) |
| SDL2_image (con libpng) | decodifica las imagenes (`gfx/soft/image_loader.c`) |
| ICU | texto y fechas de JavaScriptCore (`Intl`, `toLocaleString`...) |
| libvpx, libopus | video VP8/VP9 y audio Opus de WebM; solo con video |
