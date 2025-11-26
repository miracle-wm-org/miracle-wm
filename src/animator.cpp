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

#define MIR_LOG_COMPONENT "animator"

#include "animator.h"
#include "geometry_helpers.h"
#include <algorithm>
#include <chrono>
#include <mir/server_action_queue.h>
#include <vector>

using namespace miracle;
using namespace std::chrono_literals;
namespace geom = mir::geometry;

AnimationFrameResult AnimationFrameResult::merge(AnimationFrameResult const& other) const
{
    return AnimationFrameResult {
        is_complete || other.is_complete,
        rectangle ? *rectangle : other.rectangle,
        transform ? *transform : other.transform,
        opacity ? *opacity : other.opacity
    };
}

Animator::Animator(std::shared_ptr<mir::ServerActionQueue> const& server_action_queue) :
    server_action_queue(server_action_queue)
{
}

AnimationHandle Animator::register_animateable()
{
    std::lock_guard<std::mutex> lock(processing_lock);
    return next_handle++;
}

void Animator::append(std::shared_ptr<Animation> const& animation)
{
    std::lock_guard<std::mutex> lock(processing_lock);
    for (auto const& other : active)
    {
        if (other->handle() == animation->handle())
            other->mark_for_removal();
    }
    active.push_back(animation);
    server_action_queue->enqueue(this, [animation = animation]()
    {
        animation->on_tick(animation->tick(0.f));
    });
    cv.notify_one();
}

void Animator::tick(float dt)
{
    std::lock_guard<std::mutex> lock(processing_lock);

    for (auto const& item : active)
    {
        if (item->is_being_removed())
            continue;

        auto result = item->tick(dt);

        server_action_queue->enqueue(this, [item = item, result = result]()
        {
            item->on_tick(result);
        });

        if (result.is_complete)
            item->mark_for_removal();
    }

    std::erase_if(active, [](std::shared_ptr<Animation> const& animation)
    {
        return animation->is_being_removed();
    });
}

void Animator::remove_by_animation_handle(AnimationHandle handle)
{
    std::lock_guard<std::mutex> lock(processing_lock);
    for (auto const& animation : active)
    {
        if (animation->handle() == handle)
            animation->mark_for_removal();
    }
}

bool Animator::is_animating(AnimationHandle handle)
{
    std::lock_guard<std::mutex> lock(processing_lock);
    return std::any_of(active.begin(), active.end(), [handle](std::shared_ptr<Animation> const& animation)
    {
        return animation->handle() == handle;
    });
}
