# cmake/android/native.cmake: la parte nativa de la app de Android: SDL2,
# SDL2_ttf, SDL2_image y libquixjs.so (quixjs como libreria: SDLActivity la
# carga y llama a su main).
#
# Deja los targets quixjs, SDL2, SDL2_ttf y SDL2_image (las .so del APK, que
# arma cmake/android/apk.cmake) y QUIX_SDL_DIR (las fuentes de SDL2, para su Java).
# Usa el toolchain cmake/toolchains/android.cmake, QUIX_SOURCES,
# QUIX_SOURCES_ANDROID, QUIX_GENERATED, FFMPEG_* y JavaScriptCore (cmake/jsc.cmake).

enable_language(CXX)
include(FetchContent)

# JavaScriptCore e ICU estaticos, para la ABI
if(QUIX_ANDROID_ABI STREQUAL "x86_64")
  set(_arch x86_64)
else()
  set(_arch arm64)
endif()
include(cmake/jsc.cmake)

# SDL2, SDL2_ttf y SDL2_image: se descargan una vez (comprobadas con su sha256)
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/obj/deps/android)
set(_sdl https://github.com/libsdl-org)
FetchContent_Declare(SDL2 URL ${_sdl}/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz
  URL_HASH SHA256=5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165)
FetchContent_Declare(SDL2_ttf URL ${_sdl}/SDL_ttf/releases/download/release-2.24.0/SDL2_ttf-2.24.0.tar.gz
  URL_HASH SHA256=0b2bf1e7b6568adbdbc9bb924643f79d9dedafe061fa1ed687d1d9ac4e453bfd)
FetchContent_Declare(SDL2_image URL ${_sdl}/SDL_image/releases/download/release-2.8.8/SDL2_image-2.8.8.tar.gz
  URL_HASH SHA256=2213b56fdaff2220d0e38c8e420cbe1a83c87374190cba8c70af2156097ce30a)
# SDL2_ttf con su freetype; SDL2_image solo con stb (PNG y JPG)
set(SDL2TTF_VENDORED ON CACHE BOOL "" FORCE)
set(SDL2TTF_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_VENDORED OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_BACKEND_STB ON CACHE BOOL "" FORCE)
set(SDL2IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
foreach(_format AVIF JXL TIF WEBP)
  set(SDL2IMAGE_${_format} OFF CACHE BOOL "" FORCE)
endforeach()
FetchContent_MakeAvailable(SDL2 SDL2_ttf SDL2_image)
set(QUIX_SDL_DIR ${sdl2_SOURCE_DIR})

# quixjs incluye <SDL2/SDL.h>, <SDL2/SDL_ttf.h>...: las cabeceras, en include/SDL2
set(_sdl_include ${CMAKE_BINARY_DIR}/include)
file(GLOB _sdl_headers ${sdl2_SOURCE_DIR}/include/*.h)
file(COPY ${_sdl_headers} ${sdl2_ttf_SOURCE_DIR}/SDL_ttf.h ${sdl2_image_SOURCE_DIR}/include/SDL_image.h
  DESTINATION ${_sdl_include}/SDL2)

# el video: solo MP4 (minimp4 y el pedazo de FFmpeg, con su config de
# Android). -fvisibility=hidden: sus simbolos quedan dentro de libquixjs.so
include(runtime/thirdparty/ffmpeg/ffmpeg.cmake)
if(_arch STREQUAL "arm64")
  set(_ffmpeg_arch aarch64)
else()
  set(_ffmpeg_arch x86_64)
endif()
add_library(ffmpeg STATIC ${FFMPEG_SOURCES} ${FFMPEG_SOURCES_${_ffmpeg_arch}})
target_compile_options(ffmpeg PRIVATE ${FFMPEG_OPTIONS} -fvisibility=hidden)
target_include_directories(ffmpeg PRIVATE ${FFMPEG_INCLUDES} ${FFMPEG_DIR}/config/android-${_arch})

add_library(quixjs SHARED ${QUIX_SOURCES} ${QUIX_SOURCES_ANDROID} ${QUIX_GENERATED})
target_compile_options(quixjs PRIVATE -O2 -w)
target_compile_definitions(quixjs PRIVATE _GNU_SOURCE QUIX_VIDEO QUIX_VIDEO_MP4)
target_include_directories(quixjs PRIVATE runtime runtime/thirdparty runtime/thirdparty/tomlc17
  ${QUIX_GEN_DIR} ${_sdl_include} ${QUIX_JSC_DIR}/include ${FFMPEG_INCLUDES})
# el main de quixjs: SDL_main (platform/android) lo llama en su thread
set_source_files_properties(runtime/core/main.c PROPERTIES COMPILE_DEFINITIONS "main=quixjs_main;SDL_MAIN_HANDLED")
target_link_libraries(quixjs PRIVATE ffmpeg SDL2::SDL2 SDL2_ttf::SDL2_ttf SDL2_image::SDL2_image
  ${QUIX_JSC_DIR}/lib/libJavaScriptCore.a ${QUIX_JSC_DIR}/lib/libWTF.a ${QUIX_JSC_DIR}/lib/libbmalloc.a
  ${QUIX_JSC_DIR}/lib/libicui18n.a ${QUIX_JSC_DIR}/lib/libicuuc.a ${QUIX_JSC_DIR}/lib/libicudata.a
  GLESv2 EGL log android m dl z)
target_link_options(quixjs PRIVATE -Wl,--gc-sections -Wl,--strip-all)
# JavaScriptCore es C++: enlazar como C++ mete la libc++ estatica del NDK
set_target_properties(quixjs PROPERTIES LINKER_LANGUAGE CXX)
add_dependencies(quixjs jsc)
