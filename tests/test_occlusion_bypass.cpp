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

#include "occlusion_bypass.h"

#include "mock_container.h"
#include "mock_session.h"
#include "mock_surface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <mir/scene/surface.h>

using namespace miracle;
using namespace testing;

namespace
{
using MockContainerPtr = std::shared_ptr<NiceMock<test::MockContainer>>;
}

class OcclusionBypassTest : public Test
{
public:
    OcclusionBypassTest() :
        session { std::make_shared<NiceMock<test::MockSession>>() },
        app { session }
    {
    }

protected:
    /// A container wrapping a window of its own, so that each one has a
    /// distinct surface for [OcclusionBypass::forget] to key on.
    MockContainerPtr bypassable()
    {
        auto const surface = std::make_shared<NiceMock<test::MockSurface>>();
        surfaces.push_back(surface);

        auto container = std::make_shared<NiceMock<test::MockContainer>>();
        ON_CALL(*container, window())
            .WillByDefault(Return(std::optional {
                miral::Window { app, surface }
        }));
        return container;
    }

    static mir::scene::Surface const* key_of(MockContainerPtr const& container)
    {
        auto const window = container->window().value();
        return window.operator std::shared_ptr<mir::scene::Surface>().get();
    }

    std::shared_ptr<test::MockSession> session;
    miral::Application app;
    /// The windows hold their surfaces weakly, so someone has to own them.
    std::vector<std::shared_ptr<test::MockSurface>> surfaces;
};

TEST_F(OcclusionBypassTest, AcquireFlagsEveryContainer)
{
    auto const first = bypassable();
    auto const second = bypassable();

    OcclusionBypass bypass;
    bypass.acquire({ first, second });

    EXPECT_TRUE(bypass.held());
    EXPECT_TRUE(first->occlusion_bypass());
    EXPECT_TRUE(second->occlusion_bypass());
}

TEST_F(OcclusionBypassTest, AcquireIsAdditiveAndIdempotent)
{
    auto const first = bypassable();
    auto const second = bypassable();

    OcclusionBypass bypass;
    bypass.acquire({ first });
    bypass.acquire({ first, second });

    EXPECT_TRUE(first->occlusion_bypass());
    EXPECT_TRUE(second->occlusion_bypass());

    // Both are remembered exactly once, so one release puts both back.
    bypass.release();
    EXPECT_FALSE(first->occlusion_bypass());
    EXPECT_FALSE(second->occlusion_bypass());
}

TEST_F(OcclusionBypassTest, ReleaseClearsEveryContainer)
{
    auto const first = bypassable();
    auto const second = bypassable();

    OcclusionBypass bypass;
    bypass.acquire({ first, second });
    bypass.release();

    EXPECT_FALSE(bypass.held());
    EXPECT_FALSE(first->occlusion_bypass());
    EXPECT_FALSE(second->occlusion_bypass());
}

TEST_F(OcclusionBypassTest, ReleaseIsIdempotent)
{
    auto const container = bypassable();

    OcclusionBypass bypass;
    bypass.acquire({ container });
    bypass.release();

    // A second release must not undo a bypass somebody else has since applied.
    container->set_occlusion_bypass(true);
    bypass.release();

    EXPECT_TRUE(container->occlusion_bypass());
}

TEST_F(OcclusionBypassTest, DestructorReleases)
{
    auto const container = bypassable();

    {
        OcclusionBypass bypass;
        bypass.acquire({ container });
        EXPECT_TRUE(container->occlusion_bypass());
    }

    EXPECT_FALSE(container->occlusion_bypass());
}

TEST_F(OcclusionBypassTest, AContainerThatDiedIsSkipped)
{
    auto container = bypassable();
    auto const survivor = bypassable();

    OcclusionBypass bypass;
    bypass.acquire({ container, survivor });

    // The surface is dying with it, and its transform is no longer ours.
    container.reset();

    bypass.release();
    EXPECT_FALSE(survivor->occlusion_bypass());
}

TEST_F(OcclusionBypassTest, ForgetDropsAContainerWithoutClearingIt)
{
    auto const container = bypassable();
    auto const other = bypassable();

    OcclusionBypass bypass;
    bypass.acquire({ container, other });

    bypass.forget(key_of(container));

    bypass.release();

    // Forgotten, so the release left it exactly as it was.
    EXPECT_TRUE(container->occlusion_bypass());
    EXPECT_FALSE(other->occlusion_bypass());
}
