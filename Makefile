# Makefile for quixjs runtime
TARGET = quixjs

# el primer objetivo es all (la regla de los shaders va antes)
.DEFAULT_GOAL := all

CC = gcc

# El motor de JS: JavaScriptCore (WebKit), estatico. En el PC sale de
# tools/build-jsc.sh host (obj/deps/jsc-build/ARCH-host/jsc del PC) y usa el ICU del
# sistema; para PortMaster, portmaster/portmaster.mk usa el de cada arquitectura.
JSC_BUILD ?= obj/deps/jsc-build/$(shell uname -m)-host/jsc
JSC_CFLAGS = -I$(JSC_BUILD)/JavaScriptCore/Headers
JSC_LIBS = $(JSC_BUILD)/lib/libJavaScriptCore.a $(JSC_BUILD)/lib/libWTF.a $(JSC_BUILD)/lib/libbmalloc.a \
           $(shell pkg-config --libs icu-i18n icu-uc) -lstdc++ -latomic -ldl

CFLAGS = -g -O2 -D_GNU_SOURCE
CFLAGS += -Iruntime -Iruntime/thirdparty -Iruntime/thirdparty/tomlc17 -Iobj/gen $(JSC_CFLAGS)
LDFLAGS = -lm -lpthread -lz -lSDL2_ttf -lSDL2_image -lSDL2 $(JSC_LIBS)

# Video: WebM (nestegg + libvpx + libopus, si estan instaladas; o VIDEO=1/0)
# y MP4 (minimp4 + el H.264 y el AAC de FFmpeg, en runtime/thirdparty)
VIDEO ?= $(shell pkg-config --exists vpx opus && echo 1 || echo 0)
include runtime/thirdparty/ffmpeg/ffmpeg.mk
FFMPEG_ARCH ?= $(shell uname -m)
ifeq ($(VIDEO),1)
CFLAGS += -DQUIX_VIDEO $(shell pkg-config --cflags vpx opus) $(FFMPEG_INCLUDES)
LDFLAGS += $(shell pkg-config --libs vpx opus)
# el H.264 de FFmpeg en x86_64 usa ensamblador con direcciones fijas: no
# cabe en un ejecutable PIE, lo que hace por defecto el gcc de Ubuntu (el del
# CI: "relocation R_X86_64_32 ... can not be used when making a PIE"). Como
# el de Fedora: un ejecutable normal
ifeq ($(FFMPEG_ARCH),x86_64)
CFLAGS += -fno-pie
LDFLAGS += -no-pie
endif
CORE_SRCS_VIDEO = runtime/thirdparty/nestegg/nestegg.c $(FFMPEG_SRCS) $(FFMPEG_SRCS_$(FFMPEG_ARCH))
endif

# PLATFORM=portmaster construye el port de PortMaster (portmaster/portmaster.mk)
PLATFORM ?= linux
# el sistema operativo (runtime/platform/<PLATFORM_OS>): linux en el PC y en las portatiles
PLATFORM_OS ?= linux

