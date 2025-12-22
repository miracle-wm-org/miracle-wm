cmake_minimum_required(VERSION 3.16) # PCH support needs 3.16+
cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0022 NEW)

project(miracle-wm VERSION 0.8.2)

# ----------------------------------------
# Speed tweaks
# ----------------------------------------

# Prefer Ninja for faster builds (if user hasn't chosen a generator)
if(CMAKE_GENERATOR STREQUAL "")
    message(STATUS "Tip: Use Ninja for faster builds: cmake -G Ninja")
endif()

# Enable ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

# Enable mold if available
find_program(MOLD_LINKER mold)
if(MOLD_LINKER)
    message(STATUS "Using mold linker: ${MOLD_LINKER}")
    add_link_options(-fuse-ld=mold)
endif()

# Optional LTO
option(ENABLE_LTO "Enable link-time optimization (slower builds, faster binaries)" OFF)
if(ENABLE_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# ----------------------------------------
# Build settings
# ----------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(SNAP_BUILD "Building as a snap?" OFF)
option(SYSTEMD_INTEGRATION "Specifies that the systemd integration script will run at startup" OFF)
option(COMPILING_AGAINST_DEV "Enable if compiling against Mir RC or dev" OFF)
option(ENABLE_PLUGIN_SYSTEM "Enable the plugin system" OFF)
option(ENABLE_TESTS "Build unit tests" ON)

if(COMPILING_AGAINST_DEV)
    set(COMPILING_AGAINST_DEV 1)
else()
    set(COMPILING_AGAINST_DEV 0)
endif()

if(ENABLE_PLUGIN_SYSTEM)
    set(ENABLE_PLUGIN_SYSTEM 1)
else()
    set(ENABLE_PLUGIN_SYSTEM 0)
endif()

# ----------------------------------------
# Compiler warnings
# ----------------------------------------
if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
    add_compile_options(
            -Wall -Wextra -Wpedantic
            -Wconversion -Wsign-conversion
            -Wimplicit-fallthrough
            -Wunreachable-code
            -Wno-unused-parameter
            -g -fno-omit-frame-pointer
    )
else()
    message(STATUS "Skipping most warnings for non-Debug build")
endif()

if(CMAKE_BUILD_TYPE MATCHES Release)
    add_compile_options(
            -O3
            -march=native
    )
endif()

# ----------------------------------------
# Dependencies
# ----------------------------------------
find_package(PkgConfig)

set(MIR_MINIMUM_VERSION 2.18)
pkg_check_modules(MIRAL REQUIRED miral>=5.1)
pkg_check_modules(MIRRENDERER mirrenderer>=${MIR_MINIMUM_VERSION})
pkg_check_modules(MIRPLATFORM REQUIRED mirplatform>=${MIR_MINIMUM_VERSION})
pkg_check_modules(MIRCOMMON REQUIRED mircommon>=${MIR_MINIMUM_VERSION})
pkg_check_modules(MIRCOMMON_INTERNAL REQUIRED mircommon-internal>=${MIR_MINIMUM_VERSION})
pkg_check_modules(MIRSERVER REQUIRED mirserver>=${MIR_MINIMUM_VERSION})
pkg_check_modules(MIRSERVER_INTERNAL REQUIRED mirserver-internal>=${MIR_MINIMUM_VERSION})
pkg_check_modules(MIRWAYLAND REQUIRED mirwayland>=${MIR_MINIMUM_VERSION})
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(YAML REQUIRED IMPORTED_TARGET yaml-cpp)
pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)
find_package(nlohmann_json 3.2.0 REQUIRED)
pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(GLESv2 REQUIRED IMPORTED_TARGET glesv2)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)
find_path(WASMEDGE_INCLUDE_DIR wasmedge/wasmedge.h)
find_library(WASMEDGE_LIB wasmedge)

include(GNUInstallDirs)

# ----------------------------------------
# miracle-wm-implementation library
# ----------------------------------------
add_library(miracle-wm-implementation STATIC
        src/policy.cpp
        src/container.cpp
        src/window_helpers.cpp
        src/config.cpp
        src/output.cpp
        src/parent_background_internal_client.cpp
        src/workspace_manager.cpp
        src/ipc_message_handler.cpp
        src/ipc_connection_manager.cpp
        src/auto_restarting_launcher.cpp
        src/workspace_observer.cpp
        src/workspace.cpp
        src/leaf_container.cpp
        src/parent_container.cpp
        src/window_manager_tools_window_controller.cpp
        src/renderer.cpp
        src/tessellation_helpers.cpp
        src/ipc_command.cpp
        src/ipc_command_executor.cpp
        src/render_data_manager.cpp
        src/animator.cpp
        src/animation.cpp
        src/program_factory.cpp
        src/mode_observer.cpp
        src/shell_component_container.cpp
        src/container_group_container.cpp
        src/layout_scheme.cpp
        src/scratchpad.cpp
        src/compositor_state.cpp
        src/command_controller.cpp
        src/animator_loop.cpp
        src/drag_and_drop_service.cpp
        src/output_manager.cpp
        src/output_factory.cpp
        src/move_service.cpp
        src/resize_service.cpp
        src/forwarding_surface.cpp
        src/dying_surface_manager.cpp
        src/shader_2d.cpp
        src/wlr-output-management-unstable-v1_wrapper.cpp
        src/wlr-ouput-management-unstable-v1.cpp
        src/output_listener.cpp
        src/display_config.cpp
        src/window_observer.cpp
        src/config_observer.cpp
        src/binding_event.cpp
        src/magnifier_wrapper.cpp
        src/math_helpers.cpp
        src/plugin_manager.cpp
        src/shell_application_manager.cpp
)

