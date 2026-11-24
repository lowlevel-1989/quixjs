# CMakeLists.txt: como se compila quixjs, para todas las plataformas.
#
# Una plataforma por preset (CMakePresets.json): pc, portmaster y android.
# El Makefile de la raiz los usa (make pc, make portmaster, make android);
# cada parte esta en cmake/, un archivo por tema.

cmake_minimum_required(VERSION 3.20)
project(quixjs C ASM)

# las opciones (make ... NOMBRE=valor)
set(QUIX_PLATFORM "pc" CACHE STRING "la plataforma: pc, portmaster o android")
set(QUIX_VIDEO "AUTO" CACHE STRING "el video: AUTO (si estan libvpx y libopus), ON u OFF")
option(QUIX_TRACE "las medidas [trace] de los puntos criticos (docs/benchmark.md)" OFF)

# lo que sale de la compilacion va en build/; lo intermedio, en la carpeta
# del preset (obj/cmake/<preset>)
set(QUIX_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/build CACHE PATH "donde sale lo construido")

# las rutas de los archivos dentro del binario (__FILE__, la depuracion),
# relativas a la raiz: sin la carpeta de quien compila
add_compile_options(-ffile-prefix-map=${CMAKE_SOURCE_DIR}/=)

include(cmake/sources.cmake)
include(cmake/generated.cmake)

if(QUIX_PLATFORM STREQUAL "pc")
  include(cmake/pc.cmake)
  include(cmake/tests.cmake)
elseif(QUIX_PLATFORM STREQUAL "portmaster")
  include(cmake/portmaster.cmake)
elseif(QUIX_PLATFORM STREQUAL "android")
  include(cmake/android.cmake)
else()
  message(FATAL_ERROR "plataforma desconocida: ${QUIX_PLATFORM}")
endif()
