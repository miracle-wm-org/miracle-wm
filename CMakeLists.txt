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

option(SNAP_BUILD "Building as a snap?" OFF)

find_package(PkgConfig)
pkg_check_modules(MIRAL miral REQUIRED)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(YAML REQUIRED IMPORTED_TARGET yaml-cpp)
pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)
find_package(nlohmann_json 3.2.0 REQUIRED)
pkg_check_modules(LIBNOTIFY REQUIRED IMPORTED_TARGET libnotify)

include(GNUInstallDirs)

add_library(miracle-wm-implementation
    STATIC
    src/policy.cpp
    src/tiling_window_tree.cpp
    src/node.cpp
    src/window_helpers.h
    src/window_helpers.cpp
    src/miracle_config.cpp
    src/miracle_config.h
    src/output_content.cpp
    src/workspace_manager.cpp
    src/ipc.cpp
    src/auto_restarting_launcher.cpp
    src/workspace_observer.cpp
    src/window_metadata.cpp
    src/workspace_content.cpp
    src/leaf_node.cpp
    src/parent_node.cpp
    src/window_manager_tools_tiling_interface.cpp
)

add_executable(miracle-wm
    src/main.cpp
)

target_include_directories(miracle-wm-implementation PUBLIC SYSTEM ${MIRAL_INCLUDE_DIRS})
target_link_libraries(     miracle-wm-implementation               ${MIRAL_LDFLAGS}
    PkgConfig::YAML PkgConfig::GLIB PkgConfig::LIBEVDEV PkgConfig::LIBNOTIFY nlohmann_json::nlohmann_json)

target_include_directories(miracle-wm PUBLIC SYSTEM ${MIRAL_INCLUDE_DIRS})
target_link_libraries(miracle-wm PUBLIC ${MIRAL_LDFLAGS} PRIVATE miracle-wm-implementation)

install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/miracle-wm
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

configure_file(session/usr/local/share/wayland-sessions/miracle-wm.desktop.in miracle-wm.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/miracle-wm.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/wayland-sessions)

add_custom_target(miracle-wm-sensible-terminal ALL
    cp ${CMAKE_CURRENT_SOURCE_DIR}/src/miracle-wm-sensible-terminal ${CMAKE_BINARY_DIR}/bin
)

install(PROGRAMS
    src/miracle-wm-sensible-terminal
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(SNAP_BUILD)
    add_custom_target(miracle-wm-unsnap ALL
        cp ${CMAKE_CURRENT_SOURCE_DIR}/src/miracle-wm-unsnap ${CMAKE_BINARY_DIR}/bin
    )
    install(PROGRAMS
        src/miracle-wm-unsnap
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

add_subdirectory(tests/)