target_compile_definitions(miracle-wm-implementation PRIVATE COMPILING_AGAINST_DEV=${COMPILING_AGAINST_DEV} ENABLE_PLUGIN_SYSTEM=${ENABLE_PLUGIN_SYSTEM})

# ----------------------------------------
# Precompiled headers for speed
# ----------------------------------------
target_precompile_headers(miracle-wm-implementation PRIVATE
        # C++ Standard Library (most frequently used)
        <memory>
        <vector>
        <string>
        <mutex>
        <algorithm>
        <optional>
        <map>
        <utility>
        <functional>

        # Third-party libraries (frequent)
        # NOTE: mir/log.h is NOT included here because it requires MIR_LOG_COMPONENT to be defined first
        <miral/window_manager_tools.h>
        <mir/geometry/rectangle.h>
        <mir/geometry/point.h>
        <mir/scene/surface.h>
        <mir/scene/session.h>
        <miral/window.h>
        <glm/glm.hpp>
        <nlohmann/json.hpp>
        <yaml-cpp/yaml.h>

        # OpenGL
        <GLES2/gl2.h>

        # PCRE2
        <jpcre2.h>
        <pcre2.h>
)

target_include_directories(miracle-wm-implementation PUBLIC SYSTEM
        ${MIRAL_INCLUDE_DIRS}
        ${MIRCOMMON_INCLUDE_DIRS}
        ${MIRRENDERER_INCLUDE_DIRS}
        ${MIRPLATFORM_INCLUDE_DIRS}
        ${MIRCOMMON_INTERNAL_INCLUDE_DIRS}
        ${MIRSERVER_INTERNAL_INCLUDE_DIRS}
        ${MIRSERVER_INCLUDE_DIRS}
        ${MIRWAYLAND_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/miracle-wm-config/include
        ${XKBCOMMON_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if (ENABLE_PLUGIN_SYSTEM)
    target_include_directories(miracle-wm-implementation PRIVATE ${WASMEDGE_INCLUDE_DIR})
endif()

target_link_libraries(miracle-wm-implementation
        ${MIRAL_LDFLAGS}
        ${MIRRENDERER_LDFLAGS}
        ${MIRPLATFORM_LDFLAGS}
        ${MIRSERVER_LDFLAGS}
        ${MIRSERVER_INTERNAL_LDFLAGS}
        ${MIRWAYLAND_LDFLAGS}
        ${XKBCOMMON_LIBRARIES}
        PkgConfig::YAML
        PkgConfig::GLIB
        PkgConfig::LIBEVDEV
        nlohmann_json::nlohmann_json
        PkgConfig::EGL
        PkgConfig::GLESv2
        -lpcre2-8 -lpcre2-16 -lpcre2-32
        miracle-wm-config
        xdg-shell-protocol
)

if (ENABLE_PLUGIN_SYSTEM)
    target_link_libraries(miracle-wm-implementation ${WASMEDGE_LIB})
endif()

# ----------------------------------------
# miracle-wm executable
# ----------------------------------------
add_executable(miracle-wm src/main.cpp)
target_include_directories(miracle-wm PUBLIC SYSTEM ${MIRAL_INCLUDE_DIRS})
target_link_libraries(miracle-wm PUBLIC ${MIRAL_LDFLAGS} PRIVATE miracle-wm-implementation)

# ----------------------------------------
# Install rules
# ----------------------------------------
install(TARGETS miracle-wm DESTINATION ${CMAKE_INSTALL_BINDIR})

if(SYSTEMD_INTEGRATION)
    install(FILES
            session/usr/lib/systemd/user/miracle-wm-session.target
            session/usr/lib/systemd/user/miracle-wm-session-shutdown.target
            session/usr/lib/systemd/user/miracle-wm-xdg-autostart.target
            DESTINATION /usr/lib/systemd/user/
    )
    configure_file(session/usr/bin/libexec/miracle-wm-session-setup.in miracle-wm-session-setup @ONLY)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/miracle-wm-session-setup DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
    install(PROGRAMS session/usr/bin/libexec/miracle-wm-wait-sni-ready DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/miracle-wm-session.systemd.in miracle-wm-session @ONLY)
else()
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/miracle-wm-session.none.in miracle-wm-session @ONLY)
endif()

configure_file(session/usr/local/share/wayland-sessions/miracle-wm.desktop.in miracle-wm.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/miracle-wm.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/wayland-sessions)

install(PROGRAMS
        src/miracle-wm-sensible-terminal
        ${CMAKE_CURRENT_BINARY_DIR}/miracle-wm-session
        DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(SNAP_BUILD)
    install(PROGRAMS src/miracle-wm-unsnap DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(
    DIRECTORY shaders/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/miracle-wm/shaders
    FILES_MATCHING PATTERN "*.frag"
)

# ----------------------------------------
# Subdirectories
# ----------------------------------------
if (ENABLE_TESTS)
add_subdirectory(tests/)
endif()
add_subdirectory(miracle-wm-config/)
add_subdirectory(protocols/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w") # Disable warnings for miraclemsg, since its a fork
add_subdirectory(miraclemsg/)
