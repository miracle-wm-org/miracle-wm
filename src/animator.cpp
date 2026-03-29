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
        .is_complete = is_complete || other.is_complete,
        .rectangle = rectangle ? *rectangle : other.rectangle,
        .transform = transform ? *transform : other.transform,
        .opacity = opacity ? *opacity : other.opacity,
        .clip_area = clip_area ? *clip_area : other.clip_area
    };
}

AnimationHandle Animator::register_animateable()
{
    std::lock_guard<std::mutex> lock(processing_lock);
    return next_handle++;
}

void Animator::append(Animation&& animation)
{
    std::lock_guard<std::mutex> lock(processing_lock);
    pending_active.push_back(std::move(animation));
    animation_count.fetch_add(1, std::memory_order_release);
    cv.notify_one();
}

void Animator::append(CustomAnimation&& animation)
{
    std::lock_guard<std::mutex> lock(processing_lock);
    pending_active_custom.push_back(std::move(animation));
    animation_count.fetch_add(1, std::memory_order_release);
    cv.notify_one();
}

void Animator::tick(float dt)
{
    {
        // Transfer pending lists to the active lists. We must NOT hold this
        // lock while ticking animations, because animation callbacks may call
        // append() or remove_by_animation_handle() back into this class.
        std::lock_guard<std::mutex> lock(processing_lock);

        // Process pending removals first so that a remove followed immediately
        // by a re-add of the same handle works correctly.
        for (auto handle : pending_remove)
        {
            for (auto& other : active)
            {
                if (other.handle() == handle)
                    other.mark_for_removal();
            }
            for (auto& other : active_custom)
            {
                if (other.handle() == handle)
                    other.mark_for_removal();
            }
        }
        pending_remove.clear();

        for (auto const& pending : pending_active)
        {
            for (auto& other : active)
            {
                if (other.handle() == pending.handle())
                    other.mark_for_removal();
            }
        }

        for (auto const& pending : pending_active)
            active.push_back(pending);
        pending_active.clear();

        for (auto const& pending : pending_active_custom)
        {
            for (auto& other : active_custom)
            {
                if (other.handle() == pending.handle())
                    other.mark_for_removal();
            }
        }

        for (auto const& pending : pending_active_custom)
            active_custom.push_back(pending);
        pending_active_custom.clear();
    }

    for (auto& item : active)
    {
        if (item.is_being_removed())
            continue;

        if (item.tick(dt))
            item.mark_for_removal();
    }

    auto removed = std::erase_if(active, [](Animation const& animation)
    {
        return animation.is_being_removed();
    });
    animation_count.fetch_sub(static_cast<int>(removed), std::memory_order_release);

    for (auto& item : active_custom)
    {
        if (item.is_being_removed())
            continue;

        if (item.tick(dt))
            item.mark_for_removal();
    }

    removed = std::erase_if(active_custom, [](CustomAnimation const& animation)
    {
        return animation.is_being_removed();
    });
    animation_count.fetch_sub(static_cast<int>(removed), std::memory_order_release);
}

void Animator::remove_by_animation_handle(AnimationHandle handle)
{
    // Queue the removal; tick() will apply it to the active list on the next
    // frame while holding the lock. This avoids a data race with tick(), which
    // processes active/active_custom without holding processing_lock.
    std::lock_guard<std::mutex> lock(processing_lock);
    pending_remove.push_back(handle);
}

bool Animator::is_animating(AnimationHandle handle)
{
    // Only checks the pending queues (which are safe to read under the lock).
    // Animations already promoted to the active list are not detected here —
    // see the comment on the declaration for the full rationale.
    std::lock_guard<std::mutex> lock(processing_lock);
    return std::ranges::any_of(pending_active, [handle](Animation const& animation)
    {
        return animation.handle() == handle;
    }) || std::ranges::any_of(pending_active_custom, [handle](CustomAnimation const& animation)
    {
        return animation.handle() == handle;
    });
}
