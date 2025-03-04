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

#include "animating_surface.h"

using namespace miracle;

AnimatingSurface::AnimatingSurface(
    std::shared_ptr<mir::scene::Surface> const& surface,
    AnimationHandle handle,
    AnimationDefinition definition,
    mir::geometry::Rectangle const& from,
    mir::geometry::Rectangle const& to,
    mir::geometry::Rectangle const& current
) : Animation(handle, definition, from, to, current),
    surface_(surface)
{
}

bool AnimatingSurface::input_area_contains(const mir::geometry::Point& point) const
{
    return surface_->input_area_contains(point);
}

mir::input::InputReceptionMode AnimatingSurface::reception_mode() const
{
    return surface_->reception_mode();
}

void AnimatingSurface::consume(const std::shared_ptr<MirEvent const>& event)
{
    surface_->consume(event);
}

bool AnimatingSurface::visible_on_lock_screen() const
{
    return surface_->visible_on_lock_screen();
}

mir::geometry::Displacement AnimatingSurface::content_offset() const
{
    return surface_->content_offset();
}

std::shared_ptr<mir::frontend::BufferStream> AnimatingSurface::primary_buffer_stream() const
{
    return surface_->primary_buffer_stream();
}

const mir::wayland::Weak<mir::frontend::WlSurface>& AnimatingSurface::wayland_surface()
{
    return surface_->wayland_surface();
}

void AnimatingSurface::register_interest(const std::weak_ptr<mir::scene::SurfaceObserver>& observer)
{
    surface_->register_interest(observer);
}

void AnimatingSurface::register_interest(const std::weak_ptr<mir::scene::SurfaceObserver>& observer, mir::Executor& executor)
{
    surface_->register_interest(observer, executor);
}

void AnimatingSurface::register_early_observer(const std::weak_ptr<mir::scene::SurfaceObserver>& observer, mir::Executor& executor)
{
    surface_->register_early_observer(observer, executor);
}

void AnimatingSurface::unregister_interest(const mir::scene::SurfaceObserver& observer)
{
    surface_->unregister_interest(observer);
}

void AnimatingSurface::initial_placement_done()
{
    surface_->initial_placement_done();
}

std::string AnimatingSurface::name() const
{
    return surface_->name();
}

mir::geometry::Size AnimatingSurface::content_size() const
{
    return surface_->content_size();
}

mir::geometry::Rectangle AnimatingSurface::input_bounds() const
{
    return surface_->input_bounds();
}

mir::geometry::Point AnimatingSurface::top_left() const
{
    return surface_->top_left();
}

mir::geometry::Size AnimatingSurface::window_size() const
{
    return surface_->window_size();
}

mir::graphics::RenderableList AnimatingSurface::generate_renderables(mir::compositor::CompositorID id) const
{
    return surface_->generate_renderables(id);
}

MirWindowType AnimatingSurface::type() const
{
    return surface_->type();
}

MirWindowState AnimatingSurface::state() const
{
    return surface_->state();
}

mir::scene::SurfaceStateTracker AnimatingSurface::state_tracker() const
{
    return surface_->state_tracker();
}

void AnimatingSurface::hide()
{
    surface_->hide();
}

void AnimatingSurface::show()
{
    surface_->show();
}

bool AnimatingSurface::visible() const
{
    return surface_->visible();
}

void AnimatingSurface::move_to(const mir::geometry::Point& top_left)
{
    surface_->move_to(top_left);
}

void AnimatingSurface::set_input_region(const std::vector<mir::geometry::Rectangle>& region)
{
    surface_->set_input_region(region);
}

std::vector<mir::geometry::Rectangle> AnimatingSurface::get_input_region() const
{
    return surface_->get_input_region();
}

void AnimatingSurface::resize(const mir::geometry::Size& window_size)
{
    surface_->resize(window_size);
}

void AnimatingSurface::set_transformation(const glm::mat4& t)
{
    surface_->set_transformation(t);
}

void AnimatingSurface::set_alpha(float alpha)
{
    surface_->set_alpha(alpha);
}

void AnimatingSurface::set_orientation(MirOrientation orientation)
{
    surface_->set_orientation(orientation);
}

void AnimatingSurface::set_cursor_image(const std::shared_ptr<mir::graphics::CursorImage>& image)
{
    surface_->set_cursor_image(image);
}

std::shared_ptr<mir::graphics::CursorImage> AnimatingSurface::cursor_image() const
{
    return surface_->cursor_image();
}

void AnimatingSurface::set_reception_mode(mir::input::InputReceptionMode mode)
{
    surface_->set_reception_mode(mode);
}

void AnimatingSurface::request_client_surface_close()
{
    surface_->request_client_surface_close();
}

std::shared_ptr<mir::scene::Surface> AnimatingSurface::parent() const
{
    return surface_->parent();
}

int AnimatingSurface::configure(MirWindowAttrib attrib, int value)
{
    return surface_->configure(attrib, value);
}

int AnimatingSurface::query(MirWindowAttrib attrib) const
{
    return surface_->query(attrib);
}

void AnimatingSurface::rename(const std::string& title)
{
    surface_->rename(title);
}

void AnimatingSurface::set_streams(const std::list<mir::scene::StreamInfo>& streams)
{
    surface_->set_streams(streams);
}

void AnimatingSurface::set_confine_pointer_state(MirPointerConfinementState state)
{
    surface_->set_confine_pointer_state(state);
}

MirPointerConfinementState AnimatingSurface::confine_pointer_state() const
{
    return surface_->confine_pointer_state();
}
void AnimatingSurface::placed_relative(const mir::geometry::Rectangle& placement)
{
    surface_->placed_relative(placement);
}

MirDepthLayer AnimatingSurface::depth_layer() const
{
    return surface_->depth_layer();
}

void AnimatingSurface::set_depth_layer(MirDepthLayer depth_layer)
{
    surface_->set_depth_layer(depth_layer);
}

void AnimatingSurface::set_visible_on_lock_screen(bool visible)
{
    surface_->set_visible_on_lock_screen(visible);
}

std::optional<mir::geometry::Rectangle> AnimatingSurface::clip_area() const
{
    return surface_->clip_area();
}

void AnimatingSurface::set_clip_area(const std::optional<mir::geometry::Rectangle>& area)
{
    surface_->set_clip_area(area);
}

MirWindowFocusState AnimatingSurface::focus_state() const
{
    return surface_->focus_state();
}

void AnimatingSurface::set_focus_state(MirWindowFocusState focus_state)
{
    surface_->set_focus_state(focus_state);
}

auto AnimatingSurface::application_id() const -> std::string
{
     return surface_->application_id();
}

void AnimatingSurface::set_application_id(const std::string& application_id)
{
    surface_->set_application_id(application_id);
}

std::weak_ptr<mir::scene::Session> AnimatingSurface::session() const
{
    return surface_->session();
}

void AnimatingSurface::set_window_margins(mir::geometry::DeltaY top, mir::geometry::DeltaX left, mir::geometry::DeltaY bottom, mir::geometry::DeltaX right)
{
    surface_->set_window_margins(top, left, bottom, right);
}

MirFocusMode AnimatingSurface::focus_mode() const
{
    return surface_->focus_mode();
}

void AnimatingSurface::set_focus_mode(MirFocusMode focus_mode)
{
    surface_->set_focus_mode(focus_mode);
}

void AnimatingSurface::on_tick(AnimationStepResult const& result)
{
    if (result.transform)
        set_transformation(result.transform.value());
}