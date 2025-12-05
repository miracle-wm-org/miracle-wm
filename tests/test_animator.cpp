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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace miracle;

class AnimatorTest : public testing::Test
{
};

TEST_F(AnimatorTest, CanStepLinearSlideAnimation)
{
    Animator animator;
    auto const handle = animator.register_animateable();
    AnimationDefinition const definition {
        AnimationType::built_in,
        false,
        1.f,
        { BuiltInAnimationDefinition {
            .type = BultInAnimationType::slide,
            .function = EaseFunction::linear,
        } }
    };
    bool was_called = false;
    auto const animation = std::make_shared<Animation>(
        handle,
        definition,
        AnimationData {
            mir::geometry::Rectangle(
                mir::geometry::Point(0, 0),
                mir::geometry::Size(0, 0)),
            mir::geometry::Rectangle(
                mir::geometry::Point(600, 0),
                mir::geometry::Size(0, 0)),
            0, 1 },
        [&](AnimationFrameResult const&)
    {
        was_called = true;
    });
    animator.append(animation);
    animator.tick(0.16f);
    EXPECT_EQ(was_called, true);
}

TEST_F(AnimatorTest, CanUpdateOpacityFadeIn)
{
    Animator animator;
    auto const handle = animator.register_animateable();
    AnimationDefinition const definition {
        AnimationType::built_in,
        false,
        1.f,
        { BuiltInAnimationDefinition {
            .type = BultInAnimationType::fade,
            .function = EaseFunction::linear,
        } }
    };
    float opacity = -1.f;
    auto const animation = std::make_shared<Animation>(
        handle,
        definition,
        AnimationData {
            mir::geometry::Rectangle(
                mir::geometry::Point(0, 0),
                mir::geometry::Size(0, 0)),
            mir::geometry::Rectangle(
                mir::geometry::Point(600, 0),
                mir::geometry::Size(0, 0)),
            0, 1 },
        [&](AnimationFrameResult const& asr)
    {
        opacity = asr.opacity.value();
    });
    animator.append(animation);
    EXPECT_THAT(opacity, testing::Eq(0.f));
    animator.tick(0.75f);
    EXPECT_THAT(opacity, testing::Eq(0.75f));
}

TEST_F(AnimatorTest, CanUpdateOpacityFadeOut)
{
    Animator animator;
    auto const handle = animator.register_animateable();
    AnimationDefinition const definition {
        AnimationType::built_in,
        false,
        1.f,
        { BuiltInAnimationDefinition {
            .type = BultInAnimationType::fade,
            .function = EaseFunction::linear,
        } }
    };
    float opacity = -1.f;
    auto const animation = std::make_shared<Animation>(
        handle,
        definition,
        AnimationData {
            mir::geometry::Rectangle(
                mir::geometry::Point(0, 0),
                mir::geometry::Size(0, 0)),
            mir::geometry::Rectangle(
                mir::geometry::Point(600, 0),
                mir::geometry::Size(0, 0)),
            1,
            0,
        },
        [&](AnimationFrameResult const& asr)
    {
        opacity = asr.opacity.value();
    });
    animator.append(animation);
    EXPECT_THAT(opacity, testing::Eq(1.f));
    animator.tick(0.75f);
    EXPECT_THAT(opacity, testing::Eq(0.25f));
}
