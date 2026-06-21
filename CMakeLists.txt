cmake_minimum_required(VERSION 3.16) # PCH support needs 3.16+
cmake_policy(SET CMP0015 NEW)
cmake_policy(SET CMP0022 NEW)

project(miracle-wm VERSION 0.10.0)

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
option(ENABLE_TESTS "Build unit tests" ON)

if(COMPILING_AGAINST_DEV)
  set(COMPILING_AGAINST_DEV 1)
else()
  set(COMPILING_AGAINST_DEV 0)
endif()

# Feature flags
option(FEATURE_PLUGIN_SYSTEM "Enable the plugin system" ON)
option(FEATURE_PARENT_CONTAINER_WALLPAPERS "Enable parent container wallpapers feature" OFF)
option(BUILD_ERROR_REPORTER "Build the miracle-wm-basic-error-reporter GTK client" ON)
option(BUILD_DEBUG_OVERLAY "Build the miracle-wm-debug-overlay GTK client" ON)

if(FEATURE_PLUGIN_SYSTEM)
  set(FEATURE_PLUGIN_SYSTEM 1)
else()
  set(FEATURE_PLUGIN_SYSTEM 0)
endif()

if(FEATURE_PARENT_CONTAINER_WALLPAPERS)
  set(FEATURE_PARENT_CONTAINER_WALLPAPERS 1)
else()
  set(FEATURE_PARENT_CONTAINER_WALLPAPERS 0)
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
# Generate version.h and feature_flags.h from templates
# ----------------------------------------
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/src/version.h
    @ONLY
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/feature_flags.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/src/feature_flags.h
    @ONLY
)

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
        src/error_reporter_controller.cpp
        src/debug_overlay_controller.cpp
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
        src/plugin_manager_null.cpp
        src/plugin_manager_loader.cpp
        src/plugin_bridge.h src/plugin_bridge.cpp
        src/shell_application_manager.cpp src/shell_application_manager.h
        src/shell_application_spawner.h
        src/internal_shell_application_spawner.h src/internal_shell_application_spawner.cpp
        src/plugin_managed_container.h src/plugin_managed_container.cpp
        src/freestyle_window_container.h src/freestyle_window_container.cpp
        src/collection_container.h src/collection_container.cpp
        src/container_effect.h src/container_effect.cpp
        src/window_container.cpp src/window_container.h
        src/sampler_registry.h src/sampler_registry.cpp
)

target_compile_definitions(miracle-wm-implementation PRIVATE
        COMPILING_AGAINST_DEV=${COMPILING_AGAINST_DEV}
        FEATURE_PLUGIN_SYSTEM=${FEATURE_PLUGIN_SYSTEM}
        MIRACLE_PLUGIN_MODULE_PATH="${CMAKE_INSTALL_FULL_LIBDIR}/libmiracle-wm-plugins.so")

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
        ${MIRSERVER_INTERNAL_INCLUDE_DIRS}
        ${MIRSERVER_INCLUDE_DIRS}
        ${MIRWAYLAND_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/miracle-wm-c/include
        ${XKBCOMMON_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/src
)

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
        miracle-wm-c
        xdg-shell-protocol
)

# ----------------------------------------
# miracle-wm-plugins shared module (WasmEdge-backed plugin system)
# ----------------------------------------
# The WasmEdge runtime links LLVM. When that LLVM sits in the process's global
# symbol scope it interposes on Mesa's software renderer (llvmpipe), which also
# uses LLVM, and breaks rendering (issue #865 — black screen in QEMU). To avoid
# this, the WasmEdge-backed PluginManager lives in its own shared module that the
# main binary loads with dlopen()/RTLD_LOCAL only when needed, so libwasmedge and
# its LLVM never enter the global scope. The module resolves host symbols against
# the main executable at load time (hence ENABLE_EXPORTS on miracle-wm below).
if (FEATURE_PLUGIN_SYSTEM)
  add_library(miracle-wm-plugins SHARED src/plugin_manager_impl.cpp)

  target_compile_definitions(miracle-wm-plugins PRIVATE
          COMPILING_AGAINST_DEV=${COMPILING_AGAINST_DEV}
          FEATURE_PLUGIN_SYSTEM=1)

  target_include_directories(miracle-wm-plugins PRIVATE SYSTEM
          $<TARGET_PROPERTY:miracle-wm-implementation,INTERFACE_INCLUDE_DIRECTORIES>
          ${WASMEDGE_INCLUDE_DIR})

  # miracle-wm-c provides config-cpp headers/symbols; nlohmann is header-only.
  # The host (miracle-wm-implementation) symbols are intentionally NOT linked
  # here — they are resolved against the main executable at dlopen() time.
  target_link_libraries(miracle-wm-plugins
          PkgConfig::YAML
          nlohmann_json::nlohmann_json
          miracle-wm-c
          ${WASMEDGE_LIB})

  install(TARGETS miracle-wm-plugins DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# ----------------------------------------
# miracle-wm executable
# ----------------------------------------
add_executable(miracle-wm src/main.cpp)
target_include_directories(miracle-wm PUBLIC SYSTEM ${MIRAL_INCLUDE_DIRS})
# Export the executable's dynamic symbols so the dlopen'd plugin module can
# resolve host symbols against the main binary, and force every object of the
# (static) implementation library into the executable so none are dropped.
set_target_properties(miracle-wm PROPERTIES ENABLE_EXPORTS ON)
target_link_libraries(miracle-wm
        PUBLIC ${MIRAL_LDFLAGS}
        PRIVATE -Wl,--whole-archive miracle-wm-implementation -Wl,--no-whole-archive)

# ----------------------------------------
# Install rules
# ----------------------------------------
install(TARGETS miracle-wm DESTINATION ${CMAKE_INSTALL_BINDIR})

if(SYSTEMD_INTEGRATION)
  install(FILES
            session/usr/lib/systemd/user/miracle-wm-session.target
            session/usr/lib/systemd/user/miracle-wm-session-shutdown.target
            session/usr/lib/systemd/user/miracle-wm-xdg-autostart.target
            DESTINATION lib/systemd/user/
    )
  configure_file(session/usr/bin/libexec/miracle-wm-session-setup.in miracle-wm-session-setup @ONLY)
  install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/miracle-wm-session-setup DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
  install(PROGRAMS session/usr/bin/libexec/miracle-wm-wait-sni-ready DESTINATION ${CMAKE_INSTALL_LIBEXECDIR})
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/miracle-wm-session.systemd.in miracle-wm-session @ONLY)
else()
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/miracle-wm-session.none.in miracle-wm-session @ONLY)
endif()

configure_file(session/usr/share/wayland-sessions/miracle-wm.desktop.in miracle-wm.desktop @ONLY)
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
add_subdirectory(miracle-wm-c/)
add_subdirectory(protocols/)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w") # Disable warnings for miraclemsg, since its a fork
add_subdirectory(miraclemsg/)

if (BUILD_ERROR_REPORTER)
  add_subdirectory(error-reporter/)
endif()

if (BUILD_DEBUG_OVERLAY)
  add_subdirectory(debug-overlay/)
endif()
