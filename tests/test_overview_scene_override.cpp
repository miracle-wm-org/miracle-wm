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

#include "overview_scene_override.h"

#include "animator.h"
#include "compositor_state.h"
#include "leaf_container.h"
#include "mock_output.h"
#include "mock_output_factory.h"
#include "mock_parent_container.h"
#include "mock_session.h"
#include "mock_surface.h"
#include "mock_window_controller.h"
#include "mock_workspace.h"
#include "output_manager.h"
#include "overview_scene_override_delegate.h"
#include "shell_application_spawner.h"
#include "shell_component_container.h"
#include "stub_configuration.h"
#include "workspace_manager.h"
#include "workspace_observer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <miral/window_specification.h>

using namespace miracle;
using namespace testing;
namespace geom = mir::geometry;

namespace
{
geom::Rectangle const OUTPUT_AREA {
    { 0,    0    },
    { 1920, 1080 }
};

/// Records what the overview asked the desktop to do.
class RecordingDelegate : public OverviewSceneOverrideDelegate
{
public:
    void on_exit_started() override { exit_started = true; }
    void on_workspace_selected(uint32_t id) override { selected_workspace = id; }
    void on_output_selected(int id) override { selected_output = id; }
    void on_done() override { done = true; }

    bool exit_started = false;
    bool done = false;
    std::optional<uint32_t> selected_workspace;
    std::optional<int> selected_output;
};

/// The shell component container needs one, but the overview never drives it.
class NoopShellDelegate : public ShellApplicationDelegate
{
public:
    void place_window(miral::WindowSpecification&) override { }
    void handle_ready(std::shared_ptr<Container> const&) override { }
};
}

/// One output carrying two workspaces, with whatever windows a test chooses to
/// put into the focus order.
class OverviewSceneOverrideTest : public Test
{
public:
    OverviewSceneOverrideTest() :
        session { std::make_shared<NiceMock<test::MockSession>>() },
        app { session }
    {
        workspaces.push_back(first_workspace);
        workspaces.push_back(second_workspace);

        for (auto const& workspace : { first_workspace, second_workspace })
        {
            ON_CALL(*workspace, get_output()).WillByDefault(Invoke([weak = std::weak_ptr(output)]
            {
                return std::static_pointer_cast<AbstractOutput>(weak.lock());
            }));
        }
        ON_CALL(*first_workspace, id()).WillByDefault(Return(1));
        ON_CALL(*second_workspace, id()).WillByDefault(Return(2));

        ON_CALL(*output, id()).WillByDefault(Return(1));
        ON_CALL(*output, is_defunct()).WillByDefault(Return(false));
        ON_CALL(*output, get_area()).WillByDefault(ReturnRef(OUTPUT_AREA));
        ON_CALL(*output, get_app_zones()).WillByDefault(ReturnRef(app_zones));
        ON_CALL(*output, get_workspaces()).WillByDefault(Invoke([this]
        {
            return workspaces;
        }));
        ON_CALL(*output, active()).WillByDefault(Invoke([weak = std::weak_ptr(first_workspace)]
        {
            return std::static_pointer_cast<AbstractWorkspace>(weak.lock());
        }));

        auto factory = std::make_unique<NiceMock<test::MockOutputFactory>>();
        ON_CALL(*factory, create(_, _, _))
            .WillByDefault(Return(std::static_pointer_cast<AbstractOutput>(output)));

        output_manager = std::make_shared<OutputManager>(std::move(factory));
        workspace_manager = std::make_shared<WorkspaceManager>(
            std::make_shared<WorkspaceObserverRegistrar>(), config, output_manager);
        output_manager->create("Output1", 1, OUTPUT_AREA, *workspace_manager);
        output_manager->focus(1);
    }

    std::unique_ptr<OverviewSceneOverride> create()
    {
        return OverviewSceneOverride::create(
            *output_manager, animator, window_controller, compositor_state, config, delegate);
    }

    /// A window that the overview treats as an ordinary toplevel: a tiled leaf,
    /// which is anchored and so never consults its [miral::WindowInfo].
    miral::Window add_toplevel()
    {
        auto const window = make_window();
        auto leaf = std::make_shared<LeafContainer>(
            first_workspace,
            window_controller,
            geom::Rectangle {
                { 0,   0   },
                { 400, 300 }
        },
            config,
            parent,
            compositor_state);
        leaf->associate_to_window(window);
        compositor_state->add(leaf);
        containers.push_back(leaf);
        return window;
    }

