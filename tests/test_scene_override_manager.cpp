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

#include "scene_override.h"

#include <gtest/gtest.h>
#include <thread>

using namespace miracle;

namespace
{
class StubSceneOverride : public SceneOverride
{
public:
    explicit StubSceneOverride(bool* destroyed = nullptr) :
        destroyed { destroyed }
    {
    }

    ~StubSceneOverride() override
    {
        if (destroyed)
            *destroyed = true;
    }

    void handle_keyboard_event(MirKeyboardEvent const*) override { }
    void handle_pointer_event(MirPointerEvent const*) override { }
    void place(
        mir::scene::Surface const&,
        mir::geometry::Rectangle const&,
        std::vector<SceneOverridePlacement>&) override
    {
    }
    void handle_output_changed() override { }

private:
    bool* destroyed;
};
}

TEST(SceneOverrideManagerTest, TryOverrideReturnsAToken)
{
    SceneOverrideManager manager;
    auto const token = manager.try_override(std::make_unique<StubSceneOverride>());
    ASSERT_TRUE(token.has_value());
}

TEST(SceneOverrideManagerTest, SecondOverrideIsRejectedWhileFirstIsActive)
{
    SceneOverrideManager manager;
    auto const first = manager.try_override(std::make_unique<StubSceneOverride>());
    ASSERT_TRUE(first.has_value());
    auto const second = manager.try_override(std::make_unique<StubSceneOverride>());
    EXPECT_FALSE(second.has_value());
}

TEST(SceneOverrideManagerTest, TryResolveReturnsNullWhenNoOverrideIsActive)
{
    SceneOverrideManager manager;
    EXPECT_EQ(manager.try_resolve(), nullptr);
}

TEST(SceneOverrideManagerTest, TryResolveReturnsTheActiveOverride)
{
    SceneOverrideManager manager;
    manager.try_override(std::make_unique<StubSceneOverride>());
    EXPECT_NE(manager.try_resolve(), nullptr);
}

TEST(SceneOverrideManagerTest, ReleaseWithWrongTokenFails)
{
    SceneOverrideManager manager;
    auto const token = manager.try_override(std::make_unique<StubSceneOverride>());
    ASSERT_TRUE(token.has_value());
    EXPECT_FALSE(manager.try_release_override(*token + 1));
    EXPECT_NE(manager.try_resolve(), nullptr);
}

TEST(SceneOverrideManagerTest, ReleaseWithCorrectTokenSucceeds)
{
    SceneOverrideManager manager;
    auto const token = manager.try_override(std::make_unique<StubSceneOverride>());
    ASSERT_TRUE(token.has_value());
    EXPECT_TRUE(manager.try_release_override(*token));
    EXPECT_EQ(manager.try_resolve(), nullptr);
}

TEST(SceneOverrideManagerTest, ReleaseWithStaleTokenFails)
{
    SceneOverrideManager manager;
    auto const first = manager.try_override(std::make_unique<StubSceneOverride>());
    ASSERT_TRUE(manager.try_release_override(*first));
    auto const second = manager.try_override(std::make_unique<StubSceneOverride>());
    ASSERT_TRUE(second.has_value());
    EXPECT_FALSE(manager.try_release_override(*first));
    EXPECT_NE(manager.try_resolve(), nullptr);
}

TEST(SceneOverrideManagerTest, ReleaseWithoutAnOverrideFails)
{
    SceneOverrideManager manager;
    EXPECT_FALSE(manager.try_release_override(0));
    EXPECT_FALSE(manager.try_release_override(1));
}

TEST(SceneOverrideManagerTest, NewOverrideAfterReleaseGetsANewToken)
{
    SceneOverrideManager manager;
    auto const first = manager.try_override(std::make_unique<StubSceneOverride>());
    manager.try_release_override(*first);
    auto const second = manager.try_override(std::make_unique<StubSceneOverride>());
    ASSERT_TRUE(second.has_value());
    EXPECT_NE(*first, *second);
}

TEST(SceneOverrideManagerTest, ResolvedPointerKeepsOverrideAliveAcrossRelease)
{
    SceneOverrideManager manager;
    bool destroyed = false;
    auto const token = manager.try_override(std::make_unique<StubSceneOverride>(&destroyed));

    auto const pinned = manager.try_resolve();
    ASSERT_NE(pinned, nullptr);

    EXPECT_TRUE(manager.try_release_override(*token));
    EXPECT_FALSE(destroyed) << "the pinned reference must keep the override alive";
    EXPECT_EQ(manager.try_resolve(), nullptr);
}

TEST(SceneOverrideManagerTest, OverrideIsDestroyedWhenLastPinDrops)
{
    SceneOverrideManager manager;
    bool destroyed = false;
    auto const token = manager.try_override(std::make_unique<StubSceneOverride>(&destroyed));

    {
        auto const pinned = manager.try_resolve();
        manager.try_release_override(*token);
    }

    EXPECT_TRUE(destroyed);
}

TEST(SceneOverrideManagerTest, ConcurrentResolveAndReleaseIsSafe)
{
    SceneOverrideManager manager;
    auto const token = manager.try_override(std::make_unique<StubSceneOverride>());

    // [place] never dereferences the surface; it is only ever a key.
    auto const surface = reinterpret_cast<mir::scene::Surface const*>(&manager);
    std::thread resolver([&manager, surface]
    {
        for (int i = 0; i < 1000; ++i)
        {
            std::vector<SceneOverridePlacement> placements;
            if (auto const resolved = manager.try_resolve())
                resolved->place(*surface, mir::geometry::Rectangle {}, placements);
        }
    });
    std::thread releaser([&manager, token]
    {
        manager.try_release_override(*token);
    });

    resolver.join();
    releaser.join();
    EXPECT_EQ(manager.try_resolve(), nullptr);
}
