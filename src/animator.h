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
#include <miracle/cpp/animation_definition.h>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace mir
{
class ServerActionQueue;
}

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
    virtual AnimationFrameResult init() = 0;
    virtual AnimationFrameResult step(float dt) = 0;
    [[nodiscard]] AnimationHandle get_handle() const;
    virtual void mark_for_removal();
    [[nodiscard]] virtual bool is_being_removed() const;
    virtual void on_tick(AnimationFrameResult const&) = 0;

protected:
    float runtime_seconds = 0.f;

private:
    AnimationHandle handle;
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
        mir::geometry::Rectangle const& current,
        float opacity_start,
        float opacity_end);

    AnimationFrameResult init() override;
    AnimationFrameResult step(float dt) override;

    /// TODO: We shouldn't provide an empty function implementation here, but
    ///  it is useful for MultiBuiltInAnimation
    void on_tick(AnimationFrameResult const& result) override { }

private:
    float duration_seconds;
    BuiltInAnimationDefinition definition;
    mir::geometry::Rectangle current;
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
        mir::geometry::Rectangle const& current,
        float opacity_start,
        float opacity_end);

    MultiBuiltInAnimation& operator=(MultiBuiltInAnimation const& other) = default;

    AnimationFrameResult init() override;
    AnimationFrameResult step(float dt) override;

private:
    std::vector<BuiltInAnimation> animations;
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
    explicit Animator(std::shared_ptr<mir::ServerActionQueue> const& server_action_queue);

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

    /// Remove an animation by its handle.
    void remove_by_animation_handle(AnimationHandle handle);

    [[nodiscard]] bool has_animations() const { return !active.empty(); }
    [[nodiscard]] bool is_animating(AnimationHandle handle);
    std::condition_variable& get_cv() { return cv; }
    std::mutex& get_lock() { return processing_lock; }

private:
    std::shared_ptr<mir::ServerActionQueue> server_action_queue;
    std::vector<std::shared_ptr<Animation>> active;
    std::thread run_thread;
    std::condition_variable cv;
    std::mutex processing_lock;
    AnimationHandle next_handle = 1;
};

} // miracle

#endif // MIRACLEWM_ANIMATOR_H
