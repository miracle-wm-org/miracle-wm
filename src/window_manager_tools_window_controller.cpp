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

#define MIR_LOG_COMPONENT "window_manager_tools_tiling_interface"

#include "window_manager_tools_window_controller.h"
#include "animator.h"
#include "compositor_state.h"
#include "config.h"
#include "leaf_container.h"
#include "plugin_bridge.h"
#include "policy.h"
#include "window_container.h"
#include "window_helpers.h"
#include <mir/log.h>
#include <mir/server_action_queue.h>

using namespace miracle;

namespace
{
auto create_window_animation_callback(std::weak_ptr<Container> const& container, WindowManagerToolsWindowController* controller, std::shared_ptr<mir::ServerActionQueue> const& action_queue)
{
    return [container_weak = container, controller, action_queue](
               AnimationFrameResult const& result)
    {
        action_queue->enqueue(controller, [container_weak = container_weak, result = result, controller]
        {
            if (auto const container = container_weak.lock())
                controller->process_animation(result, container);
        });
    };
}
}

WindowManagerToolsWindowController::WindowManagerToolsWindowController(
    miral::WindowManagerTools const& tools,
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<PluginManager> const& plugin_manager,
    std::shared_ptr<mir::ServerActionQueue> const& server_action_queue,
    std::shared_ptr<CompositorState> const& state,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<WindowIdMap> const& window_id_map) :
    tools { tools },
    animator { animator },
    plugin_manager { plugin_manager },
    server_action_queue { server_action_queue },
    state { state },
    config { config },
    window_id_map { window_id_map }
{
}

void WindowManagerToolsWindowController::open(miral::Window const& window)
{
    auto container = get_window_container(window);
    if (!container)
    {
        mir::log_error("Cannot set rectangle of window that lacks container");
        return;
    }

    if (!container->can_animate())
        return;

    geom::Rectangle rect { window.top_left(), window.size() };
    if (!config->are_animations_enabled())
    {
        process_animation(AnimationFrameResult { true, rect, std::nullopt, std::nullopt }, container);
        return;
    }

    miral::WindowInfo const& win_info = tools.info_for(window);
    uint64_t win_id = 0;
    for (auto const& [id, w] : *window_id_map)
        if (w == window)
        {
            win_id = id;
            break;
        }
    AnimationData anim_data { AnimateableEvent::window_open, rect, rect, 0, 1 };
    anim_data.window_info = from_window(win_info, win_id, container.get());
    anim_data.window_name = win_info.name();
    animator->append(Animation(
        container->animation_handle(),
        config->get_animation_definition(AnimateableEvent::window_open),
        std::move(anim_data),
        create_window_animation_callback(container, this, server_action_queue), plugin_manager));
}

void WindowManagerToolsWindowController::set_rectangle(
    miral::Window const& window, geom::Rectangle const& from, geom::Rectangle const& to, bool with_animations)
{
    auto container = get_window_container(window);
    if (!container)
    {
        mir::log_error("Cannot set rectangle of window that lacks container");
        return;
    }

    if (!container->can_animate())
        return;

    if (!config->are_animations_enabled() || !with_animations)
    {
        process_animation(
            AnimationFrameResult {
                true,
                to,
                glm::mat4(1.f),
                std::nullopt },
            container);
        return;
    }

    miral::WindowInfo const& win_info = tools.info_for(window);
    uint64_t win_id = 0;
    for (auto const& [id, w] : *window_id_map)
        if (w == window)
        {
            win_id = id;
            break;
        }
    AnimationData anim_data { AnimateableEvent::window_move, from, to, 1, 1 };
    anim_data.window_info = from_window(win_info, win_id, container.get());
    anim_data.window_name = win_info.name();
    animator->append(Animation(
        container->animation_handle(),
        config->get_animation_definition(AnimateableEvent::window_move),
        std::move(anim_data),
        create_window_animation_callback(container, this, server_action_queue), plugin_manager));
}

MirWindowState WindowManagerToolsWindowController::get_state(miral::Window const& window)
{
    auto& window_info = tools.info_for(window);
    return window_info.state();
}

void WindowManagerToolsWindowController::change_state(miral::Window const& window, MirWindowState state)
{
    auto const& window_info = tools.info_for(window);
    miral::WindowSpecification spec;
    spec.state() = state;
    tools.place_and_size_for_state(spec, window_info);
    tools.modify_window(window, spec);
}

