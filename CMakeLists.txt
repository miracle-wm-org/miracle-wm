cmake_minimum_required(VERSION 3.22)

project(MirCompositor)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER g++)

set(ENV{PKG_CONFIG_PATH} "/usr/local/lib/pkgconfig/")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(PkgConfig)
pkg_check_modules(MIRAL miral REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(compositor
        src/display_listener.cpp
        src/main.cpp
        src/mirie_window_management_policy.cpp
        src/tiling_region.cpp
        src/tiled_node.cpp
    )

target_include_directories(compositor PUBLIC SYSTEM ${MIRAL_INCLUDE_DIRS})
target_link_libraries(     compositor               ${MIRAL_LDFLAGS})