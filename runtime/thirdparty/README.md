# thirdparty

English | [Español](README.es.md)

Code from other projects, built with quixjs.

| folder | library | what quixjs uses it for |
|--------|---------|-------------------------|
| `tomlc17/` | tomlc17 (https://github.com/cktan/tomlc17) | reads every toml: the config of the game (`core/config_loader.c`), the launcher options (`launcher/settings.c`), `[hooks]`, `[symbols]`, achievements and cheats |
| `stb/stb_vorbis.c` | stb_vorbis (https://github.com/nothings/stb) | decodes the ogg vorbis of the audio while it plays (`audio/vorbis.c`, `audio/mixer.c`) |
| `minimp3/` | minimp3 (https://github.com/lieff/minimp3, CC0), commit [1428237](https://github.com/lieff/minimp3/tree/142823796ab8efa55e1bf11406028f3be77ecceb) | decodes the mp3 of the audio while it plays (`audio/mp3.c`) |
| `stb/stb_easy_font.h` | stb_easy_font (https://github.com/nothings/stb) | the text of the launcher menu and of the notices on screen (`launcher/ui.c`, `backend/toast.c`) |
| `nestegg/` | nestegg from Mozilla (https://github.com/mozilla/nestegg) | reads WebM files for `<video>` (`video/video.c`); built only with video |
| `minimp4/` | minimp4 (https://github.com/lieff/minimp4, CC0) | reads MP4 files for `<video>` (`video/video.c`); built only with video |
| `ffmpeg/` | a piece of FFmpeg 6.1.2 (https://ffmpeg.org, LGPL 2.1+): the H.264 and AAC decoders | decodes the MP4 videos (`video/video.c`); built only with video ([README](ffmpeg/README.md)) |
| `webkit/` | JavaScriptCore, WTF and bmalloc from WebKit (https://github.com/WebKit/WebKit) | runs the JavaScript of the game; built apart by `tools/build-jsc.sh` ([README](webkit/README.md)) |
| `webkit/unifdef/` | unifdef (https://dotat.at/prog/unifdef/) | a tool of the WebKit build, not part of the binary |

tomlc17, stb, minimp3, nestegg, minimp4 and the FFmpeg sources are not modified. The changes in `webkit/` are listed
in its [README](webkit/README.md).

## Libraries of the system

These are not in the repo: they are linked from the system (the port carries
some of them in `libs/`).

| library | what quixjs uses it for |
|---------|-------------------------|
| SDL2 | window, input, OpenGL ES context and audio output |
| SDL2_ttf (with freetype) | the fonts of the game (`gfx/font.c`) |
| SDL2_image (with libpng) | decodes the images (`gfx/image_loader.c`) |
| ICU | text and dates of JavaScriptCore (`Intl`, `toLocaleString`...) |
| libvpx, libopus | VP8/VP9 video and Opus audio of WebM; only with video |
