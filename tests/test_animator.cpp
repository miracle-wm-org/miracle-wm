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

#include "animator.h"
#include "mock_animation.h"
#include <gtest/gtest.h>

using namespace miracle;

namespace
{
class StubAnimation : public Animation
{
public:
    StubAnimation(
        AnimationHandle handle,
        AnimationDefinition definition,
        mir::geometry::Rectangle const& from,
        mir::geometry::Rectangle const& to,
        mir::geometry::Rectangle const& current) :
        Animation(handle, definition, from, to, current)
    {
    }

    void on_tick(AnimationStepResult const& asr) override
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
    Animator animator;
    auto const handle = animator.register_animateable();
    AnimationDefinition definition {
        .type = AnimationType::slide,
        .function = EaseFunction::linear,
        .duration_seconds = 1
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
        mir::geometry::Rectangle(
            mir::geometry::Point(0, 0),
            mir::geometry::Size(0, 0)));
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
    Animator animator;
    auto const handle = animator.register_animateable();
    AnimationDefinition definition {
        .type = AnimationType::fade_in,
        .function = EaseFunction::linear,
        .duration_seconds = 1
    };
    auto const animation = std::make_shared<test::MockAnimation>(
        handle,
        definition,
        mir::geometry::Rectangle(
            mir::geometry::Point(0, 0),
            mir::geometry::Size(0, 0)),
        mir::geometry::Rectangle(
            mir::geometry::Point(600, 0),
            mir::geometry::Size(0, 0)),
        mir::geometry::Rectangle(
            mir::geometry::Point(0, 0),
            mir::geometry::Size(0, 0)));
    EXPECT_CALL(*animation, on_tick(OpacityIs(0.f)));
    animator.append(animation);
    EXPECT_CALL(*animation, on_tick(OpacityIs(0.75f)));
    animator.tick(0.75f);
}

TEST_F(AnimatorTest, CanUpdateOpacityFadeOut)
{
    Animator animator;
    auto const handle = animator.register_animateable();
    AnimationDefinition definition {
        .type = AnimationType::fade_out,
        .function = EaseFunction::linear,
        .duration_seconds = 1
    };
    auto const animation = std::make_shared<test::MockAnimation>(
        handle,
        definition,
        mir::geometry::Rectangle(
            mir::geometry::Point(0, 0),
            mir::geometry::Size(0, 0)),
        mir::geometry::Rectangle(
            mir::geometry::Point(600, 0),
            mir::geometry::Size(0, 0)),
        mir::geometry::Rectangle(
            mir::geometry::Point(0, 0),
            mir::geometry::Size(0, 0)));
    EXPECT_CALL(*animation, on_tick(OpacityIs(1.f)));
    animator.append(animation);
    EXPECT_CALL(*animation, on_tick(OpacityIs(0.25f)));
    animator.tick(0.75f);
}
