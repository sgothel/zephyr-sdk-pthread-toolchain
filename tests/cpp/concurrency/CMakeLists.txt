# SPDX-License-Identifier: Apache-2.0

#set (MY_CC_FLAGS "-fno-omit-frame-pointer" "-mno-omit-leaf-frame-pointer" "-fexceptions" "-fasynchronous-unwind-tables")
#list(APPEND TOOLCHAIN_LD_FLAGS ${MY_CC_FLAGS})

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(cpp_concurrency)

get_filename_component(TESTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
get_filename_component(TEMPDIR1 "${TESTS_DIR}" DIRECTORY)
get_filename_component(PROJECT_REPO_ROOT "${TEMPDIR1}" DIRECTORY)
include(${PROJECT_REPO_ROOT}/cmake/ClangdSetup.cmake)
ClangdSetup()

# Generate zephyr/heap_constants.h first
# https://docs.zephyrproject.org/latest/releases/migration-guide-4.4.html#kernel
add_dependencies(app zephyr_generated_headers)

FILE(GLOB app_sources src/*.cpp)
zephyr_include_directories(${ZEPHYR_BASE}/subsys/portability/posix)

target_sources(app PRIVATE ${app_sources})

#zephyr_library_compile_options(${MY_CC_FLAGS})
## zephyr_compile_options(${MY_CC_FLAGS})
## target_compile_options(app PRIVATE ${MY_CC_FLAGS})

target_include_directories(app PRIVATE ${ZEPHYR_BASE}/subsys/portability/posix/options)

