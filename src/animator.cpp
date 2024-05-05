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

QueuedAnimation::QueuedAnimation(
    miral::Window const& window,
    miracle::AnimationType animation_type)
    : window{window},
      type{animation_type}
{
}

QueuedAnimation QueuedAnimation::operator=(miracle::QueuedAnimation other)
{
    QueuedAnimation result(other.window, other.type);
    result.from = other.from;
    result.to = other.to;
    result.endtime_seconds = other.endtime_seconds;
    result.runtime_seconds = other.runtime_seconds;
    return result;
}

QueuedAnimation QueuedAnimation::move_lerp(
    miral::Window const& window,
    mir::geometry::Rectangle const& _from,
    mir::geometry::Rectangle const& _to)
{
    QueuedAnimation result(window, AnimationType::move_lerp);
    result.from = _from;
    result.to = _to;
    result.endtime_seconds = 0.3f;
    return result;
}

std::weak_ptr<mir::scene::Surface> QueuedAnimation::get_surface() const
{
    return window.operator std::weak_ptr<mir::scene::Surface>();
}

glm::mat4 QueuedAnimation::step(bool& should_erase)
{
    auto weak_surface = window.operator std::weak_ptr<mir::scene::Surface>();
    if (weak_surface.expired())
    {
        should_erase = true;
        return glm::mat4{1.f};
    }

    runtime_seconds += timestep_seconds;
    if (runtime_seconds >= endtime_seconds)
    {
        should_erase = true;
        return glm::mat4{1.f};
    }

    should_erase = false;
    switch (type)
    {
    case AnimationType::move_lerp:
    {
        auto distance = to.top_left - from.top_left;
        float fraction = 1.f - (runtime_seconds / endtime_seconds);
        float x = (float)distance.dx.as_int() * fraction;
        float y = (float)distance.dy.as_int() * fraction;

        return glm::mat4 {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            -x, -y, 0, 1
        };
    }
    }
}

Animator::Animator(
    miral::WindowManagerTools const& tools,
    std::shared_ptr<mir::ServerActionQueue> const& server_action_queue)
    : tools{tools},
      server_action_queue{server_action_queue},
      run_thread([&]() { update(); })
{
}

Animator::~Animator()
{
};

void Animator::animate_window_movement(
    miral::Window const& window,
    mir::geometry::Rectangle const& from,
    mir::geometry::Rectangle const& to)
{
    std::lock_guard<std::mutex> lock(processing_lock);
    for (auto it = processing.begin(); it != processing.end(); it++)
    {
        if (it->get_window() == window)
        {
            processing.erase(it);
            break;
        }
    }

    processing.push_back(QueuedAnimation::move_lerp(
        window,
        from,
        to));
}

namespace
{
struct PendingUpdateData
{
    std::weak_ptr<mir::scene::Surface> surface;
    glm::mat4 next_transform;
};
}

void Animator::update()
{
    // https://gist.github.com/mariobadr/673bbd5545242fcf9482
    using clock = std::chrono::high_resolution_clock;
    constexpr std::chrono::nanoseconds timestep(16ms);
    std::chrono::nanoseconds lag(0ns);
    auto time_start = clock::now();
    bool running = true;

    std::vector<PendingUpdateData> update_data;
    std::mutex update_data_lock;
    while (running)
    {
        // Copy over pending into processing. This is also where we reconcile
        // in-progress against new animations.
        auto delta_time = clock::now() - time_start;
        time_start = clock::now();
        lag += std::chrono::duration_cast<std::chrono::nanoseconds>(delta_time);

        while(lag >= timestep) {
            lag -= timestep;

            std::lock_guard<std::mutex> lock(processing_lock);
            for (auto it = processing.begin(); it != processing.end();)
            {
                auto& item = *it;
                bool should_remove = false;
                auto transform = item.step(should_remove);

                {
                    std::lock_guard update(update_data_lock);
                    update_data.push_back({item.get_surface(), transform});
                }

                if (should_remove)
                    it = processing.erase(it);
                else
                    it++;
            }

            server_action_queue->enqueue(this, [&]() {
                std::lock_guard update(update_data_lock);
                for (auto& update_item : update_data)
                {
                    if (auto surface = update_item.surface.lock())
                    {
                        surface->set_transformation(update_item.next_transform);
                    }
                    else
                    {
                        mir::log_warning("Update data item was deleted before the transformation could be set");
                    }
                }

                update_data.clear();
            });
        }
    }
}