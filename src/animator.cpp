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

namespace
{
float now() {
    using namespace std::chrono;
    return static_cast<float>(
        duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()) / 1000.f;
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

Animator::~Animator() {
};

void Animator::animate_window_movement(
    miral::Window const& window,
    mir::geometry::Point const& from,
    mir::geometry::Point const& to)
{
    auto surface = window.operator std::weak_ptr<mir::scene::Surface>();
    data.push_back({
        window,
        surface,
        from,
        to,
        AnimationType::move_lerp
    });
}

void Animator::update()
{
    // https://gist.github.com/mariobadr/673bbd5545242fcf9482
    using clock = std::chrono::high_resolution_clock;
    constexpr std::chrono::nanoseconds timestep(16ms);
    constexpr float timestep_seconds = 0.016;
    std::chrono::nanoseconds lag(0ns);
    auto time_start = clock::now();
    running = true;

    while (running)
    {
        auto delta_time = clock::now() - time_start;
        time_start = clock::now();
        lag += std::chrono::duration_cast<std::chrono::nanoseconds>(delta_time);

        while(lag >= timestep) {
            lag -= timestep;

            for (auto it = data.begin(); it != data.end(); it++)
            {
                auto& item = *it;
                if (item.surface.expired())
                {
                    it = data.erase(it);
                    continue;
                }

                auto surface = item.surface.lock();
                item.runtime_seconds += timestep_seconds;
                switch (item.type)
                {
                    case AnimationType::move_lerp:
                    {
                        auto vector = item.to - item.from;
                        float fraction = (item.runtime_seconds / item.endtime_seconds);
                        float x = vector.dx.as_int() * fraction;
                        float y = vector.dy.as_int() * fraction;
                        surface->set_transformation(
                            glm::mat4(
                            1, 0, 0, 0,
                            0, 1, 0, 0,
                            0, 0, 1, 0,
                            x, y, 0, 1
                            )
                        );
                        break;
                    }
                }

                // TODO: Queue only one action for all changed surfaces
                server_action_queue->enqueue(this, [&]() {
                    auto window = item.window;
                    miral::WindowSpecification spec;
                    spec.top_left() = window.top_left();
                    tools.modify_window(window, spec);
                });
            }
        }
    }
}