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
#include <glm/gtx/string_cast.hpp>
#include <mir/geometry/rectangle.h>

using namespace miracle;
using namespace std::chrono_literals;

QueuedAnimation::QueuedAnimation(
    miral::Window const& window,
    miracle::AnimationType animation_type)
    : window{window},
      type{animation_type}
{
}

QueuedAnimation& QueuedAnimation::operator=(miracle::QueuedAnimation const& other)
{
    window = other.window;
    type = other.type;
    from = other.from;
    to = other.to;
    endtime_seconds = other.endtime_seconds;
    runtime_seconds = other.runtime_seconds;
    return *this;
}

QueuedAnimation QueuedAnimation::move_lerp(
    miral::Window const& window,
    mir::geometry::Rectangle const& _from,
    mir::geometry::Rectangle const& _to)
{
    QueuedAnimation result(window, AnimationType::move_lerp);
    result.from = _from;
    result.to = _to;
    result.endtime_seconds = 1.f;
    return result;
}

std::weak_ptr<mir::scene::Surface> QueuedAnimation::get_surface() const
{
    return window.operator std::weak_ptr<mir::scene::Surface>();
}

StepResult QueuedAnimation::step()
{
    auto weak_surface = get_surface();
    if (weak_surface.expired())
    {
        return {
            true,
            glm::vec2(to.top_left.x.as_int(), to.top_left.y.as_int()),
            glm::vec2(to.size.width.as_int(), to.size.height.as_int()),
            glm::mat4(1.f),
        };
    }

    runtime_seconds += timestep_seconds;
    if (runtime_seconds >= endtime_seconds)
    {
        return {
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

        // Next, we lerp the size. The size is held in the transform, because
        // resizing over-and-over again is not performant to say the least.
        // We want to scale around the bottom right corner of the surface.
        // https://math.stackexchange.com/questions/3245481/rotate-and-scale-a-point-around-different-origins
        auto width_diff = to.size.width.as_int() - from.size.width.as_int();
        auto height_diff = to.size.height.as_int() - from.size.height.as_int();
        float w = (float)width_diff * fraction;
        float h = (float)height_diff * fraction;
        float w_scale = (w + (float)from.size.width.as_int()) / (float)from.size.width.as_int();
        float h_scale = (h + (float)from.size.height.as_int()) / (float)from.size.height.as_int();

       auto translate_matrix = glm::translate(
            glm::mat4(1.f),
            glm::vec3(-(from.size.width.as_int() / 2.f), -(from.size.height.as_int() / 2.f), 0));
        auto scale_matrix = glm::mat4{
            w_scale, 0, 0, 0,
            0, h_scale, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        };
        auto reverse_translate_matrix = glm::translate(
            glm::mat4(1.f),
            glm::vec3(from.size.width.as_int() / 2.f, from.size.height.as_int() / 2.f, 0));

        return {
            false,
            position,
            glm::vec2(from.size.width.as_int(), from.size.height.as_int()),
            translate_matrix * scale_matrix * reverse_translate_matrix
        };
    }
    default:
        return {
            false,
            glm::vec2(to.top_left.x.as_int(), to.top_left.y.as_int()),
            glm::vec2(to.size.width.as_int(), to.size.height.as_int()),
            glm::mat4(1.f)
        };
    }
}

Animator::Animator(
    miral::WindowManagerTools const& tools,
    std::shared_ptr<mir::ServerActionQueue> const& server_action_queue)
    : tools{tools},
      server_action_queue{server_action_queue},
      run_thread([&]() { run(); })
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
    cv.notify_one();
}

namespace
{
struct PendingUpdateData
{
    miral::Window window;
    StepResult result;
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
            if (processing.empty())
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
                for (auto it = processing.begin(); it != processing.end();)
                {
                    auto& item = *it;
                    auto result = item.step();

                    update_data.push_back({ item.get_window(), result });
                    if (result.should_erase)
                        it = processing.erase(it);
                    else
                        it++;
                }
            }

            server_action_queue->enqueue(this, [&, update_data]() {
                for (auto& update_item : update_data)
                {
                    if (auto surface = update_item.window.operator std::shared_ptr<mir::scene::Surface>())
                    {
                        surface->set_transformation(update_item.result.transform);
                        // Set the positions on the windows and the sub windows
                        miral::WindowSpecification spec;
                        spec.top_left() = mir::geometry::Point(
                            update_item.result.position.x,
                            update_item.result.position.y
                        );
                        spec.size() = mir::geometry::Size(
                            update_item.result.size.x,
                            update_item.result.size.y
                        );
                        tools.modify_window(update_item.window, spec);

                        auto& window_info = tools.info_for(update_item.window);
                        mir::geometry::Rectangle r(
                            mir::geometry::Point(
                                update_item.result.position.x,
                                update_item.result.position.y
                                ),
                            mir::geometry::Size(
                                update_item.result.size.x,
                                update_item.result.size.y
                                ));
                        window_info.clip_area(r);

                        for (auto const& child : window_info.children())
                        {
                            miral::WindowSpecification sub_spec;
                            sub_spec.top_left() = spec.top_left();
                            sub_spec.size() = spec.size();
                            tools.modify_window(child, sub_spec);
                        }
                    }
                    else
                    {
                        mir::log_warning("Update data item was deleted before the transformation could be set");
                    }
                }
            });
        }
    }
}