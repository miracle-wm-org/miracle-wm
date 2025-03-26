cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig)
pkg_check_modules(MIRCORE mircore REQUIRED)
pkg_check_modules(YAML REQUIRED IMPORTED_TARGET yaml-cpp)

add_library(libmiracle-wm-config
    SHARED
    src/miracle-wm-config.cpp include/miracle/miracle-wm-config.h
    include/miracle/animation_definition.h
    include/miracle/container_type.h
    include/miracle/default_key_command.h
    include/miracle/modifiers.h)

target_include_directories(libmiracle-wm-config PUBLIC SYSTEM
    include
    ${MIRCORE_INCLUDE_DIRS})

target_link_libraries(libmiracle-wm-config
    ${MIRCORE_LDFLAGS}
    PkgConfig::YAML)