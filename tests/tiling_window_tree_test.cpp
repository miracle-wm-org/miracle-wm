/**
Copyright (C) 2024  Matthew Kosarek

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

#include "tiling_window_tree.h"
#include "compositor_state.h"
#include "window_controller.h"
#include "leaf_container.h"
#include "window_metadata.h"
#include "stub_configuration.h"
#include "stub_session.h"
#include "stub_surface.h"
#include <gtest/gtest.h>
#include <miral/window_management_options.h>

using namespace miracle;

class SimpleTilingWindowTreeInterface : public TilingWindowTreeInterface
{
public:
    geom::Rectangle const& get_area() override
    {
        return r;
    }

    std::vector<miral::Zone> const& get_zones()  override
    {
        return zones;
    }

private:
    geom::Rectangle r{
        geom::Point(0, 0),
        geom::Size(1280, 720)
    };
    std::vector<miral::Zone> zones = {r};
};

class StubWindowController : public miracle::WindowController
{
public:
    StubWindowController(std::vector<std::pair<miral::Window, std::shared_ptr<WindowMetadata>>>& pairs)
        : pairs{pairs} {}
    bool is_fullscreen(miral::Window const&) override
    {
        return false;
    }
    void set_rectangle(miral::Window const&, geom::Rectangle const&, geom::Rectangle const&) override {}
    MirWindowState get_state(miral::Window const&) override
    {
        return mir_window_state_restored;
    }

    void change_state(miral::Window const&, MirWindowState state) override {}
    void clip(miral::Window const&, geom::Rectangle const&) override {}
    void noclip(miral::Window const&) override {}
    void select_active_window(miral::Window const&) override {}
    std::shared_ptr<WindowMetadata> get_metadata(miral::Window const& window) override
    {
        for (auto const& p : pairs)
        {
            if (p.first == window)
                return p.second;
        }
        return nullptr;
    }
    std::shared_ptr<WindowMetadata> get_metadata(miral::Window const& window, TilingWindowTree const*) override
    {
        for (auto const& p : pairs)
        {
            if (p.first == window)
                return p.second;
        }
        return nullptr;
    }

    void raise(miral::Window const&) override {}
    void send_to_back(miral::Window const&) override {}
    void open(miral::Window const&) override {}
    void close(miral::Window const&) override {}
    void on_animation(miracle::AnimationStepResult const& result, std::shared_ptr<WindowMetadata> const&) override {}
    void set_user_data(miral::Window const&, std::shared_ptr<void> const&) override {}
    void modify(miral::Window const&, miral::WindowSpecification const&) override {}
    miral::WindowInfo& info_for(miral::Window const&) override {}

private:
    std::vector<std::pair<miral::Window, std::shared_ptr<WindowMetadata>>>& pairs;
};

class TilingWindowTreeTest : public testing::Test
{
public:
    TilingWindowTreeTest()
        : tree(
            std::make_unique<SimpleTilingWindowTreeInterface>(),
            window_controller,
            state,
            std::make_shared<test::StubConfiguration>()
        )
    {
    }

    std::shared_ptr<LeafContainer> create_leaf()
    {
        miral::WindowSpecification spec;
        spec = tree.allocate_position(spec);

        auto session = std::make_shared<test::StubSession>();
        sessions.push_back(session);
        auto surface = std::make_shared<test::StubSurface>();
        surfaces.push_back(surface);
        
        miral::Window window(session, surface);
        miral::WindowInfo info(window, spec);
        auto metadata = std::make_shared<WindowMetadata>(WindowType::tiled, window);
        pairs.push_back({window, metadata});
        info.userdata(metadata);

        auto leaf = tree.advise_new_window(info);
        metadata->associate_to_node(leaf);

        state.active_window = window;
        tree.advise_focus_gained(info.window());
        return leaf;
    }

    CompositorState state;
    std::vector<std::shared_ptr<test::StubSession>> sessions;
    std::vector<std::shared_ptr<test::StubSurface>> surfaces;
    std::vector<std::pair<miral::Window, std::shared_ptr<WindowMetadata>>> pairs;
    StubWindowController window_controller{pairs};
    TilingWindowTree tree;
};


TEST_F(TilingWindowTreeTest, can_add_single_window_without_border_and_gaps)
{
    auto leaf = create_leaf();
    ASSERT_EQ(leaf->get_logical_area().size, geom::Size(1280, 720));
    ASSERT_EQ(leaf->get_logical_area().top_left, geom::Point(0, 0));
}

TEST_F(TilingWindowTreeTest, can_add_two_windows_horizontally_without_border_and_gaps)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();

    ASSERT_EQ(leaf1->get_logical_area().size, geom::Size(1280 / 2.f, 720));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, 0));

    ASSERT_EQ(leaf2->get_logical_area().size, geom::Size(1280 / 2.f, 720));
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(1280 / 2.f, 0));
}

TEST_F(TilingWindowTreeTest, can_add_two_windows_vertically_without_border_and_gaps)
{
    auto leaf1 = create_leaf();

    tree.request_vertical();

    auto leaf2 = create_leaf();
    ASSERT_EQ(leaf1->get_logical_area().size, geom::Size(1280, 720 / 2.f));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, 0));

    ASSERT_EQ(leaf2->get_logical_area().size, geom::Size(1280, 720 / 2.f));
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(0, 720/2.f));
}

TEST_F(TilingWindowTreeTest, can_add_three_windows_horizontally_without_border_and_gaps)
{
    auto leaf1 = create_leaf();
    auto leaf2 = create_leaf();
    auto leaf3 = create_leaf();

    ASSERT_EQ(leaf1->get_logical_area().size, geom::Size(ceilf(1280 / 3.f), 720));
    ASSERT_EQ(leaf1->get_logical_area().top_left, geom::Point(0, 0));

    ASSERT_EQ(leaf2->get_logical_area().size, geom::Size(ceilf(1280 / 3.f), 720));
    ASSERT_EQ(leaf2->get_logical_area().top_left, geom::Point(ceilf(1280 / 3.f), 0));

    ASSERT_EQ(leaf3->get_logical_area().size, geom::Size(floorf(1280 / 3.f), 720));
    ASSERT_EQ(leaf3->get_logical_area().top_left, geom::Point(floorf(1280 * (2.f / 3.f)) - 1, 0));
}