# Main source files
CORE_SRCS = runtime/js/jsc.c \
						runtime/binding/console.c \
						runtime/core/args.c \
						runtime/core/memstats.c \
						runtime/core/compiler.c \
						runtime/core/printer.c \
						runtime/core/config_loader.c \
						runtime/core/runner.c \
						runtime/core/timer.c \
						runtime/core/keymap.c \
						runtime/core/url.c \
						runtime/core/sandbox.c \
						runtime/core/worker.c \
						runtime/core/gen_config.c \
						runtime/core/input_script.c \
						runtime/core/debug_server.c \
						runtime/gfx/surface.c \
						runtime/gfx/color.c \
						runtime/gfx/paint.c \
						runtime/gfx/raster.c \
						runtime/gfx/image_loader.c \
						runtime/gfx/image_scale.c \
						runtime/gfx/font.c \
						runtime/gfx/gl.c \
						runtime/gfx/driver.c \
						runtime/gfx/shader_cache.c \
						$(wildcard runtime/gfx/drivers/*.c) \
						runtime/binding/timer.c \
						runtime/binding/events.c \
						runtime/binding/navigator.c \
						runtime/binding/document.c \
						runtime/binding/window.c \
						runtime/binding/image.c \
						runtime/binding/xhr.c \
						runtime/binding/quix.c \
						runtime/binding/polyfill.c \
						runtime/binding/script.c \
						runtime/binding/nwjs.c \
						runtime/binding/blob.c \
						runtime/binding/audio.c \
						runtime/binding/webgl.c \
						runtime/audio/mixer.c \
						runtime/audio/vorbis.c \
						runtime/audio/mp3.c \
						runtime/audio/wav.c \
						runtime/audio/decoder.c \
						runtime/binding/canvas/2d/context2d.c \
						runtime/binding/canvas/2d/state.c \
						runtime/binding/canvas/2d/style.c \
						runtime/binding/canvas/2d/transform.c \
						runtime/binding/canvas/2d/path.c \
						runtime/binding/canvas/2d/draw.c \
						runtime/binding/canvas/2d/image.c \
						runtime/binding/canvas/2d/text.c \
						runtime/binding/canvas.c \
						runtime/backend/backend.c \
						runtime/backend/toast.c \
						runtime/backend/dummy/backend.c \
						runtime/backend/sdl.c \
						runtime/backend/sdl/mapper.c \
						runtime/backend/sdl/keypad.c \
						runtime/launcher/ui.c \
						runtime/launcher/launcher.c \
						runtime/launcher/rom.c \
						runtime/launcher/settings.c \
						runtime/transfer/identity.c \
						runtime/transfer/net.c \
						runtime/transfer/saves.c \
						runtime/transfer/sender.c \
						runtime/transfer/receiver.c \
						runtime/transfer/transfer.c \
						runtime/lan/lan.c \
						runtime/binding/lan.c \
						runtime/achievements/achievements.c \
						runtime/zip/zip.c \
						runtime/zip/convert.c \
						runtime/zip/asar.c \
						runtime/rom/rom.c \
						runtime/rom/rom_zip.c \
						runtime/rom/rom_dir.c \
						runtime/symbols/symbols.c \
						runtime/cheats/cheats.c \
						runtime/hooks/hooks.c \
						runtime/core/profiler.c \
						runtime/engines/engine_api.c \
						$(wildcard runtime/engines/*/*.c) \
						runtime/core/watchdog.c \
						runtime/core/dispatch.c \
						runtime/core/save.c \
						runtime/video/video.c \
						runtime/video/video_webm.c \
						runtime/video/video_mp4.c \
						runtime/binding/video.c \
						runtime/gfx/gpu2d.c \
						runtime/gfx/gl_state.c \
						runtime/platform/$(PLATFORM_OS)/platform.c \
						runtime/core/main.c

# tomlc17 source
TOMLC_SRCS = runtime/thirdparty/tomlc17/tomlc17.c

SRCS = $(CORE_SRCS) $(TOMLC_SRCS) $(CORE_SRCS_VIDEO)

OBJS = $(SRCS:.c=.o)

