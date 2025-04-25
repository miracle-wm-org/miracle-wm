cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig)
pkg_check_modules(MIRCORE mircore REQUIRED)
pkg_check_modules(YAML REQUIRED IMPORTED_TARGET yaml-cpp)
pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)
find_package(GTest REQUIRED)

add_library(miracle-wm-config
    SHARED
    src/miracle-wm-config.cpp include/miracle/miracle-wm-config.h
    include/miracle/animation_definition.h
    include/miracle/container_type.h
    include/miracle/default_key_command.h
    include/miracle/modifiers.h
    src/miracle-wm-config-c.cpp include/miracle/miracle-wm-config-c.h)

set_target_properties(miracle-wm-config PROPERTIES
    VERSION 0.0.1
    SOVERSION 1
)

target_include_directories(miracle-wm-config PUBLIC SYSTEM
    include
    ${MIRCORE_INCLUDE_DIRS})

target_link_libraries(miracle-wm-config
    ${MIRCORE_LDFLAGS}
    PkgConfig::YAML
    PkgConfig::LIBEVDEV)

# Installation
install(TARGETS miracle-wm-config
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
)

# Tests
add_executable(test_miracle_wm_config_c_api
    tests/test_miracle_wm_config_c_api.cpp
)

target_include_directories(test_miracle_wm_config_c_api PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${MIRCORE_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
    ${GMOCK_INCLUDE_DIRS}
)

target_link_libraries(test_miracle_wm_config_c_api
    miracle-wm-config
    GTest::GTest
    GTest::Main
    ${MIRCORE_LDFLAGS}
)

add_test(NAME test_miracle_wm_config_c_api COMMAND test_miracle_wm_config_c_api)
