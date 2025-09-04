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

#ifndef MIRACLEWM_ANIMATOR_H
#define MIRACLEWM_ANIMATOR_H

#include <condition_variable>
#include <glm/glm.hpp>
#include <mir/geometry/rectangle.h>
#include <miracle/animation_definition.h>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace miracle
{
/// Unique handle provided to track animators
typedef uint32_t AnimationHandle;

/// Reserved for windows who lack an animation handle
extern const AnimationHandle none_animation_handle;

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
    bool const is_complete;

    /// The current rectangle set by the animation, if any.
    std::optional<mir::geometry::Rectangle> const rectangle;

    /// The current transform set by the animation, if any.
    std::optional<glm::mat4> const transform;

    /// The current opacity set by the animation, if any.
    std::optional<float> const opacity;
};

class Animation
{
public:
    Animation(
        AnimationHandle handle,
        AnimationDefinition definition,
        mir::geometry::Rectangle const& from,
        mir::geometry::Rectangle const& to,
        mir::geometry::Rectangle const& current);
    virtual ~Animation() = default;

    Animation& operator=(Animation const& other) = default;

    AnimationFrameResult init();
    AnimationFrameResult step(float dt);
    [[nodiscard]] AnimationHandle get_handle() const { return handle; }
    float get_runtime_seconds() const { return runtime_seconds; }
    void set_current_size(mir::geometry::Size const& size);
    void mark_for_removal();
    bool is_being_removed() const;
    virtual void on_tick(AnimationFrameResult const&) = 0;

private:
    AnimationHandle handle;
    AnimationDefinition definition;
    mir::geometry::Rectangle current;
    mir::geometry::Rectangle from;
    mir::geometry::Rectangle to;
    mir::geometry::Size real_size;
    float runtime_seconds = 0.f;
    bool should_leave_this_animator_for_the_great_animator_in_the_sky = false;
};

/// Manages the animation queue.
///
/// This class is used in conjunction with the [AnimatorLoop] which drives
/// this class.
///
/// If multiple animations are queued for a window,  then the latest animation
/// may override values from previous animations.
class Animator
{
public:
    /// Registers a new animation handle.
    ///
    /// Components that want to animate must provide a valid handler before
    /// doing so.
    AnimationHandle register_animateable();

    /// Run a frame of the animator.
    ///
    /// This is typically called by an [AnimatorLoop].
    void tick(float dt);

    /// Append a new animation to the queue.
    void append(std::shared_ptr<Animation> const& animation);
    void set_size_hack(AnimationHandle handle, mir::geometry::Size const& size);

    /// Remove an animation by its handle.
    void remove_by_animation_handle(AnimationHandle handle);

    [[nodiscard]] bool has_animations() const { return !active.empty(); }
    [[nodiscard]] bool is_animating(AnimationHandle handle);
    std::condition_variable& get_cv() { return cv; }
    std::mutex& get_lock() { return processing_lock; }

private:
    std::vector<std::shared_ptr<Animation>> active;
    std::thread run_thread;
    std::condition_variable cv;
    std::mutex processing_lock;
    AnimationHandle next_handle = 1;
};

} // miracle

#endif // MIRACLEWM_ANIMATOR_H
