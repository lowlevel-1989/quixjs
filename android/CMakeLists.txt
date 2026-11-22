# android/CMakeLists.txt: la parte nativa de la app de Android, para
# arm64-v8a. La ejecuta android/android.mk (make PLATFORM=android) con el
# toolchain del NDK. Adaptado de tamatama (android/CMakeLists.txt, MIT):
# https://github.com/lowlevel-1989/tamatama/blob/780b7e723f3dbe8a58bc54ed571f1518dac9191a/android/CMakeLists.txt
#   libSDL2.so        SDL2, de android/deps
#   libSDL2_ttf.so    SDL2_ttf (con su freetype y harfbuzz)
#   libSDL2_image.so  SDL2_image (PNG y JPG con stb_image, sin mas librerias)
#   libquixjs.so      quixjs: las mismas fuentes que el Makefile (QUIX_SRCS),
#                     con runtime/platform/android y el video MP4 (FFmpeg
#                     para arm64, FFMPEG_SRCS de ffmpeg.mk; sin WebM: no
#                     lleva libvpx ni libopus); main pasa a
#                     ser quixjs_main y SDL entra por SDL_main
#                     (runtime/platform/android/platform.c). Y el JNI de
#                     las pantallas de Android (platform/android/launcher.c:
#                     el menu del juego y Transfer Save). JavaScriptCore,
#                     ICU y libc++ dentro (estaticos, de JSC_DIR)
cmake_minimum_required(VERSION 3.16)
project(quixjs C CXX ASM)

set(REPO ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(DEPS ${CMAKE_CURRENT_SOURCE_DIR}/deps/src)

# SDL2, SDL2_ttf y SDL2_image, compartidas: SDLActivity las carga por nombre
add_subdirectory(${DEPS}/SDL2 SDL2)
set(SDL2TTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDL2TTF_SAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${DEPS}/SDL2_ttf SDL2_ttf)
set(SDL2IMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
foreach(format AVIF JXL TIF WEBP)
  set(SDL2IMAGE_${format} OFF CACHE BOOL "" FORCE)
endforeach()
set(SDL2IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${DEPS}/SDL2_image SDL2_image)

# quixjs incluye <SDL2/SDL.h>, <SDL2/SDL_ttf.h>...: las cabeceras, en
# include/SDL2 del build
set(SDL_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include)
file(GLOB SDL_HEADERS ${DEPS}/SDL2/include/*.h)
file(COPY ${SDL_HEADERS} ${DEPS}/SDL2_ttf/SDL_ttf.h ${DEPS}/SDL2_image/include/SDL_image.h
     DESTINATION ${SDL_INCLUDE}/SDL2)

# FFmpeg (el H.264 y el AAC de los MP4, runtime/thirdparty/ffmpeg): las
# fuentes de ffmpeg.mk para arm64 (con NEON) y su config de Android, con sus
# opciones (las de FFMPEG_CFLAGS de ffmpeg.mk), en una libreria aparte
set(FFMPEG ${REPO}/runtime/thirdparty/ffmpeg)
separate_arguments(FFMPEG_SRCS)
set(FFMPEG_SOURCES "")
foreach(src ${FFMPEG_SRCS})
  list(APPEND FFMPEG_SOURCES ${REPO}/${src})
endforeach()
add_library(ffmpeg STATIC ${FFMPEG_SOURCES})
target_include_directories(ffmpeg PRIVATE ${FFMPEG} ${FFMPEG}/gen ${FFMPEG}/config/android-arm64)
target_compile_definitions(ffmpeg PRIVATE HAVE_AV_CONFIG_H _ISOC99_SOURCE _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE
  _POSIX_C_SOURCE=200112 _XOPEN_SOURCE=600)
# -fvisibility=hidden: sus simbolos quedan dentro de libquixjs.so (el
# ensamblador NEON los toma por cercanos, y en una .so eso pide que no se vean
# desde fuera)
target_compile_options(ffmpeg PRIVATE $<$<COMPILE_LANGUAGE:C>:-std=c11> -O3 -fomit-frame-pointer -fno-math-errno
  -fno-signed-zeros -fvisibility=hidden -w)

# las fuentes del Makefile (SRCS), con la plataforma android y el video solo MP4
separate_arguments(QUIX_SRCS)
set(SOURCES "")
foreach(src ${QUIX_SRCS})
  if(src MATCHES "runtime/platform/linux/" OR src MATCHES "nestegg" OR src MATCHES "thirdparty/ffmpeg/")
    continue()
  endif()
  list(APPEND SOURCES ${REPO}/${src})
endforeach()
list(APPEND SOURCES ${REPO}/runtime/platform/android/platform.c ${REPO}/runtime/platform/android/launcher.c)

add_library(quixjs SHARED ${SOURCES})
target_include_directories(quixjs PRIVATE ${REPO}/runtime ${REPO}/runtime/thirdparty
  ${REPO}/runtime/thirdparty/tomlc17 ${REPO}/obj/gen ${SDL_INCLUDE} ${JSC_DIR}/include)
target_compile_definitions(quixjs PRIVATE _GNU_SOURCE QUIX_VIDEO QUIX_VIDEO_MP4)
target_include_directories(quixjs PRIVATE ${FFMPEG} ${FFMPEG}/gen)
target_compile_options(quixjs PRIVATE -O2 -w)
# el main de quixjs: SDL_main (platform/android) lo llama en su thread.
# SDL_MAIN_HANDLED: que SDL.h no renombre este main a SDL_main
set_source_files_properties(${REPO}/runtime/core/main.c PROPERTIES COMPILE_DEFINITIONS "main=quixjs_main;SDL_MAIN_HANDLED")

target_link_libraries(quixjs PRIVATE ffmpeg SDL2::SDL2 SDL2_ttf::SDL2_ttf SDL2_image::SDL2_image
  ${JSC_DIR}/lib/libJavaScriptCore.a ${JSC_DIR}/lib/libWTF.a ${JSC_DIR}/lib/libbmalloc.a
  ${JSC_DIR}/lib/libicui18n.a ${JSC_DIR}/lib/libicuuc.a ${JSC_DIR}/lib/libicudata.a
  GLESv2 EGL log android m dl z)
target_link_options(quixjs PRIVATE -Wl,--gc-sections -Wl,--strip-all)
# JavaScriptCore es C++: enlazar como C++ mete la libc++ estatica del NDK
# (ANDROID_STL=c++_static)
set_target_properties(quixjs PROPERTIES LINKER_LANGUAGE CXX)
