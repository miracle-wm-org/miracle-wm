cmake_minimum_required(VERSION 3.7)
cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0022 NEW)

project(miracle-wm)

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

add_executable(miracle-wm
    src/main.cpp
    src/miracle_window_management_policy.cpp
    src/window_tree.cpp
    src/node.cpp
    src/window_helpers.h
    src/window_helpers.cpp
)

target_include_directories(miracle-wm PUBLIC SYSTEM ${MIRAL_INCLUDE_DIRS})
target_link_libraries(     miracle-wm               ${MIRAL_LDFLAGS})

install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/miracle-wm
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

add_subdirectory(tests/)
