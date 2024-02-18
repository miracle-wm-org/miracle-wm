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
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(YAML REQUIRED IMPORTED_TARGET yaml-cpp)
pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)
find_package(nlohmann_json 3.2.0 REQUIRED)

add_executable(miracle-wm
    src/main.cpp
    src/miracle_window_management_policy.cpp
    src/window_tree.cpp
    src/node.cpp
    src/window_helpers.h
    src/window_helpers.cpp
    src/miracle_config.cpp
    src/miracle_config.h
    src/screen.cpp
    src/workspace_manager.cpp
    src/ipc.cpp
    src/auto_restarting_launcher.cpp
)

target_include_directories(miracle-wm PUBLIC SYSTEM ${MIRAL_INCLUDE_DIRS})
target_link_libraries(     miracle-wm               ${MIRAL_LDFLAGS} PkgConfig::YAML PkgConfig::GLIB PkgConfig::LIBEVDEV nlohmann_json::nlohmann_json)

install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/miracle-wm
    DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(FILES session/usr/local/share/wayland-sessions/miracle-wm-non-snap.desktop
    DESTINATION /usr/share/wayland-sessions)

add_subdirectory(tests/)
