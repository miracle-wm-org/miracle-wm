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
#include "miracle_config.h"
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
    AnimationDefinition const& definition,
    std::function<void(AnimationStepResult const&)> const& callback)
    : handle{handle},
      definition{definition},
      callback{callback}
{
}

Animation& Animation::operator=(miracle::Animation const& other)
{
    handle = other.handle;
    definition = other.definition;
    from = other.from;
    to = other.to;
    callback = other.callback;
    return *this;
}

Animation Animation::window_move(
    AnimationHandle handle,
    AnimationDefinition const& definition,
    mir::geometry::Rectangle const& _from,
    mir::geometry::Rectangle const& _to,
    std::function<void(AnimationStepResult const&)> const& callback)
{
    Animation result(handle, definition, callback);
    result.from = _from;
    result.to = _to;
    return result;
}

namespace
{
inline float ease(AnimationDefinition const& defintion, float t, float start_value, float end_value)
{
    // https://easings.net/
    const float diff = end_value - start_value;
    switch (defintion.function)
    {
    case EaseFunction::linear:
        return start_value + (t * diff);
    case EaseFunction::ease_out_back:
    {
        const float p = 1 + defintion.c3 * powf(t - 1, 3) + defintion.c1 * powf(t - 1, 2);
        return start_value + (p * diff);
    }
    default:
        return end_value;
    }
}
}

AnimationStepResult Animation::step()
{
    runtime_seconds += timestep_seconds;
    if (runtime_seconds >= definition.duration_seconds)
    {
        return {
            handle,
            true,
            glm::vec2(to.top_left.x.as_int(), to.top_left.y.as_int()),
            glm::vec2(to.size.width.as_int(), to.size.height.as_int()),
            glm::mat4(1.f),
        };
    }

    float t = (runtime_seconds / definition.duration_seconds);
    switch (definition.type)
    {
    case AnimationType::slide:
    {
        auto p = ease(definition, t, 0.f, 1.f);
        auto distance = to.top_left - from.top_left;
        float x = (float)distance.dx.as_int() * p;
        float y = (float)distance.dy.as_int() * p;

        glm::vec2 position = {
            (float)from.top_left.x.as_int() + x,
            (float)from.top_left.y.as_int() + y
        };

        return {
            handle,
            false,
            position,
            glm::vec2(to.size.width.as_int(), to.size.height.as_int()),
            glm::mat4(1.f)
        };
    }
    case AnimationType::grow:
    {
        auto p = ease(definition, t, 0.f, 1.f);
        glm::mat4 transform(
            p, 0, 0, 0,
            0, p, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1);
        return { handle, false, {}, {}, transform };
    }
    case AnimationType::shrink:
    {
        auto p = 1.f - ease(definition, t, 0.f, 1.f);
        glm::mat4 transform(
            p, 0, 0, 0,
            0, p, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1);
        return { handle, false, {}, {}, transform };
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
    std::shared_ptr<mir::ServerActionQueue> const& server_action_queue,
    std::shared_ptr<MiracleConfig> const& config)
    : server_action_queue{server_action_queue},
      config{config},
      run_thread([&]() { run(); })
{
}

Animator::~Animator()
{
    stop();
};

AnimationHandle Animator::window_move(
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

    // If animations aren't enabled, let's give them the position that
    // they want to go to immediately and don't bother animating anything.
    auto handle = next_handle++;
    if (!config->are_animations_enabled())
    {
        callback(
            { handle,
            true,
            glm::vec2(to.top_left.x.as_int(), to.top_left.y.as_int()),
            glm::vec2(to.size.width.as_int(), to.size.height.as_int()),
            glm::mat4(1.f) });
        return handle;
    }

    queued_animations.push_back(Animation::window_move(
        handle,
        config->get_animation_definitions()[(int)AnimateableEvent::window_move],
        from,
        to,
        callback));
    cv.notify_one();
    return handle;
}

AnimationHandle Animator::window_open(
    AnimationHandle previous,
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

    // If animations aren't enabled, let's give them the position that
    // they want to go to immediately and don't bother animating anything.
    auto handle = next_handle++;
    if (!config->are_animations_enabled())
    {
        callback({ handle, true});
        return handle;
    }

    queued_animations.push_back({
        handle,
        config->get_animation_definitions()[(int)AnimateableEvent::window_open],
        callback});
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
    running = true;

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
                    if (result.is_complete)
                        it = queued_animations.erase(it);
                    else
                        it++;
                }
            }

            server_action_queue->enqueue(this, [&, update_data]() {
                if (!running)
                    return;

                for (auto const& update_item : update_data)
                    update_item.callback(update_item.result);
            });
        }
    }
}

void Animator::stop()
{
    if (!running)
        return;

    running = false;
    run_thread.join();
}