void WindowManagerToolsWindowController::clip(miral::Window const& window, geom::Rectangle const& r)
{
    auto& window_info = tools.info_for(window);
    window_info.clip_area(r);
}

void WindowManagerToolsWindowController::noclip(miral::Window const& window)
{
    auto& window_info = tools.info_for(window);
    window_info.clip_area(mir::optional_value<geom::Rectangle>());
}

void WindowManagerToolsWindowController::select_active_window(miral::Window const& window)
{
    if (state->mode() != WindowManagerMode::normal)
        return;

    tools.select_active_window(window);
}

std::shared_ptr<WindowContainer> WindowManagerToolsWindowController::get_window_container(miral::Window const& window)
{
    if (window == miral::Window {})
        return nullptr;

    auto& info = tools.info_for(window);
    if (info.userdata())
        return static_pointer_cast<WindowContainer>(info.userdata());

    if (info.parent())
        return get_window_container(info.parent());

    return nullptr;
}

void WindowManagerToolsWindowController::raise(miral::Window const& window)
{
    tools.raise_tree(window);
}

void WindowManagerToolsWindowController::send_to_back(miral::Window const& window)
{
    tools.send_tree_to_back(window);
}

void WindowManagerToolsWindowController::process_animation(
    AnimationFrameResult const& result,
    std::shared_ptr<Container> const& container)
{
    // Note: We MUST invoke this under the lock, because the window could theoretically get
    // removed while it is ani
    invoke_under_lock([&]
    {
        // TODO (hack): we really need to refactor the entire animation system. The issue
        //  here is that our current system relies on us enqueuing animation processing
        //  onto the main loop for animations. However, this may cause a situation where
        //  a window is deleted, but we still have an animation pending to it. When we
        //  try to modify that windows, things blow up on us. The solution here is temporary
        //  but the real solution is to refactor how we're doing animations so that it is
        //  less generic and makes more sense. The "hack" is the try/catch block
        try
        {
            bool needs_modify = false;
            miral::WindowSpecification spec;
            auto const& rectangle = result.rectangle;

            if (rectangle)
            {
                spec.top_left() = rectangle->top_left;
                // TODO: Remove once a real fix is added upstream
                // Fixes: https://github.com/miracle-wm-org/miracle-wm/issues/489
                // Workaround for: https://github.com/canonical/mir/issues/4222
                auto const& info = tools.info_for(container->window().value());
                spec.size() = geom::Size {
                    geom::Width(std::max(rectangle->size.width.as_int(), info.min_width().as_value())),
                    geom::Height(std::max(rectangle->size.height.as_int(), info.min_height().as_value()))
                };

                needs_modify = true;
            }

            if (!container->window())
                return;

            auto const window = container->window().value();
            if (!window)
                return;

            // TODO: Modify window can throw an exception, which we are catching
            if (needs_modify)
                tools.modify_window(window, spec);

            if (result.transform)
                container->set_animation_transform(result.transform.value());

            if (result.opacity != std::nullopt)
                container->set_animation_alpha(result.opacity.value());

            if (result.is_complete)
                container->constrain();
            else if (rectangle)
                clip(window, rectangle.value());
        }
        catch (std::out_of_range const&)
        {
        }
    });
}

void WindowManagerToolsWindowController::set_user_data(
    miral::Window const& window, std::shared_ptr<void> const& data)
{
    miral::WindowSpecification spec;
    spec.userdata() = data;
    tools.modify_window(window, spec);
}

void WindowManagerToolsWindowController::modify(
    miral::Window const& window, miral::WindowSpecification const& spec)
{
    tools.modify_window(window, spec);
}

miral::WindowInfo& WindowManagerToolsWindowController::info_for(miral::Window const& window)
{
    return tools.info_for(window);
}

miral::ApplicationInfo& WindowManagerToolsWindowController::info_for(miral::Application const& app)
{
    return tools.info_for(app);
}

miral::ApplicationInfo& WindowManagerToolsWindowController::app_info(miral::Window const& window)
{
    return tools.info_for(window.application());
}

void WindowManagerToolsWindowController::close(miral::Window const& window)
{
    tools.ask_client_to_close(window);
}

void WindowManagerToolsWindowController::move_cursor_to(float x, float y)
{
    tools.move_cursor_to(geom::PointF { x, y });
}

miral::Window WindowManagerToolsWindowController::window_at(float x, float y)
{
    return tools.window_at({ x, y });
}

void WindowManagerToolsWindowController::invoke_under_lock(std::function<void()> const& f)
{
    tools.invoke_under_lock(f);
}
