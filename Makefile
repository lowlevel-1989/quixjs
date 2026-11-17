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
LDFLAGS = -lm -lpthread -lSDL2_ttf -lSDL2_image -lSDL2 $(JSC_LIBS)

# Video WebM (nestegg + libvpx + libopus): si estan instaladas (o VIDEO=1/0)
VIDEO ?= $(shell pkg-config --exists vpx opus && echo 1 || echo 0)
ifeq ($(VIDEO),1)
CFLAGS += -DQUIX_VIDEO $(shell pkg-config --cflags vpx opus)
LDFLAGS += $(shell pkg-config --libs vpx opus)
CORE_SRCS_VIDEO = runtime/thirdparty/nestegg/nestegg.c
endif

# PLATFORM=portmaster construye el port de PortMaster (portmaster/portmaster.mk)
PLATFORM ?= linux
# el sistema operativo (runtime/platform/<PLATFORM_OS>): linux en el PC y en las portatiles
PLATFORM_OS ?= linux

# Main source files
CORE_SRCS = runtime/js/js_jsc.c \
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
						runtime/core/worker.c \
						runtime/core/init.c \
						runtime/core/input_script.c \
						runtime/core/debug_server.c \
						runtime/gfx/surface.c \
						runtime/gfx/color.c \
						runtime/gfx/paint.c \
						runtime/gfx/raster.c \
						runtime/gfx/image_loader.c \
						runtime/gfx/font.c \
						runtime/gfx/gl.c \
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
						runtime/backend/sdl/backend.c \
						runtime/backend/sdl/mapper.c \
						runtime/launcher/ui.c \
						runtime/launcher/launcher.c \
						runtime/launcher/settings.c \
						runtime/transfer/identity.c \
						runtime/transfer/net.c \
						runtime/transfer/saves.c \
						runtime/transfer/sender.c \
						runtime/transfer/receiver.c \
						runtime/transfer/transfer.c \
						runtime/achievements/achievements.c \
						runtime/symbols/symbols.c \
						runtime/cheats/cheats.c \
						runtime/hooks/hooks.c \
						runtime/core/profiler.c \
						runtime/engines/engine_api.c \
						$(wildcard runtime/engines/*/*.c) \
						runtime/core/watchdog.c \
						runtime/video/video.c \
						runtime/binding/video.c \
						runtime/gfx/gpu2d.c \
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

# Las cabeceras propias: si cambia una, se recompila lo nuestro
HEADERS = $(shell find runtime -name '*.h' -not -path 'runtime/thirdparty/*') $(GEN_SHADERS) $(GEN_ENGINES)

ifeq ($(PLATFORM),portmaster)
include portmaster/portmaster.mk
else
.PHONY: all clean test release

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

# Ejecuta los juegos de games/tests en modo test (sin ventana)
test: $(TARGET)
	./tests/run.sh

clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf cache
endif
