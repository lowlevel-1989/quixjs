# cmake/toolchains/android.cmake: compilar para Android con el NDK.
#
# Busca el NDK (ANDROID_NDK, o el ultimo de ANDROID_HOME/ndk, por defecto
# ~/Android/Sdk) y usa su toolchain para QUIX_ANDROID_ABI (arm64-v8a, o
# x86_64 para el emulador del PC): Android 9 (28, el minimo de
# JavaScriptCore: aligned_alloc de bionic) y la libc++ estatica del NDK.

set(QUIX_ANDROID_ABI "arm64-v8a" CACHE STRING "arm64-v8a, o x86_64 para el emulador")
if(DEFINED ENV{ANDROID_HOME})
  set(QUIX_ANDROID_HOME $ENV{ANDROID_HOME})
else()
  set(QUIX_ANDROID_HOME $ENV{HOME}/Android/Sdk)
endif()
if(DEFINED ENV{ANDROID_NDK})
  set(_ndk $ENV{ANDROID_NDK})
else()
  file(GLOB _ndks ${QUIX_ANDROID_HOME}/ndk/*)
  list(SORT _ndks COMPARE NATURAL)
  list(POP_BACK _ndks _ndk)
endif()
if(NOT EXISTS "${_ndk}/build/cmake/android.toolchain.cmake")
  message(FATAL_ERROR "no hay NDK en ${QUIX_ANDROID_HOME}/ndk: instala uno con sdkmanager (o ANDROID_NDK=...)")
endif()

set(ANDROID_ABI ${QUIX_ANDROID_ABI})
set(ANDROID_PLATFORM android-28)
set(ANDROID_STL c++_static)
include(${_ndk}/build/cmake/android.toolchain.cmake)
