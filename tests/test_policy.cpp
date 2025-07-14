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

#include "config_observer.h"
#include "display_config.h"
#include "output_listener.h"
#include "policy.h"
#include "stub_configuration.h"

#include <mir/graphics/default_display_configuration_policy.h>
#include <mir_test_framework/window_management_test_harness.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg = mir::graphics;

using namespace testing;

namespace
{
int argc = 1;
const char* argv[] = { "miracle-wm" };
}

// TODO: The proper graphics library is NOT packaged in the ubuntu archive,
//  so these tests can only run against a local install of mir. We should
//  take steps to remedy this ASAP!
class PolicyTest : public mir_test_framework::WindowManagementTestHarness
{
public:
    PolicyTest() :
        runner(argc, const_cast<const char**>(argv))
    {
        server.wrap_display_configuration_policy([&](std::shared_ptr<mg::DisplayConfigurationPolicy> const&)
                                                     -> std::shared_ptr<mg::DisplayConfigurationPolicy>
        {
            return std::make_shared<mir::graphics::SideBySideDisplayConfigurationPolicy>();
        });
    }

    void SetUp() override
    {
        launcher(server);
        WindowManagementTestHarness::SetUp();
    }

    auto get_builder() -> mir_test_framework::WindowManagementPolicyBuilder override
    {
        return [&](miral::WindowManagerTools const& tools)
        {
            return std::make_unique<miracle::Policy>(
                tools,
                server,
                runner,
                launcher,
                std::make_shared<miracle::test::StubConfiguration>(),
                compositor_state,
                std::make_shared<miracle::OutputListenerMultiplexer>(),
                std::make_shared<miracle::DisplayConfig>(),
                std::make_shared<miracle::ConfigObserverRegistrar>());
        };
    }

    auto get_initial_output_configs() -> std::vector<mir::graphics::DisplayConfigurationOutput> override
    {
        auto r = output_configs_from_output_rectangles({
            mir::geometry::Rectangle { { 0, 0 },   { 800, 600 }  },
            mir::geometry::Rectangle { { 800, 0 }, { 1000, 600 } }
        });
        return r;
    }

    miral::ExternalClientLauncher launcher;
    miral::MirRunner runner;
    std::shared_ptr<miracle::CompositorState> compositor_state = std::make_shared<miracle::CompositorState>();
};

TEST_F(PolicyTest, default_window_is_tiling_window)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    auto const window = create_window(app, spec);
    EXPECT_THAT(compositor_state->first_tiling(), Ne(nullptr));
    EXPECT_THAT(compositor_state->first_tiling()->window(), Eq(window));
}

TEST_F(PolicyTest, can_remove_output_with_containers_open_on_it)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    auto const window = create_window(app, spec);
    auto const container = compositor_state->containers()[0].lock();
    EXPECT_THAT(container->get_logical_area(), Eq(mir::geometry::Rectangle {
                                                   { 800,  0   },
                                                   { 1000, 600 }
    }));
    update_outputs(output_configs_from_output_rectangles({
        mir::geometry::Rectangle { { 0, 0 }, { 800, 600 } }
    }));

    EXPECT_THAT(container->get_logical_area(), Eq(mir::geometry::Rectangle {
                                                   { 0,   0   },
                                                   { 800, 600 }
    }));

    update_outputs(output_configs_from_output_rectangles({
        mir::geometry::Rectangle { { 0, 0 },   { 800, 600 }  },
        mir::geometry::Rectangle { { 800, 0 }, { 1000, 600 } }
    }));

    EXPECT_THAT(container->get_logical_area(), Eq(mir::geometry::Rectangle {
                                                   { 0,   0   },
                                                   { 800, 600 }
    }));
}

TEST_F(PolicyTest, can_remove_all_outputs_and_readd_it)
{
    auto const app = open_application("test");
    miral::WindowSpecification spec;
    auto const window = create_window(app, spec);
    auto const container = compositor_state->containers()[0].lock();
    update_outputs(output_configs_from_output_rectangles({}));

    update_outputs(output_configs_from_output_rectangles({
        mir::geometry::Rectangle { { 0, 0 }, { 400, 300 } }
    }));

    EXPECT_THAT(container->get_logical_area(), Eq(mir::geometry::Rectangle {
                                                   { 0,   0   },
                                                   { 400, 300 }
    }));
}
