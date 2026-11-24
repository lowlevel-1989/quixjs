# cmake/portmaster.cmake: quixjs para las portatiles (PortMaster) y el zip
# de cada port.
#
# Deja el target quixjs (quixjs.aarch64), port-<nombre> para cada port de
# games/ con [portmaster] (sale en build/<nombre>.zip) y ports (todos). Usa
# el toolchain cmake/toolchains/portmaster.cmake (QUIX_SYSROOT),
# QUIX_SOURCES, QUIX_GENERATED, FFMPEG_* y JavaScriptCore (cmake/jsc.cmake).

# JavaScriptCore, ICU y libc++ estaticos, para aarch64
include(cmake/jsc.cmake)

# el video siempre: WebM (nestegg; libvpx y libopus van en libs.aarch64/) y
# MP4 (el pedazo de FFmpeg, con su config de aarch64)
include(runtime/thirdparty/ffmpeg/ffmpeg.cmake)
add_library(ffmpeg OBJECT ${FFMPEG_SOURCES} ${FFMPEG_SOURCES_aarch64})
target_compile_options(ffmpeg PRIVATE ${FFMPEG_OPTIONS})
target_include_directories(ffmpeg PRIVATE ${FFMPEG_INCLUDES} ${FFMPEG_DIR}/config/aarch64)

add_executable(quixjs ${QUIX_SOURCES} ${QUIX_SOURCES_LINUX} ${QUIX_GENERATED}
  runtime/thirdparty/nestegg/nestegg.c $<TARGET_OBJECTS:ffmpeg>)
set_target_properties(quixjs PROPERTIES OUTPUT_NAME quixjs.aarch64 SKIP_BUILD_RPATH ON)
add_dependencies(quixjs jsc)
target_compile_options(quixjs PRIVATE -O2 -w)
target_compile_definitions(quixjs PRIVATE _GNU_SOURCE QUIX_VIDEO QUIX_HANDHELD)
target_include_directories(quixjs PRIVATE runtime runtime/thirdparty runtime/thirdparty/tomlc17
  ${QUIX_GEN_DIR} ${QUIX_SYSROOT}/usr/include/SDL2 ${QUIX_JSC_DIR}/include ${FFMPEG_INCLUDES})
target_link_directories(quixjs PRIVATE ${QUIX_JSC_DIR}/lib)
# JavaScriptCore, ICU y libc++ van dentro del binario; las demas, del sistema
# de la portatil o de libs.aarch64/ (rpath)
target_link_libraries(quixjs PRIVATE JavaScriptCore WTF bmalloc icui18n icuuc icudata
  -l:libc++.a -l:libc++abi.a vpx opus z SDL2_ttf SDL2_image SDL2 m pthread dl)
target_link_options(quixjs PRIVATE -Wl,--gc-sections -Wl,--strip-all -Wl,--allow-shlib-undefined
  -Wl,--disable-new-dtags "-Wl,-rpath,$ORIGIN/libs.aarch64" -static-libgcc)

# un zip por port: los de games/ con [portmaster] en su port.toml
file(GLOB _tomls CONFIGURE_DEPENDS games/*/port.toml)
add_custom_target(ports)
foreach(_toml ${_tomls})
  file(STRINGS ${_toml} _pm REGEX "^\\[portmaster\\]")
  if(NOT _pm)
    continue()
  endif()
  get_filename_component(_dir ${_toml} DIRECTORY)
  get_filename_component(_port ${_dir} NAME)
  add_custom_target(port-${_port}
    COMMAND python3 tools/port_zip.py ${_dir} $<TARGET_FILE:quixjs> ${QUIX_SYSROOT} ${QUIX_JSC_DIR}
      ${QUIX_OUTPUT_DIR}/${_port}.zip
    DEPENDS quixjs WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} VERBATIM)
  add_dependencies(ports port-${_port})
endforeach()
