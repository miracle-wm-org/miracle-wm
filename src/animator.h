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

#include "animation_defintion.h"
#include <condition_variable>
#include <functional>
#include <glm/glm.hpp>
#include <mir/geometry/rectangle.h>
#include <mutex>
#include <optional>
#include <thread>

namespace mir
{
class ServerActionQueue;
}

namespace miracle
{
class MiracleConfig;

/// Unique handle provided to track animators
typedef uint32_t AnimationHandle;

/// Reserved for windows who lack an animation handle
extern const AnimationHandle none_animation_handle;

/// Callback data provided to the caller on each tick.
struct AnimationStepResult
{
    AnimationHandle handle = none_animation_handle;
    bool is_complete = false;
    std::optional<glm::vec2> position;
    std::optional<glm::vec2> size;
    std::optional<glm::mat4> transform;
};

class Animation
{
public:
    Animation(
        AnimationHandle handle,
        AnimationDefinition const& definition,
        std::function<void(AnimationStepResult const&)> const& callback);

    static Animation window_move(
        AnimationHandle handle,
        AnimationDefinition const& definition,
        mir::geometry::Rectangle const& from,
        mir::geometry::Rectangle const& to,
        std::function<void(AnimationStepResult const&)> const& callback);
    Animation& operator=(Animation const& other);

    AnimationStepResult init();
    AnimationStepResult step();
    [[nodiscard]] std::function<void(AnimationStepResult const&)> const& get_callback() const { return callback; }
    [[nodiscard]] AnimationHandle get_handle() const { return handle; }

private:
    AnimationHandle handle;
    AnimationDefinition definition;
    mir::geometry::Rectangle from;
    mir::geometry::Rectangle to;
    const float timestep_seconds = 0.016;
    std::function<void(AnimationStepResult const&)> callback;
    float runtime_seconds = 0.f;
};

/// Manages the animation queue. If multiple animations are queued for a window,
/// then the latest animation may override values from previous animations.
class Animator
{
public:
    explicit Animator(
        std::shared_ptr<mir::ServerActionQueue> const&,
        std::shared_ptr<MiracleConfig> const&);
    ~Animator();

    /// Animateable components must register with the Animator before being
    /// able to be animated.
    AnimationHandle register_animateable();

    void window_move(
        AnimationHandle handle,
        mir::geometry::Rectangle const& from,
        mir::geometry::Rectangle const& to,
        std::function<void(AnimationStepResult const&)> const& callback);

    void window_open(
        AnimationHandle handle,
        std::function<void(AnimationStepResult const&)> const& callback);

    void cancel(AnimationHandle);
    void pause(AnimationHandle);
    void resume(AnimationHandle);
    void stop();

private:
    void run();
    bool running = false;
    std::shared_ptr<mir::ServerActionQueue> server_action_queue;
    std::shared_ptr<MiracleConfig> config;
    std::vector<Animation> queued_animations;
    std::thread run_thread;
    std::mutex processing_lock;
    std::condition_variable cv;
    AnimationHandle next_handle = 1;
};

} // miracle

#endif // MIRACLEWM_ANIMATOR_H
