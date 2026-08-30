set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/../src
)

find_package(PkgConfig)
pkg_check_modules(MIRAL miral REQUIRED)
pkg_check_modules(MIRSERVER mirserver REQUIRED)
pkg_check_modules(MIRTEST mirtest REQUIRED)
pkg_check_modules(YAML REQUIRED IMPORTED_TARGET yaml-cpp)
find_package(GTest REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

set(SOURCES
    mock_binding_event_listener.h
    mock_command_controller.h
    mock_configuration.h
    mock_container.h
    mock_ipc_command_executor.h
    mock_plugin_manager.h
    mock_output.h
    mock_output_factory.h
    mock_surface_stack.h
    mock_main_loop.h
    test_filesystem_configuration.cpp
    test_workspace.cpp
    test_ipc_command_parser.cpp
    test_animator.cpp
    test_feature_flags.cpp
    test_drag_and_drop_service.cpp
    test_resize_service.cpp
    test_render_data_manager.cpp
    test_scene_override_manager.cpp
    test_carousel_layout.cpp
    test_overview_scene_override.cpp
    test_workspace_preview.cpp
    mock_workspace.h
    test_sampler_registry.cpp
    test_output_manager.cpp
    test_output.cpp
    test_freestyle_window_container.cpp
    test_plugin_managed_container.cpp
    test_leaf_container.cpp
    test_scratchpad.cpp
    test_command_controller.cpp
    test_dying_surface_manager.cpp
    test_tiling_algorithms.cpp
    test_parent_container.cpp
    test_container.cpp
    test_ipc_message_handler.cpp
    test_ipc_command_executor.cpp
    test_workspace_manager.cpp
    test_compositor_state.cpp
    test_plugin_helpers.cpp
    test_display_config.cpp
    stub_configuration.h
    stub_surface.h
    stub_window_controller.h
    stub_container.h
    vertical_display_configuration_policy.h)

if (END_TO_END_TESTS)
    list(APPEND SOURCES test_policy.cpp)
endif()

add_executable(miracle-wm-tests ${SOURCES})

target_include_directories(miracle-wm-tests PUBLIC SYSTEM
        ${PROJECT_SOURCE_DIR}/miracle-wm-c/include
    ${PROJECT_SOURCE_DIR}/miraclemsg
    ${MIRAL_INCLUDE_DIRS}
    ${MIRSERVER_INCLUDE_DIRS}
    ${MIRTEST_INCLUDE_DIRS}
    ${GTEST_INCLUDE_DIRS}
    ${GMOCK_INCLUDE_DIRS})

target_link_libraries(miracle-wm-tests
    miracle-wm-implementation
    miracle-wm-c
    miracle-ipc
    ${GTEST_LIBRARIES}
    ${MIRAL_LDFLAGS}
    ${MIRSERVER_LDFLAGS}
    ${MIRTEST_LDFLAGS}
    PkgConfig::YAML
    nlohmann_json::nlohmann_json
    pthread
    GTest::GTest GTest::Main GTest::gmock)

target_compile_definitions(miracle-wm-tests PRIVATE
    COMPILING_AGAINST_DEV=${COMPILING_AGAINST_DEV}
    FEATURE_PLUGIN_SYSTEM=${FEATURE_PLUGIN_SYSTEM}
    FEATURE_PARENT_CONTAINER_WALLPAPERS=${FEATURE_PARENT_CONTAINER_WALLPAPERS}
)

# Precompiled headers for faster test compilation
# Note: Can't reuse from implementation due to C++20 vs C++23 difference
target_precompile_headers(miracle-wm-tests PRIVATE
        # C++ Standard Library
        <memory>
        <vector>
        <string>
        <optional>
        <map>
        <algorithm>
        <functional>
        <filesystem>

        # GLM (CRITICAL - template-heavy, used by almost all tests)
        <glm/glm.hpp>

        # MIRAL headers (frequently used)
        <miral/window_manager_tools.h>
        <miral/window.h>
        <miral/window_info.h>
        <miral/window_specification.h>
        <miral/application_info.h>
        <miral/output.h>
        <miral/zone.h>

        # Mir headers
        <mir/geometry/rectangle.h>
        <mir/geometry/point.h>
        <mir/scene/surface.h>
        <mir/scene/session.h>
        <mir_toolkit/common.h>

        # Third-party libraries
        <nlohmann/json.hpp>
        <yaml-cpp/yaml.h>

        # GTest/GMock
        <gtest/gtest.h>
        <gmock/gmock.h>
)

enable_testing()
