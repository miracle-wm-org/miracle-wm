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
#include "window_controller.h"

miracle::WindowContainer::WindowContainer(
    uint64_t id,
    std::shared_ptr<RenderDataManager> const& rdm,
    std::shared_ptr<WindowController> const& window_controller,
    bool enable_render_data) :
    Container(id),
    rdm(rdm),
    window_controller_(window_controller),
    enable_render_data_(enable_render_data)
{
}

miracle::WindowContainer::~WindowContainer()
{
    auto const state = window_sync.lock();
    if (state->render_id.has_value())
        if (auto const locked = rdm.lock())
            locked->remove(state->render_id.value());
}

void miracle::WindowContainer::associate_to_window(miral::Window const& window)
{
    window_sync.lock()->window_ = window;
    auto const workspace = get_workspace();
    auto const output = get_output();
    glm::mat4 workspace_transform(1.f);
    if (workspace)
        workspace_transform = workspace->transform();
    if (output)
        workspace_transform = output->get_transform() * workspace_transform;
    if (enable_render_data_)
    {
        if (auto const locked = rdm.lock())
        {
            window_sync.lock()->render_id = locked->add({
                RenderData {
                            .surface = window.operator std::shared_ptr<mir::scene::Surface>().get(),
                            .needs_outline = needs_outline(),
                            .is_focused = is_focused(),
                            .transform = get_animation_transform(),
                            .workspace_transform = workspace_transform,
                            .output_area = get_output()->get_area() }
            });
        }
    }
}

uint32_t miracle::WindowContainer::animation_handle() const
{
    return window_sync.lock()->animation_handle_;
}

void miracle::WindowContainer::animation_handle(uint32_t handle)
{
    window_sync.lock()->animation_handle_ = handle;
}

void miracle::WindowContainer::set_workspace_transform(glm::mat4 const& transform)
{
    auto state = window_sync.lock();
    state->workspace_effect.transform = transform;
    if (state->render_id.has_value())
        if (auto const rdm_locked = rdm.lock())
            rdm_locked->workspace_transform_change(state->render_id.value(), transform);
    rerender();
}

void miracle::WindowContainer::set_workspace_alpha(float a)
{
    window_sync.lock()->workspace_effect.alpha = a;
    rerender();
}

glm::mat4 miracle::WindowContainer::get_window_transform() const
{
    return window_sync.lock()->window_effect.transform;
}

float miracle::WindowContainer::get_window_alpha() const
{
    return window_sync.lock()->window_effect.alpha;
}

void miracle::WindowContainer::set_window_transform(glm::mat4 const& t)
{
    auto state = window_sync.lock();
    state->window_effect.transform = t;
    if (state->render_id.has_value())
    {
        if (auto const rdm_locked = rdm.lock())
        {
            auto const combined = state->window_effect.blend(state->animation_effect);
            rdm_locked->transform_change(state->render_id.value(), combined.transform);
        }
    }
    rerender();
}

void miracle::WindowContainer::set_window_alpha(float alpha)
{
    window_sync.lock()->window_effect.alpha = alpha;
    rerender();
}

bool miracle::WindowContainer::can_animate()
{
    return true;
}

void miracle::WindowContainer::set_animation_transform(glm::mat4 transform)
{
    auto state = window_sync.lock();
    state->animation_effect.transform = transform;
    if (state->render_id.has_value())
    {
        if (auto const rdm_locked = rdm.lock())
        {
            auto const combined = state->window_effect.blend(state->animation_effect);
            rdm_locked->transform_change(state->render_id.value(), combined.transform);
        }
    }
    rerender();
}

glm::mat4 miracle::WindowContainer::get_workspace_transform() const
{
    return window_sync.lock()->workspace_effect.transform;
}

void miracle::WindowContainer::set_animation_alpha(float a)
{
    window_sync.lock()->animation_effect.alpha = a;
    rerender();
}

glm::mat4 miracle::WindowContainer::get_animation_transform() const
{
    return window_sync.lock()->animation_effect.transform;
}

float miracle::WindowContainer::get_alpha() const
{
    auto const state = window_sync.lock();
    return state->workspace_effect.alpha * state->window_effect.alpha * state->animation_effect.alpha;
}

void miracle::WindowContainer::on_focus_gained()
{
    if (auto sh_parent = get_parent().lock())
        sh_parent->on_focus_gained();
    auto const state = window_sync.lock();
    if (state->render_id.has_value())
        if (auto const rdm_locked = rdm.lock())
            rdm_locked->focus_change(state->render_id.value(), true);
}

void miracle::WindowContainer::on_focus_lost()
{
    auto const state = window_sync.lock();
    if (state->render_id.has_value())
        if (auto const rdm_locked = rdm.lock())
            rdm_locked->focus_change(state->render_id.value(), false);
}

void miracle::WindowContainer::rerender()
{
    // A hack to trigger a rerender on the surface by re-applying its transformation.
    auto const w = window().value();
    if (auto const surface = w.operator std::shared_ptr<mir::scene::Surface>())
    {
        auto const state = window_sync.lock();
        auto const combined = state->workspace_effect.blend(state->window_effect.blend(state->animation_effect));
        surface->set_transformation(combined.transform);
        surface->set_alpha(combined.alpha);
    }
}

bool miracle::WindowContainer::needs_outline() const
{
    return true;
}

void miracle::WindowContainer::on_open()
{
    if (window_controller_)
        window_controller_->open(window_sync.lock()->window_);
}
