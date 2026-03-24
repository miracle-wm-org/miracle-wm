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

#ifndef MIRACLEWM_ANIMATOR_H
#define MIRACLEWM_ANIMATOR_H

#include "animation.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace mir
{
class ServerActionQueue;
}

namespace miracle
{
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
    void append(Animation&& animation);

    /// Append a new custom animation to the queue.
    void append(CustomAnimation&& animation);

    /// Remove an animation by its handle.
    void remove_by_animation_handle(AnimationHandle handle);

    /// Returns true if there are any animations queued or running.
    ///
    /// This is lock-free and safe to call from any thread.
    [[nodiscard]] bool has_animations() const { return animation_count.load(std::memory_order_acquire) > 0; }

    /// Returns true if an animation with the given handle is pending.
    ///
    /// Note: this only detects animations that are still in the pending queue.
    /// An animation that has already been promoted to the active list (i.e. is
    /// currently ticking) will not be detected here. Use this only for
    /// informational / early-out purposes.
    [[nodiscard]] bool is_animating(AnimationHandle handle);
    std::condition_variable& get_cv() { return cv; }
    std::mutex& get_lock() { return processing_lock; }

private:
    std::shared_ptr<mir::ServerActionQueue> server_action_queue;

    // AnimatorLoop-thread-only: never accessed under processing_lock.
    std::vector<Animation> active;
    std::vector<CustomAnimation> active_custom;

    // Shared, always accessed under processing_lock.
    std::vector<Animation> pending_active;
    std::vector<CustomAnimation> pending_active_custom;
    std::vector<AnimationHandle> pending_remove;

    // Count of all animations (pending + active). Incremented in append(),
    // decremented in tick() as animations are erased. Atomic so has_animations()
    // is safe to call from any thread without holding processing_lock.
    std::atomic<int> animation_count { 0 };

    std::thread run_thread;
    std::condition_variable cv;
    std::mutex processing_lock;
    AnimationHandle next_handle = 1;
};

} // miracle

#endif // MIRACLEWM_ANIMATOR_H
