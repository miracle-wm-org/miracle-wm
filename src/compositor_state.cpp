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

#define MIR_LOG_COMPONENT "compositor_state"

#include "compositor_state.h"
#include "scene_override.h"
#include <mir/log.h>

using namespace miracle;

CompositorState::CompositorState() :
    render_data_manager_(std::make_shared<RenderDataManager>()),
    scene_override_manager_(std::make_shared<SceneOverrideManager>())
{
}

std::shared_ptr<Container> CompositorState::focused_container() const
{
    std::lock_guard lock(mutex);
    if (!focused.expired())
        return focused.lock();

    return nullptr;
}

void CompositorState::focus_container(std::shared_ptr<Container> const& container)
{
    if (!container)
        return;

    std::lock_guard lock(mutex);
    focused = container;

    // If the focused container is a window, bring it to the front of the focus order.
    auto const it = std::ranges::find_if(focus_order, [&](auto const& element)
    {
        return !element.expired() && element.lock() == container;
    });

    if (it != focus_order.end())
    {
        std::rotate(focus_order.begin(), it, it + 1);
    }
}

void CompositorState::unfocus_container(std::shared_ptr<Container> const& container)
{
    std::lock_guard lock(mutex);
    if (!focused.expired())
    {
        if (focused.lock() == container)
            focused.reset();
    }
}

void CompositorState::add(std::shared_ptr<WindowContainer> const& container)
{
    std::lock_guard lock(mutex);
    focus_order.push_back(container);
}

void CompositorState::remove(std::shared_ptr<WindowContainer> const& container)
{
    std::lock_guard lock(mutex);
    std::erase_if(focus_order, [&](auto const& element)
    {
        return !element.expired() && element.lock() == container;
    });
}

std::shared_ptr<WindowContainer> CompositorState::first_floating()
{
    std::lock_guard lock(mutex);
    for (auto const& container : focus_order)
    {
        if (!container.expired() && !container.lock()->anchored())
            return container.lock();
    }

    return nullptr;
}

std::shared_ptr<WindowContainer> CompositorState::first_tiling()
{
    std::lock_guard lock(mutex);
    for (auto const& container : focus_order)
    {
        if (!container.expired() && container.lock()->anchored())
            return container.lock();
    }

    return nullptr;
}

WindowManagerMode CompositorState::mode()
{
    std::lock_guard lock(mutex);
    return mode_;
}

void CompositorState::mode(WindowManagerMode next)
{
    std::lock_guard lock(mutex);
    mode_ = next;
}

std::shared_ptr<RenderDataManager> const& CompositorState::render_data_manager() const
{
    return render_data_manager_;
}

std::shared_ptr<SceneOverrideManager> const& CompositorState::scene_override_manager() const
{
    return scene_override_manager_;
}

uint64_t CompositorState::next_container_id()
{
    return ++next_container_id_;
}

uint64_t CompositorState::peak_next_container_id() const
{
    return next_container_id_ + 1;
}