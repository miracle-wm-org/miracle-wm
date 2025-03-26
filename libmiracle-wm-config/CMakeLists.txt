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
    include/miracle/modifiers.h
    src/miracle-wm-config-c.cpp include/miracle/miracle-wm-config-c.h)

target_include_directories(libmiracle-wm-config PUBLIC SYSTEM
    include
    ${MIRCORE_INCLUDE_DIRS})

target_link_libraries(libmiracle-wm-config
    ${MIRCORE_LDFLAGS}
    PkgConfig::YAML)

# Tests
if(BUILD_TESTING)
    add_executable(test_c_api
        tests/test_c_api.cpp
    )

    target_include_directories(test_c_api PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${MIRCORE_INCLUDE_DIRS}
    )

    target_link_libraries(test_c_api
        libmiracle-wm-config
        GTest::GTest
        GTest::Main
        ${MIRCORE_LDFLAGS}
    )

    add_test(NAME test_c_api COMMAND test_c_api)
endif()
