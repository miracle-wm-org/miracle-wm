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

#include "plugin_manager.h"
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <mir/geometry/rectangle.h>
#include <miracle/cpp/animation_definition.h>
#include <optional>
#include <string>

namespace miracle
{
class PluginManager;

/// Unique handle provided to track animators
typedef uint32_t AnimationHandle;

/// Evaluates \p definition's easing function at normalized time \p t in [0, 1].
float ease(BuiltInAnimationDefinition const& definition, float t);

/// The fraction of a resize animation that the pre-resize frame is cross-faded out over.
/// The fade has to be finished well before the animation is, so that the last animated
/// frames are pure live content and the hand-off to the un-animated frame after them
/// changes nothing on screen.
constexpr float kGhostFraction = 0.55f;

struct AnimationData
{
    AnimateableEvent event;
    mir::geometry::Rectangle area_start;
    mir::geometry::Rectangle area_end;
    float opacity_start;
    float opacity_end;
    /// Optional window being animated (set for window_open, window_move, window_close).
    /// The `internal` field is the stable window ID, usable for host calls.
    std::optional<miracle_window_info_t> window_info = {};
    std::optional<std::string> window_name = {};
    /// Optional workspace of the animated object (set for workspace_switch events).
    std::optional<miracle_workspace_t> workspace = {};
    std::optional<std::string> workspace_name = {};
};

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

    /// Separate scissor-clip region for slide animations.
    /// When set, process_animation() uses this for clip() instead of rectangle,
    /// so rectangle can carry the final target size for modify_window().
    std::optional<mir::geometry::Rectangle> clip_area;

    /// The size we asked the client to adopt. This only *gates* the stretch; the
    /// size the content is actually drawn at each frame comes from `clip_area`,
    /// clamped downstream to what the client can reach. Left unset by `finish()`,
    /// which is what stops the stretch on the final frame - by then the clamped
    /// clip has landed on the client's own size, so the scale is already 1.0 and
    /// switching it off is invisible.
    std::optional<mir::geometry::Size> fit_target;

    /// The window size the animation started from. The renderer draws whichever buffer
    /// the client has actually committed, and for the first frames of a resize that is
    /// still the pre-resize one - so this is the size that buffer has to be measured
    /// against until the client catches up. Unset by `finish()` along with `fit_target`.
    std::optional<mir::geometry::Size> fit_from;

    /// How much of the pre-resize frame is still showing. 1 on the first frame, easing to
    /// exactly 0 well before the animation ends so the last animated frames are pure live
    /// content. Unset for animations that do not change the window's size - there is
    /// nothing for a ghost to hide - and unset by finish() along with the fit sizes.
    std::optional<float> content_fade;

    AnimationFrameResult merge(AnimationFrameResult const& other) const;
};

/// A free-form animation driven by an arbitrary callback.
///
/// The callback receives the frame delta in seconds and returns
/// `true` when the animation is complete.
class CustomAnimation
{
public:
    CustomAnimation(
        AnimationHandle handle,
        std::function<bool(float dt)>&& on_tick);
    [[nodiscard]] AnimationHandle handle() const;
    void mark_for_removal();
    [[nodiscard]] bool is_being_removed() const;
    bool tick(float dt);

private:
    AnimationHandle handle_;
    std::function<bool(float dt)> on_tick_;
    bool is_being_removed_ = false;
};

/// An animation managed by the #Animator.
///
/// When an animation is created, it is provided with an
/// #AnimationDefinition that defines how the animation should
/// behave.
class Animation
{
public:
    /// Construct a new animation.
    ///
    /// \param handle The handle of the animated object.
    /// \param definition The definition of the animation.
    /// \param data The data for the animation.
    Animation(
        AnimationHandle handle,
        AnimationDefinition const& definition,
        AnimationData&& data,
        std::function<void(AnimationFrameResult const&)>&& on_tick,
        std::shared_ptr<PluginManager> const& plugin_manager);
    virtual ~Animation() = default;
    [[nodiscard]] AnimationHandle handle() const;
    virtual void mark_for_removal();
    [[nodiscard]] virtual bool is_being_removed() const;
    bool tick(float dt);

    /// The interpolated rectangle this animation is currently displaying.
    ///
    /// This is the clip rectangle, not the requested window rectangle, since
    /// the clip is what the user actually sees.
    ///
    /// \returns the current area, or nothing for definitions with no positional part
    [[nodiscard]] std::optional<mir::geometry::Rectangle> current_area() const;

    /// The interpolated opacity this animation is currently displaying.
    [[nodiscard]] float current_opacity() const;

    /// Restart this animation from the given visual state instead of its configured start.
    ///
    /// Used when a new animation replaces one that is still in flight, so the
    /// replacement continues from what is on screen rather than snapping back
    /// to the caller's starting rectangle.
    void retarget_from(mir::geometry::Rectangle const& area, float opacity);

private:
    AnimationFrameResult tick_built_in(BuiltInAnimationDefinition const& builtin_def, float t);
    AnimationFrameResult finish() const;

    float runtime_seconds = 0.f;
    AnimationHandle handle_;
    AnimationDefinition definition_;
    AnimationData data_;
    std::function<void(AnimationFrameResult const&)> on_tick;
    bool is_being_removed_ = false;
    std::shared_ptr<PluginManager> plugin_manager;
};
}

#endif // MIRACLE_WM_ANIMATION_H