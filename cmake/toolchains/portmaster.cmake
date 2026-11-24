# cmake/toolchains/portmaster.cmake: compilar para las portatiles
# (PortMaster): aarch64, con clang y lld, contra Debian buster.
#
# Deja QUIX_SYSROOT (el Debian de tools/sysroot.py: se descarga la primera
# vez, antes de que CMake pruebe el compilador) y QUIX_DEBIAN_PACKAGES.

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(_root ${CMAKE_CURRENT_LIST_DIR}/../..)
set(QUIX_SYSROOT ${_root}/obj/deps/sysroot-arm64)

# lo que se enlaza (cabeceras y .so) y lo que se lleva en libs.aarch64/
set(QUIX_DEBIAN_PACKAGES libc6 libc6-dev linux-libc-dev libgcc1 libgcc-8-dev
  libsdl2-2.0-0 libsdl2-dev libsdl2-image-2.0-0 libsdl2-image-dev
  libsdl2-ttf-2.0-0 libsdl2-ttf-dev libfreetype6 libpng16-16 libjpeg62-turbo
  libtiff5 libwebp6 libjbig0 libzstd1 liblzma5 zlib1g zlib1g-dev fonts-dejavu-core
  libgles2-mesa-dev mesa-common-dev libegl1-mesa-dev
  libvpx5 libvpx-dev libopus0 libopus-dev)
if(NOT EXISTS ${QUIX_SYSROOT}/.ready)
  execute_process(COMMAND python3 ${_root}/tools/sysroot.py ${QUIX_SYSROOT} arm64 ${QUIX_DEBIAN_PACKAGES}
    COMMAND_ERROR_IS_FATAL ANY)
endif()

# clang y lld, para aarch64; afinado para los Cortex-A53/A35 de las
# portatiles (el mismo ARMv8: corre en todas); sin outline-atomics: el
# libgcc de GCC 8 no las trae
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)   # para lo que se compila aparte (JavaScriptCore)
set(CMAKE_ASM_COMPILER clang)
foreach(_lang C CXX ASM)
  set(CMAKE_${_lang}_COMPILER_TARGET aarch64-linux-gnu)
endforeach()
set(CMAKE_SYSROOT ${QUIX_SYSROOT})
set(_flags "--no-default-config -mtune=cortex-a53 -mno-outline-atomics")
set(CMAKE_C_FLAGS_INIT "${_flags}")
set(CMAKE_CXX_FLAGS_INIT "${_flags}")
set(CMAKE_ASM_FLAGS_INIT "${_flags}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# las .a (las de cmake/jsc), con el ar de binutils para aarch64
find_program(CMAKE_AR aarch64-linux-gnu-ar REQUIRED)
find_program(CMAKE_RANLIB aarch64-linux-gnu-ranlib REQUIRED)

# QUIX_LIBCXX=<carpeta>: C++ contra esa libc++ (estatica) y solo con lo que
# hay en ella; asi se compila JavaScriptCore (cmake/jsc.cmake)
if(QUIX_LIBCXX)
  string(APPEND CMAKE_CXX_FLAGS_INIT " -stdlib=libc++ -nostdinc++ -isystem ${QUIX_LIBCXX}/include/c++/v1")
  set(_ld "-fuse-ld=lld -stdlib=libc++ -L${QUIX_LIBCXX}/lib -static-libstdc++ -static-libgcc -pthread")
  set(CMAKE_EXE_LINKER_FLAGS_INIT "${_ld}")
  set(CMAKE_SHARED_LINKER_FLAGS_INIT "${_ld}")
  set(CMAKE_FIND_ROOT_PATH ${QUIX_LIBCXX})
  foreach(_mode LIBRARY INCLUDE PACKAGE)
    set(CMAKE_FIND_ROOT_PATH_MODE_${_mode} ONLY)
  endforeach()
endif()
