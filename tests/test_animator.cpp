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
#include "plugin_manager.h"
#include <cmath>
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
        false,
        1.f,
        BuiltInAnimationList { BuiltInAnimationDefinition {
            .type = BultInAnimationType::slide,
            .function = EaseFunction::linear,
        } }
    };
    bool was_called = false;
    animator.append(Animation(
        handle,
        definition,
        AnimationData {
            AnimateableEvent::window_open,
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
    },
        std::shared_ptr<PluginManager>()));
    animator.tick(0.16f);
    EXPECT_EQ(was_called, true);
}

TEST_F(AnimatorTest, CanUpdateOpacityFadeIn)
{
    Animator animator;
    auto const handle = animator.register_animateable();
    AnimationDefinition const definition {
        false,
        1.f,
        BuiltInAnimationList { BuiltInAnimationDefinition {
            .type = BultInAnimationType::fade,
            .function = EaseFunction::linear,
        } }
    };
    float opacity = -1.f;
    animator.append(Animation(
        handle,
        definition,
        AnimationData {
            AnimateableEvent::window_open,
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
        return false;
    },
        std::shared_ptr<PluginManager>()));
    animator.tick(0.f);
    EXPECT_THAT(opacity, testing::Eq(0.f));
    animator.tick(0.75f);
    EXPECT_THAT(opacity, testing::Eq(0.75f));
}

TEST_F(AnimatorTest, CanUpdateOpacityFadeOut)
{
    Animator animator;
    auto const handle = animator.register_animateable();
    AnimationDefinition const definition {
        false,
        1.f,
        BuiltInAnimationList { BuiltInAnimationDefinition {
            .type = BultInAnimationType::fade,
            .function = EaseFunction::linear,
        } }
    };
    float opacity = -1.f;
    animator.append(Animation(
        handle,
        definition,
        AnimationData {
            AnimateableEvent::window_open,
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
    },
        std::shared_ptr<PluginManager>()));
    animator.tick(0.f);
    EXPECT_THAT(opacity, testing::Eq(1.f));
    animator.tick(0.75f);
    EXPECT_THAT(opacity, testing::Eq(0.25f));
}

namespace
{
/// Drives a single built-in animation part and returns the uniform scale
/// factor reported at \p at_seconds of a one second animation.
float scale_at(BuiltInAnimationDefinition const& part, float at_seconds)
{
    Animator animator;
    auto const handle = animator.register_animateable();
    AnimationDefinition const definition {
        false, 1.f, BuiltInAnimationList { part }
    };
    float scale = -1.f;
    animator.append(Animation(
        handle,
        definition,
        AnimationData {
            AnimateableEvent::window_open,
            mir::geometry::Rectangle(
                mir::geometry::Point(0, 0),
                mir::geometry::Size(100, 100)),
            mir::geometry::Rectangle(
                mir::geometry::Point(0, 0),
                mir::geometry::Size(100, 100)),
            0, 1 },
        [&](AnimationFrameResult const& asr)
    {
        if (asr.transform)
            scale = (*asr.transform)[0][0];
    },
        std::shared_ptr<PluginManager>()));
    animator.tick(at_seconds);
    return scale;
}
}

TEST_F(AnimatorTest, GrowDefaultsToScalingFromNothing)
{
    // A scale of 0 is the default and must keep the historical behaviour of
    // growing all the way up from nothing.
    BuiltInAnimationDefinition part {
        .type = BultInAnimationType::grow,
        .function = EaseFunction::linear,
    };
    EXPECT_THAT(scale_at(part, 0.f), testing::FloatEq(0.f));
    EXPECT_THAT(scale_at(part, 0.5f), testing::FloatEq(0.5f));
}

TEST_F(AnimatorTest, GrowInterpolatesFromConfiguredScale)
{
    BuiltInAnimationDefinition part {
        .type = BultInAnimationType::grow,
        .function = EaseFunction::linear,
    };
    part.scale = 0.9f;
    EXPECT_THAT(scale_at(part, 0.f), testing::FloatEq(0.9f));
    EXPECT_THAT(scale_at(part, 0.5f), testing::FloatEq(0.95f));
}

TEST_F(AnimatorTest, ShrinkDefaultsToScalingToNothing)
{
    BuiltInAnimationDefinition part {
        .type = BultInAnimationType::shrink,
        .function = EaseFunction::linear,
    };
    EXPECT_THAT(scale_at(part, 0.f), testing::FloatEq(1.f));
    EXPECT_THAT(scale_at(part, 0.5f), testing::FloatEq(0.5f));
}

TEST_F(AnimatorTest, ShrinkInterpolatesToConfiguredScale)
{
    BuiltInAnimationDefinition part {
        .type = BultInAnimationType::shrink,
        .function = EaseFunction::linear,
    };
    part.scale = 0.92f;
    EXPECT_THAT(scale_at(part, 0.f), testing::FloatEq(1.f));
    EXPECT_THAT(scale_at(part, 0.5f), testing::FloatEq(0.96f));
}

class EaseTest : public testing::Test
{
};

TEST_F(EaseTest, EaseOutBounceSpansTheUnitInterval)
{
    BuiltInAnimationDefinition const definition {
        .type = BultInAnimationType::grow,
        .function = EaseFunction::ease_out_bounce,
    };
    EXPECT_THAT(ease(definition, 0.f), testing::FloatEq(0.f));
    EXPECT_THAT(ease(definition, 1.f), testing::FloatEq(1.f));
}

TEST_F(EaseTest, EaseOutBounceIsContinuousAcrossItsBranches)
{
    BuiltInAnimationDefinition const definition {
        .type = BultInAnimationType::grow,
        .function = EaseFunction::ease_out_bounce,
    };

    // Each branch boundary is the apex of a bounce, where the curve touches 1.
    // The previous implementation subtracted the phase offsets without dividing
    // them by d1, which left large discontinuities here.
    for (float const boundary : { 1.f / 2.75f, 2.f / 2.75f, 2.5f / 2.75f })
    {
        EXPECT_NEAR(ease(definition, boundary), 1.f, 1e-4f)
            << "at boundary " << boundary;
        EXPECT_NEAR(ease(definition, std::nextafter(boundary, 0.f)), 1.f, 1e-4f)
            << "just below boundary " << boundary;
    }
}

TEST_F(EaseTest, EaseOutBounceStaysWithinTheUnitInterval)
{
    BuiltInAnimationDefinition const definition {
        .type = BultInAnimationType::grow,
        .function = EaseFunction::ease_out_bounce,
    };
    for (int i = 0; i <= 100; i++)
    {
        float const t = static_cast<float>(i) / 100.f;
        float const value = ease(definition, t);
        EXPECT_GE(value, 0.f) << "at t=" << t;
        EXPECT_LE(value, 1.f) << "at t=" << t;
    }
}

TEST_F(EaseTest, EaseInBounceIsTheMirrorOfEaseOutBounce)
{
    BuiltInAnimationDefinition out_definition {
        .type = BultInAnimationType::grow,
        .function = EaseFunction::ease_out_bounce,
    };
    BuiltInAnimationDefinition in_definition = out_definition;
    in_definition.function = EaseFunction::ease_in_bounce;

    for (int i = 0; i <= 20; i++)
    {
        float const t = static_cast<float>(i) / 20.f;
        EXPECT_NEAR(ease(in_definition, t), 1.f - ease(out_definition, 1.f - t), 1e-5f)
            << "at t=" << t;
    }
}
