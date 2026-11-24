# cmake/jsc/sources.cmake: las fuentes de ICU y de libc++ (LLVM), para
# compilar JavaScriptCore cruzado. Se descargan una vez en obj/deps/src,
# comprobadas con su sha256. JavaScriptCore no: esta en runtime/thirdparty.
#
# Deja QUIX_ICU_SRC y QUIX_LLVM_SRC.

set(_src ${CMAKE_SOURCE_DIR}/obj/deps/src)
set(_gh https://github.com)

# quix_download(<url> <sha256>): el archivo, en obj/deps/src (si ya esta y
# su sha256 cuadra, no se descarga)
function(quix_download url hash)
  get_filename_component(_name ${url} NAME)
  file(DOWNLOAD ${url} ${_src}/${_name} EXPECTED_HASH SHA256=${hash} SHOW_PROGRESS)
endfunction()

# ICU 77.1: la fuente y los datos en bruto (sin ellos no hay filtro de datos)
set(QUIX_ICU_SRC ${_src}/icu-77.1)
if(NOT EXISTS ${QUIX_ICU_SRC}/.ready)
  set(_icu ${_gh}/unicode-org/icu/releases/download/release-77-1)
  quix_download(${_icu}/icu4c-77_1-src.tgz 588e431f77327c39031ffbb8843c0e3bc122c211374485fa87dc5f3faff24061)
  quix_download(${_icu}/icu4c-77_1-data.zip 1e08bfafa442260ccabf9a872d4eab12de813d42b90769df056bab032b37e1d3)
  file(REMOVE_RECURSE ${_src}/icu ${QUIX_ICU_SRC})
  file(ARCHIVE_EXTRACT INPUT ${_src}/icu4c-77_1-src.tgz DESTINATION ${_src})
  file(RENAME ${_src}/icu ${QUIX_ICU_SRC})
  file(REMOVE_RECURSE ${QUIX_ICU_SRC}/source/data)
  file(ARCHIVE_EXTRACT INPUT ${_src}/icu4c-77_1-data.zip DESTINATION ${QUIX_ICU_SRC}/source)
  file(TOUCH ${QUIX_ICU_SRC}/.ready)
endif()

# LLVM 22.1.8, solo los runtimes (libc++ y libc++abi): la misma version que
# el clang que compila para las portatiles. Android usa la libc++ del NDK.
if(QUIX_PLATFORM STREQUAL "portmaster")
  set(_llvm 22.1.8)
  execute_process(COMMAND clang --version OUTPUT_VARIABLE _clang)
  if(NOT _clang MATCHES "clang version ${_llvm}")
    message(FATAL_ERROR "libc++ ${_llvm} pide el clang ${_llvm}: cambia _llvm y su sha256 en cmake/jsc/sources.cmake")
  endif()
  set(QUIX_LLVM_SRC ${_src}/llvm-project-${_llvm})
  if(NOT EXISTS ${QUIX_LLVM_SRC}/.ready)
    set(_name llvm-project-${_llvm}.src)
    quix_download(${_gh}/llvm/llvm-project/releases/download/llvmorg-${_llvm}/${_name}.tar.xz
      922f1817a0df7b1489272d18134ee0087a8b068828f87ac63b9861b1a9965888)
    file(REMOVE_RECURSE ${_src}/${_name} ${QUIX_LLVM_SRC})
    file(ARCHIVE_EXTRACT INPUT ${_src}/${_name}.tar.xz DESTINATION ${_src}
      PATTERNS ${_name}/runtimes ${_name}/libcxx ${_name}/libcxxabi ${_name}/libunwind ${_name}/libc
        ${_name}/cmake ${_name}/llvm/cmake ${_name}/llvm/utils ${_name}/third-party ${_name}/LICENSE.TXT)
    file(RENAME ${_src}/${_name} ${QUIX_LLVM_SRC})
    file(TOUCH ${QUIX_LLVM_SRC}/.ready)
  endif()
endif()
