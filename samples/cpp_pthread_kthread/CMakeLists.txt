cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr_sample_test)

get_filename_component(SAMPLES_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
get_filename_component(PROJECT_REPO_ROOT "${SAMPLES_DIR}" DIRECTORY)
include(${PROJECT_REPO_ROOT}/cmake/ClangdSetup.cmake)
ClangdSetup()

include_directories (${PROJECT_REPO_ROOT}/include)

add_compile_options(-g -ggdb -DDEBUG)

# Generate zephyr/heap_constants.h first
# https://docs.zephyrproject.org/latest/releases/migration-guide-4.4.html#kernel
add_dependencies(app zephyr_generated_headers)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)

# add_executable(app)
target_sources(app
               PRIVATE
               src/main.cpp
               ${PROJECT_REPO_ROOT}/src/miscutils.cpp
              )

target_link_libraries(app PRIVATE kernel)

