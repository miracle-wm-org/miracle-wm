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
#include "geometry_helpers.h"
#include "plugin_manager.h"
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
AnimationDefinition const linear_slide {
    false,
    1.f,
    BuiltInAnimationList { BuiltInAnimationDefinition {
        .type = BultInAnimationType::slide,
        .function = EaseFunction::linear,
    } }
};

AnimationData slide_data(mir::geometry::Rectangle const& from, mir::geometry::Rectangle const& to)
{
    return AnimationData { AnimateableEvent::window_move, from, to, 1, 1 };
}
}

TEST_F(AnimatorTest, ShrinkingSlideRequestsTheFinalSizeOnItsFirstFrame)
{
    Animator animator;
    auto const handle = animator.register_animateable();
    std::optional<AnimationFrameResult> first_frame;
    animator.append(Animation(
        handle,
        linear_slide,
        slide_data(
            mir::geometry::Rectangle({ 0, 0 }, { 800, 600 }),
            mir::geometry::Rectangle({ 0, 0 }, { 400, 600 })),
        [&](AnimationFrameResult const& result)
    {
        if (!first_frame)
            first_frame = result;
    },
        std::shared_ptr<PluginManager>()));

    animator.tick(0.25f);

    ASSERT_TRUE(first_frame.has_value());
    ASSERT_TRUE(first_frame->rectangle.has_value());
    EXPECT_EQ(first_frame->rectangle->size, mir::geometry::Size(400, 600));
}

TEST_F(AnimatorTest, ShrinkingSlideStillClipsToTheInterpolatedSize)
{
    Animator animator;
    auto const handle = animator.register_animateable();
    std::optional<AnimationFrameResult> first_frame;
    animator.append(Animation(
        handle,
        linear_slide,
        slide_data(
            mir::geometry::Rectangle({ 0, 0 }, { 800, 600 }),
            mir::geometry::Rectangle({ 0, 0 }, { 400, 600 })),
        [&](AnimationFrameResult const& result)
    {
        if (!first_frame)
            first_frame = result;
    },
        std::shared_ptr<PluginManager>()));

    animator.tick(0.25f);

    ASSERT_TRUE(first_frame.has_value());
    ASSERT_TRUE(first_frame->clip_area.has_value());
    // A quarter of the way from 800 to 400.
    EXPECT_EQ(first_frame->clip_area->size, mir::geometry::Size(700, 600));
    ASSERT_TRUE(first_frame->resize.has_value());
    EXPECT_EQ(first_frame->resize->target, mir::geometry::Size(400, 600));
}

TEST_F(AnimatorTest, CompletionClearsTheResizeFrame)
{
    Animator animator;
    auto const handle = animator.register_animateable();
    std::optional<AnimationFrameResult> last_frame;
    animator.append(Animation(
        handle,
        linear_slide,
        slide_data(
            mir::geometry::Rectangle({ 0, 0 }, { 800, 600 }),
            mir::geometry::Rectangle({ 0, 0 }, { 400, 600 })),
        [&](AnimationFrameResult const& result)
    {
        last_frame = result;
    },
        std::shared_ptr<PluginManager>()));

    animator.tick(1.f);

    ASSERT_TRUE(last_frame.has_value());
    EXPECT_TRUE(last_frame->is_complete);
    EXPECT_FALSE(last_frame->resize.has_value());
    ASSERT_TRUE(last_frame->rectangle.has_value());
    EXPECT_EQ(last_frame->rectangle.value(), mir::geometry::Rectangle({ 0, 0 }, { 400, 600 }));
}

TEST_F(AnimatorTest, AResizeFrameCarriesThePreResizeSize)
{
    // The renderer has no way to measure how far a client's buffer overshoots its window
    // during a resize - the surface's size is what we asked for and the buffer is what the
    // client last drew. The pre-resize size is the one pairing that is free of that
    // latency, because at the moment the animation starts we have not asked for anything.
    Animator animator;
    auto const handle = animator.register_animateable();
    std::optional<AnimationFrameResult> first_frame;
    animator.append(Animation(
        handle,
        linear_slide,
        slide_data(
            mir::geometry::Rectangle({ 0, 0 }, { 800, 600 }),
            mir::geometry::Rectangle({ 0, 0 }, { 400, 600 })),
        [&](AnimationFrameResult const& result)
    {
        if (!first_frame)
            first_frame = result;
    },
        std::shared_ptr<PluginManager>()));

    animator.tick(0.25f);

    ASSERT_TRUE(first_frame.has_value());
    ASSERT_TRUE(first_frame->resize.has_value());
    EXPECT_EQ(first_frame->resize->source, mir::geometry::Size(800, 600));
}

TEST_F(AnimatorTest, ASizeChangeTooSmallToSeeIsNotAnimated)
{
    // A handful of pixels cannot read as motion, but it still costs a stretch and a crop on
    // every frame of the animation, and lands as jitter. The move still animates; only the
    // size snaps.
    Animator animator;
    auto const handle = animator.register_animateable();
    std::optional<AnimationFrameResult> first_frame;
    animator.append(Animation(
        handle,
        linear_slide,
        slide_data(
            mir::geometry::Rectangle({ 0, 0 }, { 800, 600 }),
            mir::geometry::Rectangle({ 400, 0 }, { 808, 604 })),
        [&](AnimationFrameResult const& result)
    {
        if (!first_frame)
            first_frame = result;
    },
        std::shared_ptr<PluginManager>()));

    animator.tick(0.25f);

    ASSERT_TRUE(first_frame.has_value());
    EXPECT_FALSE(first_frame->resize.has_value());

    // The clip takes the final size straight away rather than crawling towards it.
    ASSERT_TRUE(first_frame->clip_area.has_value());
    EXPECT_EQ(first_frame->clip_area->size, mir::geometry::Size(808, 604));
    EXPECT_EQ(first_frame->clip_area->top_left, mir::geometry::Point(100, 0));
}