    /// Makes every window look like a wallpaper to the classification helpers:
    /// below the application layer and attached to all four edges.
    void report_windows_as_backgrounds()
    {
        miral::WindowSpecification spec;
        spec.depth_layer() = mir_depth_layer_background;
        spec.attached_edges() = static_cast<MirPlacementGravity>(
            mir_placement_gravity_north | mir_placement_gravity_south
            | mir_placement_gravity_east | mir_placement_gravity_west);
        background_info = std::make_unique<miral::WindowInfo>(make_window(), spec);

        ON_CALL(*window_controller, info_for(An<miral::Window const&>()))
            .WillByDefault(ReturnRef(*background_info));
    }

    /// A wallpaper: a shell component attached to every edge, below the
    /// application layer.
    miral::Window add_background()
    {
        auto const window = make_window();
        auto container = std::make_shared<ShellComponentContainer>(
            window,
            window_controller,
            std::make_shared<NoopShellDelegate>(),
            output_manager,
            compositor_state);
        compositor_state->add(container);
        containers.push_back(container);
        return window;
    }

protected:
    miral::Window make_window()
    {
        auto const surface = std::make_shared<NiceMock<test::MockSurface>>();
        surfaces.push_back(surface);
        return miral::Window { app, surface };
    }

    std::shared_ptr<Config> config = std::make_shared<test::StubConfiguration>();
    std::shared_ptr<test::MockWindowController> window_controller
        = std::make_shared<NiceMock<test::MockWindowController>>();
    std::shared_ptr<CompositorState> compositor_state = std::make_shared<CompositorState>();
    std::shared_ptr<Animator> animator = std::make_shared<Animator>();
    RecordingDelegate delegate;

    std::shared_ptr<test::MockOutput> output = std::make_shared<NiceMock<test::MockOutput>>();
    std::shared_ptr<test::MockWorkspace> first_workspace
        = std::make_shared<NiceMock<test::MockWorkspace>>();
    std::shared_ptr<test::MockWorkspace> second_workspace
        = std::make_shared<NiceMock<test::MockWorkspace>>();
    std::vector<std::shared_ptr<AbstractWorkspace>> workspaces;
    std::vector<miral::Zone> app_zones;
    std::shared_ptr<OutputManager> output_manager;
    std::shared_ptr<WorkspaceManager> workspace_manager;
    std::shared_ptr<test::MockParentContainer> parent
        = std::make_shared<NiceMock<test::MockParentContainer>>();
    std::shared_ptr<test::MockSession> session;
    miral::Application app;
    std::vector<std::shared_ptr<test::MockSurface>> surfaces;
    /// The compositor state holds its windows weakly, so someone has to own them.
    std::vector<std::shared_ptr<WindowContainer>> containers;
    std::unique_ptr<miral::WindowInfo> background_info;
};

TEST_F(OverviewSceneOverrideTest, RefusesToOpenOnAnEntirelyEmptyScene)
{
    // No windows and no furniture: there would be nothing to draw at any level.
    EXPECT_EQ(nullptr, create());
}

TEST_F(OverviewSceneOverrideTest, OpensWithNoWindowsWhenThereIsFurnitureToDraw)
{
    report_windows_as_backgrounds();

    add_background();

    // The workspace strip is the whole point of opening the overview on a
    // desktop with nothing running.
    EXPECT_NE(nullptr, create());
}

TEST_F(OverviewSceneOverrideTest, ClosingTheLastWindowKeepsTheOverviewUp)
{
    report_windows_as_backgrounds();

    add_background();
    auto const toplevel = add_toplevel();

    auto const override_ = create();
    ASSERT_NE(nullptr, override_);

    override_->handle_window_closed(toplevel);

    // An empty window strip is a perfectly good overview - the workspace strip
    // is still there to be reached.
    EXPECT_FALSE(delegate.done);
    EXPECT_FALSE(delegate.exit_started);
}

TEST_F(OverviewSceneOverrideTest, ClosingTheLastSurfaceOfAnyKindTearsTheOverviewDown)
{
    report_windows_as_backgrounds();

    auto const background = add_background();

    auto const override_ = create();
    ASSERT_NE(nullptr, override_);

    override_->handle_window_closed(background);

    // Nothing left to draw at any level, so the overview goes away.
    EXPECT_TRUE(delegate.done);
    EXPECT_TRUE(delegate.exit_started);
}
