/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "compositor_state.h"
#include "config.h"
#include "container_group_container.h"
#include "container_scope.h"
#include "leaf_container.h"
#include "mir/geometry/forward.h"
#include "mir_toolkit/common.h"
#include "miral/application_info.h"
#include "miral/window_specification.h"
#include "mock_output_factory.h"
#include "mock_parent_container.h"
#include "mock_session.h"
#include "mock_surface.h"
#include "mock_window_controller.h"
#include "mock_workspace.h"
#include "output_manager.h"
#include "parent_container.h"
#include "stub_configuration.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using namespace miracle;
namespace geom = mir::geometry;

class ParentContainerTest : public testing::Test
{
public:
    ParentContainerTest() :
        area(geom::Rectangle({ 0, 0 }, { 800, 600 })),
        container(state, window_controller, config, area, workspace.get(), nullptr, true)
    {
    }

    std::shared_ptr<CompositorState> state = std::make_shared<CompositorState>();
    std::shared_ptr<test::MockWindowController> window_controller = std::make_shared<testing::NiceMock<test::MockWindowController>>();
    std::shared_ptr<Config> config = std::make_shared<test::StubConfiguration>();
    std::unique_ptr<OutputManager> output_manager = std::make_unique<OutputManager>(std::make_unique<test::MockOutputFactory>());
    std::unique_ptr<test::MockWorkspace> workspace = std::make_unique<test::MockWorkspace>();
    geom::Rectangle area;
    ParentContainer container;
};