TEST_F(AnimatorTest, ASizeChangeJustPastTheThresholdIsStillAnimated)
{
    Animator animator;
    auto const handle = animator.register_animateable();
    std::optional<AnimationFrameResult> first_frame;
    animator.append(Animation(
        handle,
        linear_slide,
        slide_data(
            mir::geometry::Rectangle({ 0, 0 }, { 800, 600 }),
            mir::geometry::Rectangle({ 0, 0 }, { 811, 600 })),
        [&](AnimationFrameResult const& result)
    {
        if (!first_frame)
            first_frame = result;
    },
        std::shared_ptr<PluginManager>()));

    animator.tick(0.25f);

    ASSERT_TRUE(first_frame.has_value());
    ASSERT_TRUE(first_frame->resize.has_value());
    ASSERT_TRUE(first_frame->clip_area.has_value());
    EXPECT_LT(first_frame->clip_area->size.width.as_int(), 811);
}

TEST_F(AnimatorTest, APureMoveGetsNoResizeFrame)
{
    // Nothing about the content changes when a window only moves, so there is nothing to
    // stretch.
    Animator animator;
    auto const handle = animator.register_animateable();
    std::optional<AnimationFrameResult> first_frame;
    animator.append(Animation(
        handle,
        linear_slide,
        slide_data(
            mir::geometry::Rectangle({ 0, 0 }, { 800, 600 }),
            mir::geometry::Rectangle({ 600, 0 }, { 800, 600 })),
        [&](AnimationFrameResult const& result)
    {
        if (!first_frame)
            first_frame = result;
    },
        std::shared_ptr<PluginManager>()));

    animator.tick(0.25f);

    ASSERT_TRUE(first_frame.has_value());
    EXPECT_FALSE(first_frame->resize.has_value());
}

TEST_F(AnimatorTest, ReplacingALiveAnimationContinuesFromItsCurrentState)
{
    Animator animator;
    auto const handle = animator.register_animateable();
    std::optional<AnimationFrameResult> outgoing_last;
    animator.append(Animation(
        handle,
        linear_slide,
        slide_data(
            mir::geometry::Rectangle({ 0, 0 }, { 100, 100 }),
            mir::geometry::Rectangle({ 600, 0 }, { 100, 100 })),
        [&](AnimationFrameResult const& result)
    {
        outgoing_last = result;
    },
        std::shared_ptr<PluginManager>()));

    animator.tick(0.5f);
    ASSERT_TRUE(outgoing_last.has_value());
    ASSERT_TRUE(outgoing_last->rectangle.has_value());
    ASSERT_EQ(outgoing_last->rectangle->top_left, mir::geometry::Point(300, 0));

    std::optional<AnimationFrameResult> incoming_first;
    animator.append(Animation(
        handle,
        linear_slide,
        slide_data(
            mir::geometry::Rectangle({ 0, 0 }, { 100, 100 }),
            mir::geometry::Rectangle({ 1000, 0 }, { 100, 100 })),
        [&](AnimationFrameResult const& result)
    {
        if (!incoming_first)
            incoming_first = result;
    },
        std::shared_ptr<PluginManager>()));

    animator.tick(0.f);

    ASSERT_TRUE(incoming_first.has_value());
    ASSERT_TRUE(incoming_first->rectangle.has_value());
    // Continuous with the cancelled animation, not a jump back to the caller's `from`.
    EXPECT_EQ(incoming_first->rectangle->top_left, mir::geometry::Point(300, 0));
}

TEST_F(AnimatorTest, TwoAppendsBetweenTicksCollapseToTheLast)
{
    Animator animator;
    auto const handle = animator.register_animateable();
    int first_calls = 0;
    int second_calls = 0;
    std::optional<AnimationFrameResult> second_last;

    animator.append(Animation(
        handle,
        linear_slide,
        slide_data(
            mir::geometry::Rectangle({ 0, 0 }, { 100, 100 }),
            mir::geometry::Rectangle({ 600, 0 }, { 100, 100 })),
        [&](AnimationFrameResult const&)
    {
        first_calls++;
    },
        std::shared_ptr<PluginManager>()));
    animator.append(Animation(
        handle,
        linear_slide,
        slide_data(
            mir::geometry::Rectangle({ 0, 0 }, { 100, 100 }),
            mir::geometry::Rectangle({ 1000, 0 }, { 100, 100 })),
        [&](AnimationFrameResult const& result)
    {
        second_calls++;
        second_last = result;
    },
        std::shared_ptr<PluginManager>()));

    animator.tick(1.f);

    EXPECT_EQ(first_calls, 0);
    EXPECT_GT(second_calls, 0);
    ASSERT_TRUE(second_last.has_value());
    EXPECT_EQ(second_last->rectangle.value(), mir::geometry::Rectangle({ 1000, 0 }, { 100, 100 }));
}
