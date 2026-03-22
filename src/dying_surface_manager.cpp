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

#include "dying_surface_manager.h"
#include "abstract_output.h"
#include "compositor_state.h"
#include "config.h"
#include "forwarding_surface.h"
#include "plugin_bridge.h"
#include "shell_component_container.h"
#include "window_controller.h"
#include <mir/shell/surface_stack.h>

using namespace miracle;

DyingSurfaceManager::DyingSurfaceManager(
    std::shared_ptr<mir::shell::SurfaceStack> const& surface_stack,
    std::shared_ptr<CompositorState> const& compositor_state,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<PluginManager> const& plugin_manager,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<WindowIdMap> const& window_id_map) :
    surface_stack(surface_stack),
    compositor_state(compositor_state),
    config(config),
    animator(animator),
    plugin_manager(plugin_manager),
    window_controller(window_controller),
    window_id_map(window_id_map)
{
}

void DyingSurfaceManager::animate_dying_surface(std::shared_ptr<WindowContainer> const& container)
{
    if (!config->are_animations_enabled())
        return;

    if (!container->has_render_data())
        return;

    if (!container->can_animate())
        return;

    auto const output_area = container->get_output()->get_area();
    auto surface = container->window()->operator std::shared_ptr<mir::scene::Surface>();
    auto animating_surface = std::make_shared<ForwardingSurface>(surface);
    auto const handle = animator->register_animateable();
    auto const transform = container->get_window_transform() * container->get_animation_transform();
    auto const alpha = container->get_alpha();
    auto const id = compositor_state->render_data_manager()->add(
        { .surface = animating_surface.get(),
            .needs_outline = container->needs_outline(),
            .is_focused = false,
            .transform = transform,
            .workspace_transform = container->get_output_transform() * container->get_workspace_transform(),
            .output_area = output_area });
    surface->set_alpha(alpha);
    surface->set_transformation(transform);

    surface_stack->add_surface(animating_surface, mir::input::InputReceptionMode::normal);
    AnimationData anim_data {
        AnimateableEvent::window_close,
        container->get_visible_area(),
        geom::Rectangle {},
        1, 0
    };
    if (auto const win = container->window())
    {
        miral::WindowInfo const& win_info = window_controller->info_for(win.value());
        uint64_t win_id = 0;
        for (auto const& [id, w] : *window_id_map)
            if (w == win.value())
            {
                win_id = id;
                break;
            }
        anim_data.window_info = from_window(win_info, win_id, container.get());
        anim_data.window_name = win_info.name();
    }
    animator->append(Animation(
        handle,
        config->get_animation_definition(AnimateableEvent::window_close),
        std::move(anim_data),
        [compositor_state = compositor_state, surface_stack = surface_stack, animating_surface, id = id, alpha = alpha, transform = transform](AnimationFrameResult const& result)
    {
        if (result.transform)
        {
            auto const combined = transform * result.transform.value();
            compositor_state->render_data_manager()->transform_change(id, combined);
            animating_surface->set_transformation(combined);
        }

        if (result.opacity)
        {
            auto const combined = alpha * result.opacity.value();
            animating_surface->set_alpha(combined);
        }

        if (result.rectangle)
        {
            animating_surface->move_to(result.rectangle->top_left);
            animating_surface->set_clip_area(result.rectangle.value());
        }

        if (result.is_complete)
        {
            compositor_state->render_data_manager()->remove(id);
            surface_stack->remove_surface(animating_surface);
        }
    }, plugin_manager));
}
