find_package(PkgConfig)
pkg_check_modules(MIRCORE mircore REQUIRED)
pkg_check_modules(YAML REQUIRED IMPORTED_TARGET yaml-cpp)
pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
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
    CXX_STANDARD 20
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(miracle-wm-config PUBLIC SYSTEM
    include
    ${MIRCORE_INCLUDE_DIRS})

target_link_libraries(miracle-wm-config
    ${MIRCORE_LDFLAGS}
    PkgConfig::YAML
    PkgConfig::LIBEVDEV
    PkgConfig::GLIB)

# Installation
install(TARGETS miracle-wm-config
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
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
