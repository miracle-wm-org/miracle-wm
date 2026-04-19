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

#include "compositor_state.h"
#include "container_listener.h"
#include "mock_configuration.h"
#include "mock_output.h"
#include "mock_session.h"
#include "mock_surface.h"
#include "mock_window_controller.h"
#include "mock_workspace.h"
#include "plugin_managed_container.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace miracle;
using namespace testing;

namespace
{
geom::Rectangle const output_area {
    { 0,    0    },
    { 1920, 1080 }
};
geom::Point const window_position { 100, 200 };
geom::Size const window_size { 400, 300 };
}

class PluginManagedContainerTest : public ::testing::Test
{
public:
    PluginManagedContainerTest() :
        session(std::make_shared<NiceMock<test::MockSession>>()),
        surface(std::make_shared<NiceMock<test::MockSurface>>()),
        app(session),
        window(app, surface)
    {
        ON_CALL(*workspace, get_output()).WillByDefault(Return(output));
        ON_CALL(*output, get_area()).WillByDefault(ReturnRef(output_area));
        ON_CALL(*output, get_transform()).WillByDefault(Return(glm::mat4(1.f)));
        ON_CALL(*workspace, transform()).WillByDefault(Return(glm::mat4(1.f)));
        ON_CALL(*config, get_border_config()).WillByDefault(ReturnRef(border_config));
        ON_CALL(*surface, top_left()).WillByDefault(Return(window_position));
        ON_CALL(*surface, window_size()).WillByDefault(Return(window_size));
        ON_CALL(*window_controller, info_for(window))
            .WillByDefault(ReturnRef(window_info));

        container_resizable_and_movable = std::make_shared<PluginManagedContainer>(
            1, window, window_controller, state, workspace, config,
            glm::mat4(1.f), 1.f, true, true);
        container_not_resizable = std::make_shared<PluginManagedContainer>(
            2, window, window_controller, state, workspace, config,
            glm::mat4(1.f), 1.f, false, true);
        container_not_movable = std::make_shared<PluginManagedContainer>(
            3, window, window_controller, state, workspace, config,
            glm::mat4(1.f), 1.f, true, false);

        state->add(container_resizable_and_movable);
        state->add(container_not_resizable);
        state->add(container_not_movable);
    }

protected:
    BorderConfig border_config { .size = 0 };
    miral::WindowInfo window_info;
    std::shared_ptr<CompositorState> state = std::make_shared<CompositorState>();
    std::shared_ptr<test::MockWindowController> window_controller = std::make_shared<NiceMock<test::MockWindowController>>();
    std::shared_ptr<NiceMock<test::MockConfig>> config = std::make_shared<NiceMock<test::MockConfig>>();
    std::shared_ptr<NiceMock<test::MockWorkspace>> workspace = std::make_shared<NiceMock<test::MockWorkspace>>();
    std::shared_ptr<NiceMock<test::MockOutput>> output = std::make_shared<NiceMock<test::MockOutput>>();
    std::shared_ptr<NiceMock<test::MockSession>> session;
    std::shared_ptr<NiceMock<test::MockSurface>> surface;
    miral::Application app;
    miral::Window window;
    std::shared_ptr<PluginManagedContainer> container_resizable_and_movable;
    std::shared_ptr<PluginManagedContainer> container_not_resizable;
    std::shared_ptr<PluginManagedContainer> container_not_movable;
};

// ---- is_fullscreen ----

TEST_F(PluginManagedContainerTest, IsFullscreenReturnsFalseByDefault)
{
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_restored));
    EXPECT_FALSE(container_resizable_and_movable->is_fullscreen());
}

TEST_F(PluginManagedContainerTest, IsFullscreenReturnsTrueWhenStateIsFullscreen)
{
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_fullscreen));
    EXPECT_TRUE(container_resizable_and_movable->is_fullscreen());
}

// ---- toggle_fullscreen ----

TEST_F(PluginManagedContainerTest, ToggleFullscreenReturnsFalseWhenNotResizable)
{
    EXPECT_FALSE(container_not_resizable->toggle_fullscreen());
}

TEST_F(PluginManagedContainerTest, ToggleFullscreenReturnsFalseWhenNotMovable)
{
    EXPECT_FALSE(container_not_movable->toggle_fullscreen());
}

TEST_F(PluginManagedContainerTest, ToggleFullscreenReturnsTrueWhenResizableAndMovable)
{
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_restored));
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_fullscreen)).Times(1);
    EXPECT_TRUE(container_resizable_and_movable->toggle_fullscreen());
}

TEST_F(PluginManagedContainerTest, ToggleFullscreenWhenFullscreenRestores)
{
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_fullscreen));
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_restored)).Times(1);
    EXPECT_TRUE(container_resizable_and_movable->toggle_fullscreen());
}

TEST_F(PluginManagedContainerTest, ToggleFullscreenNotifiesObserver)
{
    class Listener : public NullContainerListener
    {
    public:
        MOCK_METHOD(void, on_container_fullscreen, (Container const&), (override));
    };

    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_restored));
    auto listener = std::make_shared<Listener>();
    container_resizable_and_movable->register_interest(listener);
    EXPECT_CALL(*listener, on_container_fullscreen(_)).Times(1);
    container_resizable_and_movable->toggle_fullscreen();
}

TEST_F(PluginManagedContainerTest, ToggleFullscreenRestoresAreaWhenLeavingFullscreen)
{
    // First call: window at (100,200) 400x300 → goes fullscreen, saves area
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_restored));
    container_resizable_and_movable->toggle_fullscreen();

    // Second call: window is now fullscreen → restore; set_rectangle should use saved area
    ON_CALL(*window_controller, get_state(window))
        .WillByDefault(Return(mir_window_state_fullscreen));
    EXPECT_CALL(*window_controller, set_rectangle(window, _, Truly([](geom::Rectangle const& r)
    {
        return r.top_left == geom::Point { 100, 200 }
        && r.size == geom::Size { 400, 300 };
    }),
                                        _))
        .Times(1);
    container_resizable_and_movable->toggle_fullscreen();
}

// ---- handle_modify ----

TEST_F(PluginManagedContainerTest, HandleModifyBlocksFullscreenWhenNotResizable)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_fullscreen;
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_restored)).Times(1);
    container_not_resizable->handle_modify(spec);
}

TEST_F(PluginManagedContainerTest, HandleModifyBlocksFullscreenWhenNotMovable)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_fullscreen;
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_restored)).Times(1);
    container_not_movable->handle_modify(spec);
}

TEST_F(PluginManagedContainerTest, HandleModifyAllowsFullscreenWhenResizableAndMovable)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_fullscreen;
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_fullscreen)).Times(1);
    container_resizable_and_movable->handle_modify(spec);
}

TEST_F(PluginManagedContainerTest, HandleModifyBlocksMaximizeWhenNotResizable)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_maximized;
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_restored)).Times(1);
    container_not_resizable->handle_modify(spec);
}

TEST_F(PluginManagedContainerTest, HandleModifyAllowsMaximizeWhenResizableAndMovable)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_maximized;
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_maximized)).Times(1);
    container_resizable_and_movable->handle_modify(spec);
}

TEST_F(PluginManagedContainerTest, HandleModifyBlocksMaximizeWhenNotMovable)
{
    miral::WindowSpecification spec;
    spec.state() = mir_window_state_maximized;
    EXPECT_CALL(*window_controller, change_state(window, mir_window_state_restored)).Times(1);
    container_not_movable->handle_modify(spec);
}
