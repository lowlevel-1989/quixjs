# cmake/pc.cmake: quixjs para el PC (Linux), con las librerias del sistema.
#
# Deja el target quixjs (sale en build/quixjs). Usa QUIX_SOURCES,
# QUIX_SOURCES_LINUX (sources.cmake), QUIX_GENERATED (generated.cmake),
# FFMPEG_* (runtime/thirdparty/ffmpeg/ffmpeg.cmake) y JavaScriptCore
# (jsc.cmake).

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL REQUIRED sdl2 SDL2_ttf SDL2_image)
pkg_check_modules(ICU REQUIRED icu-i18n icu-uc)
find_package(ZLIB REQUIRED)

# el video: WebM si estan libvpx y libopus (QUIX_VIDEO=AUTO), y MP4 siempre
# que haya video
pkg_check_modules(VPX vpx opus)
if(QUIX_VIDEO STREQUAL "AUTO")
  set(QUIX_VIDEO ${VPX_FOUND})
endif()

# JavaScriptCore, estatico (jsc.cmake; make pc-jsc)
include(cmake/jsc.cmake)

add_executable(quixjs ${QUIX_SOURCES} ${QUIX_SOURCES_LINUX} ${QUIX_GENERATED})
set_target_properties(quixjs PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${QUIX_OUTPUT_DIR})
add_dependencies(quixjs jsc)
target_compile_options(quixjs PRIVATE -g -O2)
target_compile_definitions(quixjs PRIVATE _GNU_SOURCE)
target_include_directories(quixjs PRIVATE runtime runtime/thirdparty runtime/thirdparty/tomlc17
  ${QUIX_GEN_DIR} ${QUIX_JSC_DIR}/JavaScriptCore/Headers)
target_link_libraries(quixjs PRIVATE m pthread ZLIB::ZLIB ${SDL_LIBRARIES}
  ${QUIX_JSC_DIR}/lib/libJavaScriptCore.a ${QUIX_JSC_DIR}/lib/libWTF.a ${QUIX_JSC_DIR}/lib/libbmalloc.a
  ${ICU_LIBRARIES} stdc++ atomic dl)

if(QUIX_VIDEO)
  # el video: nestegg (WebM) y el pedazo de FFmpeg (MP4), con sus opciones
  include(runtime/thirdparty/ffmpeg/ffmpeg.cmake)
  add_library(ffmpeg OBJECT ${FFMPEG_SOURCES} ${FFMPEG_SOURCES_${CMAKE_SYSTEM_PROCESSOR}})
  target_compile_options(ffmpeg PRIVATE ${FFMPEG_OPTIONS})
  target_include_directories(ffmpeg PRIVATE ${FFMPEG_INCLUDES} ${FFMPEG_DIR}/config/${CMAKE_SYSTEM_PROCESSOR})
  target_sources(quixjs PRIVATE runtime/thirdparty/nestegg/nestegg.c $<TARGET_OBJECTS:ffmpeg>)
  target_compile_definitions(quixjs PRIVATE QUIX_VIDEO)
  target_include_directories(quixjs PRIVATE ${FFMPEG_INCLUDES} ${VPX_INCLUDE_DIRS})
  target_link_libraries(quixjs PRIVATE ${VPX_LIBRARIES})
  # el H.264 de FFmpeg en x86_64 usa ensamblador con direcciones fijas: no
  # cabe en un ejecutable PIE (lo que hace por defecto el gcc de Ubuntu)
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    target_compile_options(quixjs PRIVATE -fno-pie)
    target_link_options(quixjs PRIVATE -no-pie)
  endif()
endif()
