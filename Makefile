# Makefile for quixjs runtime
TARGET = quixjs

CC = gcc

# quickjs flags
CFLAGS = -g -D_GNU_SOURCE -DCONFIG_VERSION=\"2025-04-26\"
CFLAGS += -Iruntime -Iruntime/thirdparty/quickjs -Iruntime/thirdparty/tomlc17
LDFLAGS = -lm -lSDL2_image -lSDL2

# Main source files
CORE_SRCS = runtime/core/args.c \
						runtime/core/compiler.c \
						runtime/core/printer.c \
						runtime/core/config_loader.c \
						runtime/core/runner.c \
						runtime/core/hash.c \
						runtime/core/timer.c \
						runtime/binding/timer.c \
						runtime/binding/navigator.c \
						runtime/binding/document.c \
						runtime/binding/window.c \
						runtime/binding/canvas/2d/draw.c \
						runtime/binding/canvas/2d/style.c \
						runtime/binding/canvas/2d/context2d.c \
						runtime/binding/canvas.c \
						runtime/backend/backend.c \
						runtime/backend/dummy/backend.c \
						runtime/backend/sdl/backend.c \
						runtime/core/main.c

# QuickJS sources (adjust as needed)
QUICKJS_SRCS = \
	runtime/thirdparty/quickjs/cutils.c \
	runtime/thirdparty/quickjs/dtoa.c \
	runtime/thirdparty/quickjs/libregexp.c \
	runtime/thirdparty/quickjs/libunicode.c \
	runtime/thirdparty/quickjs/quickjs-libc.c \
	runtime/thirdparty/quickjs/quickjs.c

# tomlc17 source
TOMLC_SRCS = runtime/thirdparty/tomlc17/tomlc17.c

SRCS = $(CORE_SRCS) $(QUICKJS_SRCS) $(TOMLC_SRCS)

OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