# Los shaders (runtime/shaders/*.vert, *.frag) van dentro del binario
SHADERS = $(wildcard runtime/shaders/*.vert runtime/shaders/*.frag)
GEN_SHADERS = obj/gen/shaders.h
$(GEN_SHADERS): $(SHADERS) tools/embed_shaders.py
	python3 tools/embed_shaders.py runtime/shaders $@

# Los hooks de motor (runtime/engines/<motor>/*.js) tambien
ENGINE_FILES = $(wildcard runtime/engines/*/*.js runtime/engines/*/hooks.toml)
GEN_ENGINES = obj/gen/engines.h
$(GEN_ENGINES): $(ENGINE_FILES) tools/embed_engines.py
	python3 tools/embed_engines.py runtime/engines $@

# El JS de quixjs (runtime/**/*.js fuera de engines/) tambien va dentro del
# binario: cada .js se edita como JS y tools/embed_js.py lo convierte en una
# cadena de C al compilar (ver su cabecera: por que se hace asi)
GEN_POLYFILLS = obj/gen/polyfills.h
$(GEN_POLYFILLS): $(wildcard runtime/binding/polyfill/*.js) tools/embed_js.py
	python3 tools/embed_js.py $@ --table POLYFILLS runtime/binding/polyfill
GEN_NWJS = obj/gen/nwjs_parts.h
$(GEN_NWJS): $(wildcard runtime/binding/nwjs/*.js) tools/embed_js.py
	python3 tools/embed_js.py $@ --table NWJS_PARTS runtime/binding/nwjs
GEN_JS = obj/gen/js_webaudio.h obj/gen/js_gamepads.h obj/gen/js_profiler.h obj/gen/js_same_code.h
obj/gen/js_webaudio.h: runtime/binding/webaudio.js tools/embed_js.py
	python3 tools/embed_js.py $@ WEBAUDIO_JS $<
obj/gen/js_gamepads.h: runtime/binding/gamepads.js tools/embed_js.py
	python3 tools/embed_js.py $@ GAMEPADS_JS $<
obj/gen/js_profiler.h: runtime/core/profiler.js tools/embed_js.py
	python3 tools/embed_js.py $@ PROFILER_JS $<
obj/gen/js_same_code.h: runtime/engines/same-code.js tools/embed_js.py
	python3 tools/embed_js.py $@ SAME_CODE_JS $<

# La version de quixjs: la del archivo VERSION (QUIX_VERSION)
GEN_VERSION = obj/gen/version.h
$(GEN_VERSION): VERSION
	@mkdir -p $(dir $@)
	printf '// generado desde VERSION: no editar\n#define QUIX_VERSION "%s"\n' "$$(tr -d ' \n' < VERSION)" > $@

# make TRACE=1: las medidas [trace] de los puntos criticos (runtime/core/trace.h,
# docs/benchmark.md). La cabecera solo cambia si cambia TRACE: entonces se
# recompila todo lo nuestro, y sin TRACE no queda nada en el binario
TRACE ?= 0
GEN_TRACE = obj/gen/trace_config.h
$(GEN_TRACE): FORCE_TRACE
	@mkdir -p $(dir $@)
	@echo '#define QUIX_TRACE $(TRACE)' | cmp -s - $@ || echo '#define QUIX_TRACE $(TRACE)' > $@
.PHONY: FORCE_TRACE
FORCE_TRACE:

# Archivos partidos en carpetas (ver docs/build.md, "Index files"): X.c
# solo incluye, en orden, las partes de la carpeta X/ y se compila como un
# solo archivo, para que el compilador genere el mismo codigo que cuando era
# uno. Su .o se recompila si cambia una parte
INDEX_FILES = runtime/js/jsc runtime/binding/webgl runtime/backend/sdl runtime/core/main runtime/core/gen_config
$(foreach d,$(INDEX_FILES),$(eval $(d).o: $(wildcard $(d)/*.c)))

# Las cabeceras propias: si cambia una, se recompila lo nuestro
HEADERS = $(shell find runtime -name '*.h' -not -path 'runtime/thirdparty/*') $(GEN_SHADERS) $(GEN_ENGINES) $(GEN_POLYFILLS) $(GEN_NWJS) $(GEN_JS) $(GEN_VERSION) $(GEN_TRACE)

ifeq ($(PLATFORM),portmaster)
include portmaster/portmaster.mk
else ifeq ($(PLATFORM),android)
# PLATFORM=android construye quixjs.apk (android/android.mk)
include android/android.mk
else
.PHONY: all clean test release bench

all: $(TARGET)

# Los archivos de una release de GitHub, en release/: quixjs solo para
# x86_64, aarch64 y armhf, y el zip del port (PORT=fearandhunger)
release:
	tools/release.sh

# JavaScriptCore: si no esta, tools/build-jsc.sh host lo descarga (WebKit, solo
# lo de JSC) y lo compila (tarda; una vez)
JSC_LIB = $(JSC_BUILD)/lib/libJavaScriptCore.a
$(JSC_LIB):
	tools/build-jsc.sh host

$(TARGET): $(OBJS) $(JSC_LIB)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

# lo nuestro necesita las cabeceras de JavaScriptCore
$(CORE_SRCS:.c=.o): $(HEADERS) | $(JSC_LIB)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# FFmpeg: con sus opciones y el config.h del PC (no con las nuestras)
$(filter $(FFMPEG_DIR)/%,$(OBJS)): CFLAGS = $(FFMPEG_CFLAGS) -I$(FFMPEG_DIR)/config/$(FFMPEG_ARCH)

# Ejecuta los juegos de games/tests en modo test (sin ventana)
test: $(TARGET)
	./tests/run.sh

# h5bench (tools/h5bench.sh): benchmark de JavaScript, WebGL, canvas 2d y
# memoria, con ventana; los resultados en results/h5bench/
bench: $(TARGET)
	tools/h5bench.sh

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf cache
endif
