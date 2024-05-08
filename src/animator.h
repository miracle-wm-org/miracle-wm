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

#include <thread>
#include <mutex>
#include <glm/glm.hpp>
#include <condition_variable>
#include <functional>
#include <mir/geometry/rectangle.h>

namespace mir
{
class ServerActionQueue;
}

namespace miracle
{

enum class AnimationType
{
    move_lerp
};

typedef uint32_t AnimationHandle;

/// Reserved for windows who lack an animation handle
extern const AnimationHandle none_animation_handle;

struct AnimationStepResult
{
    AnimationHandle handle;
    bool should_erase = false;
    glm::vec2 position;
    glm::vec2 size;
    glm::mat4 transform;
};

class Animation
{
public:
    static Animation move_lerp(
        AnimationHandle handle,
        mir::geometry::Rectangle const& from,
        mir::geometry::Rectangle const& to,
        std::function<void(AnimationStepResult const&)> const& callback);
    Animation& operator=(Animation const& other);

    AnimationStepResult step();
    [[nodiscard]] std::function<void(AnimationStepResult const&)> const& get_callback() const { return callback; }
    [[nodiscard]] AnimationHandle get_handle() const { return handle; }

private:
    Animation(
        AnimationHandle handle,
        AnimationType animation_type,
        std::function<void(AnimationStepResult const&)> const& callback);

    AnimationHandle handle;
    AnimationType type;
    mir::geometry::Rectangle from;
    mir::geometry::Rectangle to;
    float endtime_seconds = 1.f;
    float runtime_seconds = 0.f;
    const float timestep_seconds = 0.016;
    std::function<void(AnimationStepResult const&)> callback;
};

class Animator
{
public:
    /// This will take the MainLoop to schedule animation.
    explicit Animator(std::shared_ptr<mir::ServerActionQueue> const&);
    ~Animator();

    /// Queue an animation on a window from the provided point to the other
    /// point. It is assumed that the window has already been moved to the
    /// "to" position.
    AnimationHandle animate_window_movement(
        AnimationHandle previous,
        mir::geometry::Rectangle const& from,
        mir::geometry::Rectangle const& to,
        std::function<void(AnimationStepResult const&)> const& callback);

    void cancel(AnimationHandle);
    void pause(AnimationHandle);
    void resume(AnimationHandle);

private:
    void run();
    std::shared_ptr<mir::ServerActionQueue> server_action_queue;
    std::vector<Animation> queued_animations;
    std::thread run_thread;
    std::mutex processing_lock;
    std::condition_variable cv;
    AnimationHandle next_handle = 1;
};

} // miracle

#endif // MIRACLEWM_ANIMATOR_H
