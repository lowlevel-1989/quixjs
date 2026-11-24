# cmake/jsc/libcxx.cmake: libc++ y libc++abi de LLVM, estaticas, para las
# portatiles: el libstdc++ de Debian buster (GCC 8) es viejo para WebKit
# (C++23). El desenrollado de pila es el de libgcc (sin libunwind) y
# libc++.a lleva dentro libc++abi.
#
# Deja el target libcxx (instalado en QUIX_JSC_PREFIX) y lo que
# cmake/jsc/install.cmake copia a obj/deps/jsc-aarch64: QUIX_LIBCXX_A,
# QUIX_LIBCXXABI_A, QUIX_LIBCXX_LICENSES y QUIX_JSC_ABOUT.

ExternalProject_Add(libcxx
  SOURCE_DIR ${QUIX_LLVM_SRC}
  SOURCE_SUBDIR runtimes
  BINARY_DIR ${QUIX_JSC_WORK}/libcxx
  CMAKE_GENERATOR Ninja
  CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_SOURCE_DIR}/cmake/toolchains/portmaster.cmake
    -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX=${QUIX_JSC_PREFIX} -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DLLVM_ENABLE_RUNTIMES=libcxx$<SEMICOLON>libcxxabi
    -DLIBCXX_ENABLE_SHARED=OFF -DLIBCXXABI_ENABLE_SHARED=OFF
    -DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=ON -DLIBCXX_CXX_ABI=libcxxabi -DLIBCXXABI_USE_LLVM_UNWINDER=OFF
    -DLIBCXX_USE_COMPILER_RT=OFF -DLIBCXXABI_USE_COMPILER_RT=OFF -DLIBCXX_INSTALL_MODULES=OFF
    -DLIBCXX_INCLUDE_TESTS=OFF -DLIBCXX_INCLUDE_BENCHMARKS=OFF -DLIBCXXABI_INCLUDE_TESTS=OFF
  BUILD_COMMAND nice -n 10 ninja -C ${QUIX_JSC_WORK}/libcxx -j ${QUIX_JSC_JOBS}
  INSTALL_COMMAND ninja -C ${QUIX_JSC_WORK}/libcxx install)

set(QUIX_LIBCXX_A ${QUIX_JSC_PREFIX}/lib/libc++.a)
set(QUIX_LIBCXXABI_A ${QUIX_JSC_PREFIX}/lib/libc++abi.a)
set(QUIX_LIBCXX_LICENSES ${QUIX_LLVM_SRC}/libcxx/LICENSE.TXT=libc++.txt ${QUIX_LLVM_SRC}/libcxxabi/LICENSE.TXT=libc++abi.txt)
set(QUIX_JSC_ABOUT "libc++/libc++abi de LLVM 22.1.8, para aarch64-linux-gnu (Debian buster, glibc 2.28).")
