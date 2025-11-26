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

#ifndef MIRACLE_WM_ANIMATION_H
#define MIRACLE_WM_ANIMATION_H

#include <glm/glm.hpp>
#include <mir/geometry/rectangle.h>
#include <miracle/cpp/animation_definition.h>
#include <optional>

namespace miracle
{
/// Unique handle provided to track animators
typedef uint32_t AnimationHandle;

/// Data provided when the animation ticks.
///
/// Individual [Animation]s can decide what they'd like to do with
/// this. For example, Mir windows may set their position according
/// to the provided rectangle on each frame, while outputs may change
/// their current workspace to show a different workspace.
struct AnimationFrameResult
{
    /// Whether this result marks the end of animation.
    ///
    /// Once set, the animation that produced this object will
    /// be removed.
    bool is_complete = false;

    /// The current rectangle set by the animation, if any.
    std::optional<mir::geometry::Rectangle> rectangle;

    /// The current transform set by the animation, if any.
    std::optional<glm::mat4> transform;

    /// The current opacity set by the animation, if any.
    std::optional<float> opacity;

    AnimationFrameResult merge(AnimationFrameResult const& other) const;
};

/// Interface that defines an animation.
class Animation
{
public:
    explicit Animation(AnimationHandle handle);
    virtual ~Animation() = default;
    [[nodiscard]] AnimationHandle handle() const;
    virtual void mark_for_removal();
    [[nodiscard]] virtual bool is_being_removed() const;
    virtual AnimationFrameResult tick(float dt) = 0;
    virtual void on_tick(AnimationFrameResult const&) = 0;

protected:
    float runtime_seconds = 0.f;

private:
    AnimationHandle handle_;
    bool is_being_removed_ = false;
};

class BuiltInAnimation : public Animation
{
public:
    BuiltInAnimation(
        AnimationHandle handle,
        float duration_seconds,
        BuiltInAnimationDefinition definition,
        mir::geometry::Rectangle const& from,
        mir::geometry::Rectangle const& to,
        float opacity_start,
        float opacity_end);

    AnimationFrameResult tick(float dt) override;

    /// TODO: We shouldn't provide an empty function implementation here, but
    ///  it is useful for MultiBuiltInAnimation
    void on_tick(AnimationFrameResult const& result) override { }

private:
    float duration_seconds;
    BuiltInAnimationDefinition definition;
    mir::geometry::Rectangle from;
    mir::geometry::Rectangle to;
    float const opacity_start;
    float const opacity_end;
};

class MultiBuiltInAnimation : public Animation
{
public:
    MultiBuiltInAnimation(
        AnimationHandle handle,
        AnimationDefinition const& definition,
        mir::geometry::Rectangle const& from,
        mir::geometry::Rectangle const& to,
        float opacity_start,
        float opacity_end);

    MultiBuiltInAnimation& operator=(MultiBuiltInAnimation const& other) = default;

    AnimationFrameResult tick(float dt) override;

private:
    std::vector<BuiltInAnimation> animations;
};

}

#endif // MIRACLE_WM_ANIMATION_H