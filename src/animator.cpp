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

using namespace miracle;
using namespace std::chrono_literals;

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
    mir::geometry::Point const& from,
    mir::geometry::Point const& to)
{
    std::lock_guard<std::mutex> lock(processing_lock);
    for (auto it = processing.begin(); it != processing.end(); it++)
    {
        if (it->window == window)
        {
            processing.erase(it);
            break;
        }
    }

    processing.push_back({
        window,
        from,
        to,
        AnimationType::move_lerp,
        0.2f
    });
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
    constexpr float timestep_seconds = 0.016;
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
                glm::mat4 transformation;
                auto& item = *it;
                auto window = item.window;
                auto weak_surface = item.window.operator std::weak_ptr<mir::scene::Surface>();
                if (weak_surface.expired())
                {
                    it = processing.erase(it);
                    goto queue_update;
                    continue;
                }

                item.runtime_seconds += timestep_seconds;
                if (item.runtime_seconds >= item.endtime_seconds)
                {
                    it = processing.erase(it);
                    goto queue_update;
                    continue;
                }

                switch (item.type)
                {
                    case AnimationType::move_lerp:
                    {
                        auto distance = item.to - item.from;
                        float fraction = 1.f - (item.runtime_seconds / item.endtime_seconds);
                        float x = distance.dx.as_int() * fraction;
                        float y = distance.dy.as_int() * fraction;

                        transformation = glm::mat4(
                            1, 0, 0, 0,
                            0, 1, 0, 0,
                            0, 0, 1, 0,
                            -x, -y, 0, 1
                        );
                        break;
                    }
                }

                it++;

            queue_update:
                std::lock_guard update(update_data_lock);
                update_data.push_back({weak_surface, transformation});
            }

            server_action_queue->enqueue(this, [&]() {
                std::lock_guard update(update_data_lock);
                for (auto& update_item : update_data)
                {
                    if (auto surface = update_item.surface.lock())
                    {
                        surface->set_transformation(update_item.next_transform);
                    }
                }

                update_data.clear();
            });
        }
    }
}