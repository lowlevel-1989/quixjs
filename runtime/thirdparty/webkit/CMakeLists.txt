# JavaScriptCore de quixjs: el port JSCOnly de WebKit, solo con lo que usa
# quixjs (JavaScriptCore, WTF, bmalloc y unifdef). Ver README.md.
#
# NOTE: cmake_minimum_required() and project() *MUST* be the two first commands
# used, see https://cmake.org/cmake/help/v3.3/command/project.html -- the
# latter in particular handles loading a bunch of shared CMake definitions
# and loading the cross-compilation settings from CMAKE_TOOLCHAIN_FILE.

cmake_minimum_required(VERSION 3.20)
project(WebKit)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(WebKitCommon)

if (NOT USE_SYSTEM_MALLOC)
    add_subdirectory(bmalloc)
endif ()

add_subdirectory(WTF)

if (USE_SYSTEM_UNIFDEF)
    find_package(Unifdef REQUIRED)
else ()
    add_subdirectory(unifdef)
endif ()

add_subdirectory(JavaScriptCore)

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

PRINT_WEBKIT_OPTIONS()
