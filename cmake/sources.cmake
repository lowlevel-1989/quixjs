# cmake/sources.cmake: las fuentes de quixjs, por carpeta de runtime/.
#
# Deja QUIX_SOURCES (lo que se compila en todas las plataformas) y
# QUIX_SOURCES_<SO> (lo de cada sistema: linux, android). Un archivo indice
# (core/main.c, binding/webgl.c...) va solo: sus partes las incluye el.

set(QUIX_SOURCES "")

# js/: el motor de JS
list(APPEND QUIX_SOURCES
  runtime/js/jsc.c
)

# binding/: las APIs del navegador para el JS
list(APPEND QUIX_SOURCES
  runtime/binding/console.c
  runtime/binding/timer.c
  runtime/binding/events.c
  runtime/binding/navigator/navigator.c
  runtime/binding/document.c
  runtime/binding/window.c
  runtime/binding/image/image.c
  runtime/binding/image/decode.c
  runtime/binding/xhr.c
  runtime/binding/quix/quix.c
  runtime/binding/quix/input.c
  runtime/binding/quix/gpu.c
  runtime/binding/quix/achievements.c
  runtime/binding/quix/storage.c
  runtime/binding/quix/state.c
  runtime/binding/quix/fonts.c
  runtime/binding/quix/memory.c
  runtime/binding/quix/trace_saves.c
  runtime/binding/polyfill.c
  runtime/binding/script.c
  runtime/binding/module.c
  runtime/binding/nwjs.c
  runtime/binding/webview2.c
  runtime/binding/blob.c
  runtime/binding/audio/audio.c
  runtime/binding/webgl.c
  runtime/binding/canvas/2d/context2d.c
  runtime/binding/canvas/2d/state.c
  runtime/binding/canvas/2d/style.c
  runtime/binding/canvas/2d/transform.c
  runtime/binding/canvas/2d/path.c
  runtime/binding/canvas/2d/draw.c
  runtime/binding/canvas/2d/image.c
  runtime/binding/canvas/2d/text.c
  runtime/binding/canvas.c
  runtime/binding/quix/lan.c
  runtime/binding/video.c
)

# core/: el arranque, el bucle, la config y las herramientas
list(APPEND QUIX_SOURCES
  runtime/core/config/args.c
  runtime/core/debug/memstats.c
  runtime/core/scripts/load.c
  runtime/core/config/printer.c
  runtime/core/config/loader.c
  runtime/core/scripts/run.c
  runtime/core/timer.c
  runtime/core/keymap.c
  runtime/core/url/url.c
  runtime/core/url/read.c
  runtime/core/url/fs.c
  runtime/core/sandbox.c
  runtime/core/worker.c
  runtime/core/config/gen_config.c
  runtime/core/debug/input_script.c
  runtime/core/debug/debug_server/debug_server.c
  runtime/core/debug/debug_server/commands.c
  runtime/core/debug/debug_server/measure.c
  runtime/core/debug/profiler.c
  runtime/core/watchdog.c
  runtime/core/dispatch.c
  runtime/core/save.c
  runtime/core/main.c
)

# gfx/: el dibujo: por software (soft/) y en la GPU (gl/)
list(APPEND QUIX_SOURCES
  runtime/gfx/soft/surface.c
  runtime/gfx/soft/color.c
  runtime/gfx/soft/paint.c
  runtime/gfx/soft/raster.c
  runtime/gfx/soft/image_loader.c
  runtime/gfx/soft/image_scale.c
  runtime/gfx/soft/font.c
  runtime/gfx/gl/gl.c
  runtime/gfx/gl/driver.c
  runtime/gfx/gl/shader_cache.c
  runtime/gfx/gl/gpu2d.c
  runtime/gfx/gl/state.c
)
file(GLOB _found CONFIGURE_DEPENDS runtime/gfx/gl/drivers/*.c)
list(APPEND QUIX_SOURCES ${_found})

# audio/: el audio
list(APPEND QUIX_SOURCES
  runtime/audio/mixer.c
  runtime/audio/vorbis.c
  runtime/audio/mp3.c
  runtime/audio/wav.c
  runtime/audio/webm.c
  runtime/audio/decoder.c
)

# backend/: la ventana y la entrada
list(APPEND QUIX_SOURCES
  runtime/backend/backend.c
  runtime/backend/toast.c
  runtime/backend/dummy/backend.c
  runtime/backend/sdl.c
  runtime/backend/sdl/mapper.c
  runtime/backend/sdl/keypad.c
)

# launcher/: el launcher
list(APPEND QUIX_SOURCES
  runtime/launcher/ui/ui.c
  runtime/launcher/ui/draw.c
  runtime/launcher/ui/input.c
  runtime/launcher/launcher.c
  runtime/launcher/screens.c
  runtime/launcher/controls.c
  runtime/launcher/rom.c
  runtime/launcher/settings.c
)

# transfer/: Transfer Save
list(APPEND QUIX_SOURCES
  runtime/transfer/identity.c
  runtime/transfer/net.c
  runtime/transfer/saves/saves.c
  runtime/transfer/saves/pack.c
  runtime/transfer/saves/backup.c
  runtime/transfer/sender.c
  runtime/transfer/receiver.c
  runtime/transfer/transfer.c
)

# lan/: la red local para juegos
list(APPEND QUIX_SOURCES
  runtime/lan/lan.c
)

# game/: lo que quixjs agrega al juego: hooks, simbolos, logros, trucos
list(APPEND QUIX_SOURCES
  runtime/game/achievements/achievements.c
  runtime/game/achievements/define.c
  runtime/game/achievements/store.c
  runtime/game/achievements/check.c
  runtime/game/symbols/symbols.c
  runtime/game/cheats/cheats.c
  runtime/game/cheats/define.c
  runtime/game/cheats/playthrough.c
  runtime/game/cheats/apply.c
  runtime/game/hooks/hooks.c
  runtime/game/hooks/define.c
  runtime/game/hooks/mods.c
  runtime/game/hooks/api.c
  runtime/game/hooks/install.c
)

# zip/: leer zips
list(APPEND QUIX_SOURCES
  runtime/zip/zip.c
  runtime/zip/convert.c
  runtime/zip/asar.c
)

# rom/: la rom del juego
list(APPEND QUIX_SOURCES
  runtime/rom/rom.c
  runtime/rom/zip.c
  runtime/rom/dir.c
)

# engines/: las partes en C de los hooks de motor
list(APPEND QUIX_SOURCES
  runtime/engines/api.c
)
file(GLOB _found CONFIGURE_DEPENDS runtime/engines/*/natives.c)
list(APPEND QUIX_SOURCES ${_found})
file(GLOB _found CONFIGURE_DEPENDS runtime/engines/*/natives/*.c)
list(APPEND QUIX_SOURCES ${_found})

# video/: el video
list(APPEND QUIX_SOURCES
  runtime/video/video.c
  runtime/video/webm.c
  runtime/video/mp4.c
)

# platform/: lo de cada sistema operativo
set(QUIX_SOURCES_LINUX runtime/platform/linux/platform.c)
set(QUIX_SOURCES_ANDROID
  runtime/platform/android/platform.c
  runtime/platform/android/game.c
  runtime/platform/android/menu.c
  runtime/platform/android/transfer.c
  runtime/platform/android/pad.c
)

# tomlc17: lee los toml
list(APPEND QUIX_SOURCES runtime/thirdparty/tomlc17/tomlc17.c)
