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

#include "dying_surface_manager.h"
#include "compositor_state.h"
#include "config.h"
#include "forwarding_surface.h"
#include "window_controller.h"
#include <mir/main_loop.h>
#include <mir/shell/surface_stack.h>

using namespace miracle;

namespace
{
class RawSurfaceAnimation final : public Animation
{
public:
    RawSurfaceAnimation(
        std::shared_ptr<CompositorState> const& state,
        std::shared_ptr<mir::scene::Surface> const& surface,
        AnimationHandle const& handle,
        AnimationDefinition const& definition,
        mir::geometry::Rectangle const& from,
        mir::geometry::Rectangle const& to,
        mir::geometry::Rectangle const& current,
        glm::mat4 const& transform,
        glm::mat4 const& workspace_transform,
        std::function<void()> const& on_finish) :
        Animation(handle, definition, from, to, current),
        compositor_state(state),
        surface_(surface),
        on_finish(on_finish)
    {
        id = compositor_state->render_data_manager()->add(
            { .surface = surface.get(),
                .needs_outline = true,
                .is_focused = false,
                .transform = transform,
                .workspace_transform = workspace_transform });
    }

    ~RawSurfaceAnimation() override
    {
        compositor_state->render_data_manager()->remove(id);
    }

    void on_tick(AnimationStepResult const& result) override
    {
        if (result.transform)
        {
            compositor_state->render_data_manager()->transform_change(id, result.transform.value());
            surface_->set_transformation(result.transform.value());
        }

        if (result.opacity)
            surface_->set_alpha(result.opacity.value());

        if (result.is_complete)
            on_finish();
    }

private:
    RenderDataManagerId id;
    std::shared_ptr<CompositorState> compositor_state;
    std::shared_ptr<mir::scene::Surface> surface_;
    std::function<void()> on_finish;
};
}

DyingSurfaceManager::DyingSurfaceManager(
    std::shared_ptr<mir::ServerActionQueue> const& main_loop,
    std::shared_ptr<mir::shell::SurfaceStack> const& surface_stack,
    std::shared_ptr<CompositorState> const& compositor_state,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<Animator> const& animator) :
    main_loop(main_loop),
    surface_stack(surface_stack),
    compositor_state(compositor_state),
    window_controller(window_controller),
    config(config),
    animator(animator)
{
}

void DyingSurfaceManager::animate_dying_surface(std::shared_ptr<Container> const& container)
{
    if (container->get_type() != ContainerType::leaf)
        return;

    if (!config->are_animations_enabled())
        return;

    auto surface = container->window()->operator std::shared_ptr<mir::scene::Surface>();
    main_loop->enqueue(this, [this, surface = surface, container = container]
    {
        window_controller->invoke_under_lock([this, surface = surface, container = container]
        {
            auto animating_surface = std::make_shared<ForwardingSurface>(surface);
            auto const raw_surface_animation = std::make_shared<RawSurfaceAnimation>(
                compositor_state,
                animating_surface,
                animator->register_animateable(),
                config->get_animation_definition(AnimateableEvent::window_close),
                container->get_visible_area(),
                geom::Rectangle {},
                container->get_visible_area(),
                container->get_transform(),
                container->get_output_transform() * container->get_workspace_transform(),
                [surface_stack = surface_stack, animating_surface = animating_surface]
            {
                surface_stack->remove_surface(animating_surface);
            });

            surface_stack->add_surface(animating_surface, mir::input::InputReceptionMode::normal);
            animator->append(raw_surface_animation);
        });
    });
}
