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
#include "mock_animation.h"
#include "passthrough_server_action_queue.h"
#include <gtest/gtest.h>

using namespace miracle;

namespace
{
class StubAnimation : public BuiltInAnimation
{
public:
    StubAnimation(
        AnimationHandle handle,
        BuiltInAnimationDefinition definition,
        mir::geometry::Rectangle const& from,
        mir::geometry::Rectangle const& to,
        float opacity_start,
        float opacity_end) :
        BuiltInAnimation(handle, 1, definition, from, to, opacity_start, opacity_end)
    {
    }

    void on_tick(AnimationFrameResult const& asr) override
    {
        was_called = true;
    }

    bool was_called = false;
};
}

class AnimatorTest : public testing::Test
{
};

TEST_F(AnimatorTest, CanStepLinearSlideAnimation)
{
    Animator animator(std::make_shared<PassthroughServerActionQueue>());
    auto const handle = animator.register_animateable();
    BuiltInAnimationDefinition definition {
        .type = BultInAnimationType::slide,
        .function = EaseFunction::linear,
    };
    auto const animation = std::make_shared<StubAnimation>(
        handle,
        definition,
        mir::geometry::Rectangle(
            mir::geometry::Point(0, 0),
            mir::geometry::Size(0, 0)),
        mir::geometry::Rectangle(
            mir::geometry::Point(600, 0),
            mir::geometry::Size(0, 0)),
        0, 1);
    animator.append(animation);
    animator.tick(0.16f);
    EXPECT_EQ(animation->was_called, true);
}

MATCHER_P(OpacityIs, expected_opacity, "")
{
    return arg.opacity == expected_opacity;
}

TEST_F(AnimatorTest, CanUpdateOpacityFadeIn)
{
    Animator animator(std::make_shared<PassthroughServerActionQueue>());
    auto const handle = animator.register_animateable();
    BuiltInAnimationDefinition definition {
        .type = BultInAnimationType::fade,
        .function = EaseFunction::linear,
    };
    auto const animation = std::make_shared<test::MockAnimation>(
        handle,
        1,
        definition,
        mir::geometry::Rectangle(
            mir::geometry::Point(0, 0),
            mir::geometry::Size(0, 0)),
        mir::geometry::Rectangle(
            mir::geometry::Point(600, 0),
            mir::geometry::Size(0, 0)),
        0,
        1);
    EXPECT_CALL(*animation, on_tick(OpacityIs(0.f)));
    animator.append(animation);
    EXPECT_CALL(*animation, on_tick(OpacityIs(0.75f)));
    animator.tick(0.75f);
}

TEST_F(AnimatorTest, CanUpdateOpacityFadeOut)
{
    Animator animator(std::make_shared<PassthroughServerActionQueue>());
    auto const handle = animator.register_animateable();
    BuiltInAnimationDefinition definition {
        .type = BultInAnimationType::fade,
        .function = EaseFunction::linear,
    };
    auto const animation = std::make_shared<test::MockAnimation>(
        handle,
        1,
        definition,
        mir::geometry::Rectangle(
            mir::geometry::Point(0, 0),
            mir::geometry::Size(0, 0)),
        mir::geometry::Rectangle(
            mir::geometry::Point(600, 0),
            mir::geometry::Size(0, 0)),
        1, 0);
    EXPECT_CALL(*animation, on_tick(OpacityIs(1.f)));
    animator.append(animation);
    EXPECT_CALL(*animation, on_tick(OpacityIs(0.25f)));
    animator.tick(0.75f);
}
