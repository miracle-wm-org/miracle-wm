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

#include "animator.h"
#include <mir/scene/surface.h>
#include <mir/server_action_queue.h>
#include <chrono>
#define MIR_LOG_COMPONENT "animator"
#include <mir/log.h>

using namespace miracle;
using namespace std::chrono_literals;

AnimationHandle const miracle::none_animation_handle = 0;

Animation::Animation(
    AnimationHandle handle,
    miracle::AnimationType animation_type,
    std::function<void(AnimationStepResult const&)> const& callback)
    : handle{handle},
      type{animation_type},
      callback{callback}
{
}

Animation& Animation::operator=(miracle::Animation const& other)
{
    handle = other.handle;
    type = other.type;
    from = other.from;
    to = other.to;
    endtime_seconds = other.endtime_seconds;
    runtime_seconds = other.runtime_seconds;
    callback = other.callback;
    return *this;
}

Animation Animation::move_lerp(
    AnimationHandle handle,
    mir::geometry::Rectangle const& _from,
    mir::geometry::Rectangle const& _to,
    std::function<void(AnimationStepResult const&)> const& callback)
{
    Animation result(handle, AnimationType::move_lerp, callback);
    result.from = _from;
    result.to = _to;
    result.endtime_seconds = 0.25f;
    return result;
}

AnimationStepResult Animation::step()
{
    runtime_seconds += timestep_seconds;
    if (runtime_seconds >= endtime_seconds)
    {
        return {
            handle,
            true,
            glm::vec2(to.top_left.x.as_int(), to.top_left.y.as_int()),
            glm::vec2(to.size.width.as_int(), to.size.height.as_int()),
            glm::mat4(1.f),
        };
    }

    switch (type)
    {
    case AnimationType::move_lerp:
    {
        // First, we begin lerping the position
        auto distance = to.top_left - from.top_left;
        float fraction = (runtime_seconds / endtime_seconds);
        float x = (float)distance.dx.as_int() * fraction;
        float y = (float)distance.dy.as_int() * fraction;

        glm::vec2 position = {
            from.top_left.x.as_int() + x,
            from.top_left.y.as_int() + y
        };

        return {
            handle,
            false,
            position,
            glm::vec2(to.size.width.as_int(), to.size.height.as_int()),
            glm::mat4(1.f)
        };
    }
    default:
        return {
            handle,
            false,
            glm::vec2(to.top_left.x.as_int(), to.top_left.y.as_int()),
            glm::vec2(to.size.width.as_int(), to.size.height.as_int()),
            glm::mat4(1.f)
        };
    }
}

Animator::Animator(
    std::shared_ptr<mir::ServerActionQueue> const& server_action_queue)
    : server_action_queue{server_action_queue},
      run_thread([&]() { run(); })
{
}

Animator::~Animator()
{
};

AnimationHandle Animator::animate_window_movement(
    AnimationHandle previous,
    mir::geometry::Rectangle const& from,
    mir::geometry::Rectangle const& to,
    std::function<void(AnimationStepResult const&)> const& callback)
{
    std::lock_guard<std::mutex> lock(processing_lock);
    for (auto it = queued_animations.begin(); it != queued_animations.end(); it++)
    {
        if (it->get_handle() == previous)
        {
            queued_animations.erase(it);
            break;
        }
    }

    auto handle = next_handle++;
    queued_animations.push_back(Animation::move_lerp(
        handle,
        from,
        to,
        callback));
    cv.notify_one();
    return handle;
}

namespace
{
struct PendingUpdateData
{
    AnimationStepResult result;
    std::function<void(miracle::AnimationStepResult const&)> callback;
};
}

void Animator::run()
{
    // https://gist.github.com/mariobadr/673bbd5545242fcf9482
    using clock = std::chrono::high_resolution_clock;
    constexpr std::chrono::nanoseconds timestep(16ms);
    std::chrono::nanoseconds lag(0ns);
    auto time_start = clock::now();
    bool running = true;

    while (running)
    {
        {
            std::unique_lock lock(processing_lock);
            if (queued_animations.empty())
            {
                cv.wait(lock);
                time_start = clock::now();
            }
        }

        auto delta_time = clock::now() - time_start;
        time_start = clock::now();
        lag += std::chrono::duration_cast<std::chrono::nanoseconds>(delta_time);

        while(lag >= timestep) {
            lag -= timestep;

            std::vector<PendingUpdateData> update_data;
            {
                std::lock_guard<std::mutex> lock(processing_lock);
                for (auto it = queued_animations.begin(); it != queued_animations.end();)
                {
                    auto& item = *it;
                    auto result = item.step();

                    update_data.push_back({ result, item.get_callback() });
                    if (result.should_erase)
                        it = queued_animations.erase(it);
                    else
                        it++;
                }
            }

            server_action_queue->enqueue(this, [&, update_data]() {
                for (auto const& update_item : update_data)
                    update_item.callback(update_item.result);
            });
        }
    }
}