/**
Copyright (C) 2025  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include "animator.h"
#include "compositor_state.h"
#include "dying_surface_manager.h"
#include "mir/geometry/forward.h"
#include "mock_configuration.h"
#include "mock_container.h"
#include "mock_main_loop.h"
#include "mock_output.h"
#include "mock_session.h"
#include "mock_surface.h"
#include "mock_surface_stack.h"
#include "mock_window_controller.h"
#include "plugin_bridge.h"
#include <miral/window_info.h>

#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include <memory>

using namespace miracle;

class DyingSurfaceManagerTest : public testing::Test
{
public:
    DyingSurfaceManagerTest() :
        surface_stack(std::make_shared<test::MockSurfaceStack>()),
        compositor_state(std::make_shared<CompositorState>()),
        config(std::make_shared<test::MockConfig>()),
        animator(std::make_shared<Animator>()),
        window_controller(std::make_shared<test::MockWindowController>()),
        window_id_map(std::make_shared<WindowIdMap>()),
        dying_surface_manager(
            surface_stack,
            compositor_state,
            config,
            animator,
            std::make_shared<PluginManager>(),
            window_controller,
            window_id_map)
    {
    }

    std::shared_ptr<test::MockSurfaceStack> surface_stack;
    std::shared_ptr<CompositorState> compositor_state;
    std::shared_ptr<test::MockConfig> config;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<test::MockWindowController> window_controller;
    std::shared_ptr<WindowIdMap> window_id_map;
    DyingSurfaceManager dying_surface_manager;
};

TEST_F(DyingSurfaceManagerTest, RemovesSurfaceFromStackWhenAnimationCompletes)
{
    auto const container = std::make_shared<test::MockContainer>();
    auto const output = std::make_shared<test::MockOutput>();
    geom::Rectangle constexpr output_area({ 0, 0 }, { 1920, 1080 });

    EXPECT_CALL(*config, are_animations_enabled())
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*container, get_output())
        .WillOnce(testing::Return(output));
    EXPECT_CALL(*output, get_area())
        .WillOnce(testing::ReturnRef(output_area));

    auto const session = std::make_shared<test::MockSession>();
    auto const surface = std::make_shared<test::MockSurface>();
    miral::Window const window(session, surface);

    EXPECT_CALL(*container, window())
        .WillOnce(testing::Return(window));

    std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)> animation_definitions;
    animation_definitions[static_cast<int>(AnimateableEvent::window_close)] = {};
    EXPECT_CALL(*config, get_animation_definition(AnimateableEvent::window_close))
        .WillOnce(testing::ReturnRef(animation_definitions[static_cast<int>(AnimateableEvent::window_close)]));

    constexpr mir::geometry::Rectangle area(
        mir::geometry::Point(0, 0),
        mir::geometry::Size(100, 100));
    EXPECT_CALL(*container, get_visible_area())
        .WillRepeatedly(testing::Return(area));
    EXPECT_CALL(*container, get_output_transform())
        .WillOnce(testing::Return(glm::mat4(1.f)));
    EXPECT_CALL(*container, get_workspace_transform())
        .WillOnce(testing::Return(glm::mat4(1.f)));
    EXPECT_CALL(*container, get_animation_transform())
        .WillOnce(testing::Return(glm::mat4(1.f)));

    miral::WindowInfo window_info;
    EXPECT_CALL(*window_controller, info_for(testing::_))
        .WillOnce(testing::ReturnRef(window_info));

    EXPECT_CALL(*surface_stack, add_surface(testing::_, mir::input::InputReceptionMode::normal))
        .Times(1);
    EXPECT_CALL(*surface_stack, remove_surface(testing::_))
        .Times(1);

    // With duration_seconds=0, the animation completes on the initial tick inside
    // Animator::append, so remove_surface is called during animate_dying_surface itself.
    dying_surface_manager.animate_dying_surface(container);
}

TEST_F(DyingSurfaceManagerTest, CanAnimateValidSurface)
{
    auto const container = std::make_shared<test::MockContainer>();
    auto const output = std::make_shared<test::MockOutput>();
    geom::Rectangle constexpr output_area({ 0, 0 }, { 19280, 1080 });

    // Pre-conditions for starting the animation
    EXPECT_CALL(*config, are_animations_enabled())
        .WillOnce(testing::Return(true));
    EXPECT_CALL(*container, get_output())
        .WillOnce(testing::Return(output));
    EXPECT_CALL(*output, get_area())
        .WillOnce(testing::ReturnRef(output_area));

    auto const session = std::make_shared<test::MockSession>();
    auto const surface = std::make_shared<test::MockSurface>();
    miral::Window const window(session, surface);

    // Condition for starting the loop
    EXPECT_CALL(*container, window())
        .WillOnce(testing::Return(window));

    // Resolution of the animation
    std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)> animation_definitions;
    animation_definitions[static_cast<int>(AnimateableEvent::window_close)] = {};
    EXPECT_CALL(*config, get_animation_definition(AnimateableEvent::window_close))
        .WillOnce(testing::ReturnRef(animation_definitions[static_cast<int>(AnimateableEvent::window_close)]));

    constexpr mir::geometry::Rectangle area(
        mir::geometry::Point(0, 0),
        mir::geometry::Size(100, 100));
    EXPECT_CALL(*container, get_visible_area())
        .WillRepeatedly(testing::Return(area));
    EXPECT_CALL(*container, get_output_transform())
        .WillOnce(testing::Return(glm::mat4(1.f)));
    EXPECT_CALL(*container, get_workspace_transform())
        .WillOnce(testing::Return(glm::mat4(1.f)));
    EXPECT_CALL(*container, get_animation_transform())
        .WillOnce(testing::Return(glm::mat4(1.f)));

    miral::WindowInfo window_info;
    EXPECT_CALL(*window_controller, info_for(testing::_))
        .WillOnce(testing::ReturnRef(window_info));

    // Expect the surface stack to have been modified as well as the animator
    EXPECT_CALL(*surface_stack, add_surface(testing::_, mir::input::InputReceptionMode::normal))
        .Times(1);
    EXPECT_FALSE(animator->has_animations());
    dying_surface_manager.animate_dying_surface(container);
    EXPECT_TRUE(animator->has_animations());
}
