cmake_minimum_required(VERSION 3.0)
project(Project)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(BUILD_SHARED_LIBS "Build libraries as SHARED" OFF)

set(Project_INSTALL_PREFIX "${Project_BINARY_DIR}/prefix")
set(Project_DEFAULT_ARGS
    "-DBUILD_SHARED_LIBS:BOOL=${BUILD_SHARED_LIBS}"
    "-DCMAKE_PREFIX_PATH:PATH=${Project_INSTALL_PREFIX};${CMAKE_PREFIX_PATH}"
    "-DCMAKE_INSTALL_PREFIX:PATH=${Project_INSTALL_PREFIX}"
)
file(MAKE_DIRECTORY ${Project_INSTALL_PREFIX})

# build external dependencies
add_subdirectory(extern)
