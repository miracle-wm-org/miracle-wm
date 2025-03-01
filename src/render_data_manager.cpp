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

#include "render_data_manager.h"
#include "container.h"
#include "animator.h"
#include "config.h"
#include <algorithm>
#include <mir/scene/surface.h>
#include <mir/graphics/renderable.h>

using namespace miracle;

namespace
{
class AnimatedRenderable : public mir::graphics::Renderable
{
public:
    AnimatedRenderable(std::shared_ptr<Renderable> const& renderable, AnimationHandle handle) :
        renderable { renderable },
        handle { handle }
    {
    }

    [[nodiscard]] ID id() const override
    {
        return "";
    }

    [[nodiscard]] std::shared_ptr<mir::graphics::Buffer> buffer() const override
    {
        return renderable->buffer();
    }

    [[nodiscard]] geom::Rectangle screen_position() const override
    {
        return renderable->screen_position();
    }

    [[nodiscard]] geom::RectangleD src_bounds() const override
    {
        return renderable->src_bounds();
    }

    [[nodiscard]] std::optional<geom::Rectangle> clip_area() const override
    {
        return std::nullopt;
    }

    [[nodiscard]] float alpha() const override
    {
        return alpha_;
    }

    [[nodiscard]] glm::mat4 transformation() const override
    {
        return transform;
    }

    [[nodiscard]] bool shaped() const override
    {
        return renderable->shaped();
    }

    [[nodiscard]] std::optional<mir::scene::Surface const*> surface_if_any() const override
    {
        return {};
    }

    std::shared_ptr<mir::graphics::Renderable> const renderable;
    AnimationHandle const handle;
    float alpha_ = 1.f;
    glm::mat4 transform = glm::mat4(1.f);
};

class RenderableAnimation : public Animation
{
public:
    RenderableAnimation(
        std::shared_ptr<AnimatedRenderable> renderable,
        AnimationHandle handle,
        AnimationDefinition definition,
        mir::geometry::Rectangle const& from,
        mir::geometry::Rectangle const& to,
        mir::geometry::Rectangle const& current) :
        Animation(handle, definition, from, to, current),
        renderable{ std::move(renderable) }
    {}

    void on_tick(AnimationStepResult const& result) override
    {
        if (result.transform)
            renderable->transform = result.transform.value();
    }

private:
    std::shared_ptr<AnimatedRenderable> renderable;
};

inline bool needs_outline(Container const& container)
{
    auto const surface = container.window().value().operator std::shared_ptr<mir::scene::Surface>();
    container.window().value();
    return (container.get_type() == ContainerType::leaf)
        && (surface == nullptr || !surface->parent());
}

inline glm::mat4 workspace_transform(Container const& container)
{
    return container.get_output_transform() * container.get_workspace_transform();
}
}

RenderDataManager::RenderDataManager()
{
    render_data.reserve(48);
}

void RenderDataManager::add(Container const& container)
{
    if (container.window() == std::nullopt)
        return;

    std::lock_guard lock(mutex);
    render_data.emplace_back(RenderData {
        .surface = container.window()->operator std::shared_ptr<mir::scene::Surface>().get(),
        .needs_outline = needs_outline(container),
        .is_focused = container.is_focused(),
        .transform = container.get_transform(),
        .workspace_transform = workspace_transform(container) });
}

void RenderDataManager::transform_change(Container const& container)
{
    std::lock_guard lock(mutex);
    for (auto& data : render_data)
    {
        if (data.surface == container.window()->operator std::shared_ptr<mir::scene::Surface>().get())
        {
            data.transform = container.get_transform();
            return;
        }
    }
}

void RenderDataManager::workspace_transform_change(Container const& container)
{
    std::lock_guard lock(mutex);
    for (auto& data : render_data)
    {
        if (data.surface == container.window()->operator std::shared_ptr<mir::scene::Surface>().get())
        {
            data.workspace_transform = workspace_transform(container);
            return;
        }
    }
}

void RenderDataManager::focus_change(Container const& container)
{
    std::lock_guard lock(mutex);
    for (auto& data : render_data)
    {
        if (data.surface == container.window()->operator std::shared_ptr<mir::scene::Surface>().get())
        {
            data.is_focused = container.is_focused();
            return;
        }
    }
}

void RenderDataManager::remove(Container const& container)
{
    std::lock_guard lock(mutex);

    // This implementation might seem odd, but let me explain.
    //
    // When this is called, the container (and more importantly its window)
    // have been removed from the system. They no longer exist, poof, it's over.
    // However, we still might want to render the last buffers associated with
    // this window so that we can animate their disappearance. To do this, we
    // add a new animateable for the renderables that we've stored from the
    // last render for this surface. We then call surface_changed() to make
    // sure that the re-render happens as the animation happens here.
    render_data.erase(std::remove_if(render_data.begin(), render_data.end(), [&](RenderData const& data)
    {
        return data.surface == container.window()->operator std::shared_ptr<mir::scene::Surface>().get();
    }),
        render_data.end());

    if (!config->are_animations_enabled())
        return;

    for (auto const& renderable : last_renderables)
    {
        auto window = container.window();
        if (!window)
            continue;

        if (renderable->surface_if_any() == window->operator std::shared_ptr<mir::scene::Surface>().get())
        {
            auto animated_renderable = std::make_shared<AnimatedRenderable>(renderable, animator->register_animateable());
            animating_renderables.push_back(animated_renderable);
            auto animation = std::make_shared<RenderableAnimation>(
                animated_renderable,
                animated_renderable->handle,
                config->get_animation_definitions()[(int)AnimateableEvent::window_close],
                container.get_visible_area(),
                geom::Rectangle{},
                mir::geometry::Rectangle{window->top_left(), window->size()});

            animator->append(animation);
        }
    }

}

std::vector<RenderData> const& RenderDataManager::get()
{
    std::lock_guard lock(mutex);
    copy_for_renderer.clear();
    if (render_data.capacity() > copy_for_renderer.capacity())
        copy_for_renderer.reserve(render_data.capacity());
    std::ranges::copy(render_data,
        std::back_inserter(copy_for_renderer));
    return copy_for_renderer;
}

void RenderDataManager::set_last(mir::graphics::RenderableList const& last)
{
    std::lock_guard lock(mutex);
    for (auto& item : render_data)
    {
        for (auto const& renderable : last)
        {

        }
    }
    last_renderables = last;
}

