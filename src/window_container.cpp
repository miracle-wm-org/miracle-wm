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

#include "window_container.h"
#include "abstract_output.h"
#include "parent_container.h"

namespace
{
inline bool needs_outline(miracle::WindowContainer const& container)
{
    auto const surface = container.window().value().operator std::shared_ptr<mir::scene::Surface>();
    container.window().value();
    return surface == nullptr || !surface->parent();
}
}

miracle::WindowContainer::WindowContainer(std::shared_ptr<RenderDataManager> const& rdm) :
    rdm(rdm)
{
}

miracle::WindowContainer::~WindowContainer()
{
    if (auto const locked = rdm.lock())
        locked->remove(render_id);
}

void miracle::WindowContainer::associate_to_window(miral::Window const& window)
{
    window_ = window;
    auto const workspace = get_workspace();
    auto const output = get_output();
    glm::mat4 workspace_transform(1.f);
    if (workspace)
        workspace_transform = workspace->transform();
    if (output)
        workspace_transform = output->get_transform() * workspace_transform;
    if (auto const locked = rdm.lock())
    {
        render_id = locked->add({
            RenderData {
                        .surface = window.operator std::shared_ptr<mir::scene::Surface>().get(),
                        .needs_outline = needs_outline(*this),
                        .is_focused = is_focused(),
                        .transform = get_animation_transform(),
                        .workspace_transform = workspace_transform,
                        .workspace_alpha = !workspace ? 1.f : workspace->alpha(),
                        .output_area = get_output()->get_area() }
        });
    }
}

uint32_t miracle::WindowContainer::animation_handle() const
{
    return animation_handle_;
}

void miracle::WindowContainer::animation_handle(uint32_t handle)
{
    animation_handle_ = handle;
}

void miracle::WindowContainer::set_workspace_transform(glm::mat4 const& transform)
{
    workspace_effect.transform = transform;
    if (auto const rdm_locked = rdm.lock())
        rdm_locked->workspace_transform_change(render_id, transform);
    rerender();
}

void miracle::WindowContainer::set_workspace_alpha(float a)
{
    workspace_effect.alpha = a;
    if (auto const rdm_locked = rdm.lock())
        rdm_locked->workspace_alpha(render_id, a);
    rerender();
}

void miracle::WindowContainer::set_window_transform(glm::mat4 const& t)
{
    window_effect.transform = t;
    rerender();
}

void miracle::WindowContainer::set_window_alpha(float alpha)
{
    window_effect.alpha = alpha;
    if (auto const rdm_locked = rdm.lock())
        rdm_locked->alpha_change(render_id, alpha);
    rerender();
}

void miracle::WindowContainer::set_animation_transform(glm::mat4 transform)
{
    animation_effect.transform = transform;
    if (auto const rdm_locked = rdm.lock())
        rdm_locked->transform_change(render_id, transform);
    rerender();
}

glm::mat4 miracle::WindowContainer::get_workspace_transform() const
{
    return workspace_effect.transform;
}

void miracle::WindowContainer::set_animation_alpha(float a)
{
    animation_effect.alpha = a;
    rerender();
}

glm::mat4 miracle::WindowContainer::get_animation_transform() const
{
    return animation_effect.transform;
}

void miracle::WindowContainer::on_focus_gained()
{
    if (auto sh_parent = get_parent().lock())
        sh_parent->on_focus_gained();
    if (auto const rdm_locked = rdm.lock())
        rdm_locked->focus_change(render_id, true);
}

void miracle::WindowContainer::on_focus_lost()
{
    if (auto const rdm_locked = rdm.lock())
        rdm_locked->focus_change(render_id, false);
}

void miracle::WindowContainer::rerender()
{
    // A hack to trigger a rerender on the surface by re-applying its transformation.
    auto const window_ = window().value();
    if (auto const surface = window_.operator std::shared_ptr<mir::scene::Surface>())
    {
        auto const combined = workspace_effect.blend(window_effect.blend(animation_effect));
        surface->set_transformation(combined.transform);
        surface->set_alpha(combined.alpha);
    }
}
