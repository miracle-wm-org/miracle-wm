find_package(PkgConfig)
pkg_check_modules(MIRCORE mircore REQUIRED)
pkg_check_modules(MIRSERVER mirserver REQUIRED)
pkg_check_modules(MIRAL miral REQUIRED)
pkg_check_modules(YAML REQUIRED IMPORTED_TARGET yaml-cpp)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
find_package(GTest REQUIRED)

add_library(miracle-wm-c
    SHARED
    src/config-cpp.cpp include/miracle/cpp/config-cpp.h
    include/miracle/cpp/animation_definition.h
        include/miracle/cpp/default_key_command.h
    include/miracle/cpp/cursor_focus_mode.h
    include/miracle/cpp/modifiers.h
    src/config-c.cpp include/miracle/config.h
    include/miracle/plugin.h)

set_target_properties(miracle-wm-c PROPERTIES
    CXX_STANDARD 20
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_include_directories(miracle-wm-c PUBLIC SYSTEM
    include
    ${MIRCORE_INCLUDE_DIRS}
    ${MIRAL_INCLUDE_DIRS}
    ${XKBCOMMON_INCLUDE_DIRS})

target_link_libraries(miracle-wm-c
    ${MIRCORE_LDFLAGS}
    ${MIRAL_LDFLAGS}
    PkgConfig::YAML
    ${XKBCOMMON_LDFLAGS}
    PkgConfig::GLIB)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/miracle-wm-c.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/miracle-wm-c.pc
    @ONLY
)

# Installation
install(TARGETS miracle-wm-c
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/miracle
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/miracle-wm-c.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# Tests
add_executable(test_miracle_wm_c_api
    tests/test_config_c.cpp
    tests/test_config_cpp.cpp
)

target_include_directories(test_miracle_wm_c_api PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${MIRCORE_INCLUDE_DIRS}
    ${MIRSERVER_INCLUDE_DIRS}
    ${MIRAL_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
    ${GMOCK_INCLUDE_DIRS}
)

target_link_libraries(test_miracle_wm_c_api
    miracle-wm-c
    GTest::GTest
    GTest::Main
    ${MIRCORE_LDFLAGS}
    ${MIRSERVER_LDFLAGS}
    ${MIRAL_LDFLAGS}
)

add_test(NAME test_miracle_wm_c_api COMMAND test_miracle_wm_c_api)
