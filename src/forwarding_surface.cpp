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

#include "forwarding_surface.h"

using namespace miracle;

namespace
{
class RenderableForwarder : public mir::graphics::Renderable
{
public:
    explicit RenderableForwarder(
        std::shared_ptr<Renderable> const& r,
        std::shared_ptr<mir::scene::Surface const> const& surface) :
        renderable(r),
        animating_surface(surface)
    {
    }

    ID id() const override
    {
        return renderable ? renderable->id() : nullptr;
    }

    std::shared_ptr<mir::graphics::Buffer> buffer() const override
    {
        return renderable ? renderable->buffer() : nullptr;
    }

    mir::geometry::Rectangle screen_position() const override
    {
        return renderable ? renderable->screen_position() : mir::geometry::Rectangle {};
    }

    mir::geometry::RectangleD src_bounds() const override
    {
        return renderable ? renderable->src_bounds() : mir::geometry::RectangleD {};
    }

    std::optional<mir::geometry::Rectangle> clip_area() const override
    {
        return renderable ? renderable->clip_area() : std::nullopt;
    }

    float alpha() const override
    {
        return renderable ? renderable->alpha() : 1.0f;
    }

    glm::mat4 transformation() const override
    {
        return renderable ? renderable->transformation() : glm::mat4(1.0f);
    }

    bool shaped() const override
    {
        return renderable && renderable->shaped();
    }

    auto surface_if_any() const -> std::optional<mir::scene::Surface const*> override
    {
        return animating_surface.get();
    }

    MirOrientation orientation() const override { return mir_orientation_normal; }
    auto mirror_mode() const -> MirMirrorMode override
    {
        return renderable ? renderable->mirror_mode() : mir_mirror_mode_none;
    }

#ifdef MIR_VERSION_2_24_OR_GREATER
    auto opaque_region() const -> std::optional<mir::geometry::Rectangles> override
    {
        return renderable ? renderable->opaque_region() : std::nullopt;
    }
#endif

private:
    std::shared_ptr<Renderable> renderable;
    std::shared_ptr<mir::scene::Surface const> animating_surface;
};
}

ForwardingSurface::ForwardingSurface(std::shared_ptr<mir::scene::Surface> const& surface) :
    surface_(surface)
{
}

ForwardingSurface::~ForwardingSurface()
{
}

bool ForwardingSurface::input_area_contains(const mir::geometry::Point& point) const
{
    return false;
}

mir::input::InputReceptionMode ForwardingSurface::reception_mode() const
{
    return surface_->reception_mode();
}

void ForwardingSurface::consume(const std::shared_ptr<MirEvent const>& event)
{
}

bool ForwardingSurface::visible_on_lock_screen() const
{
    return surface_->visible_on_lock_screen();
}

mir::geometry::Displacement ForwardingSurface::content_offset() const
{
    return surface_->content_offset();
}

void ForwardingSurface::register_interest(const std::weak_ptr<mir::scene::SurfaceObserver>& observer)
{
    surface_->register_interest(observer);
}

void ForwardingSurface::register_interest(const std::weak_ptr<mir::scene::SurfaceObserver>& observer, mir::Executor& executor)
{
    surface_->register_interest(observer, executor);
}

void ForwardingSurface::register_early_observer(const std::weak_ptr<mir::scene::SurfaceObserver>& observer, mir::Executor& executor)
{
    surface_->register_early_observer(observer, executor);
}

void ForwardingSurface::unregister_interest(const mir::scene::SurfaceObserver& observer)
{
    surface_->unregister_interest(observer);
}

void ForwardingSurface::initial_placement_done()
{
    surface_->initial_placement_done();
}

std::string ForwardingSurface::name() const
{
    return surface_->name();
}

mir::geometry::Size ForwardingSurface::content_size() const
{
    return surface_->content_size();
}

mir::geometry::Rectangle ForwardingSurface::input_bounds() const
{
    return surface_->input_bounds();
}

mir::geometry::Point ForwardingSurface::top_left() const
{
    return surface_->top_left();
}

mir::geometry::Size ForwardingSurface::window_size() const
{
    return surface_->window_size();
}

mir::graphics::RenderableList ForwardingSurface::generate_renderables(mir::compositor::CompositorID id) const
{
    mir::graphics::RenderableList result;

    for (auto const& renderable : surface_->generate_renderables(id))
        result.push_back(std::make_shared<RenderableForwarder>(renderable, shared_from_this()));

    return result;
}

MirWindowType ForwardingSurface::type() const
{
    return surface_->type();
}

MirWindowState ForwardingSurface::state() const
{
    return surface_->state();
}

mir::scene::SurfaceStateTracker ForwardingSurface::state_tracker() const
{
    return surface_->state_tracker();
}

void ForwardingSurface::hide()
{
    surface_->hide();
}

void ForwardingSurface::show()
{
    surface_->show();
}

bool ForwardingSurface::visible() const
{
    return surface_->visible();
}

void ForwardingSurface::move_to(const mir::geometry::Point& top_left)
{
    surface_->move_to(top_left);
}

void ForwardingSurface::set_input_region(const std::vector<mir::geometry::Rectangle>& region)
{
    surface_->set_input_region(region);
}

std::vector<mir::geometry::Rectangle> ForwardingSurface::get_input_region() const
{
    return surface_->get_input_region();
}

void ForwardingSurface::resize(const mir::geometry::Size& window_size)
{
    surface_->resize(window_size);
}

void ForwardingSurface::set_transformation(const glm::mat4& t)
{
    surface_->set_transformation(t);
}

#ifdef MIR_VERSION_2_26_OR_GREATER
float ForwardingSurface::alpha() const
{
    return surface_->alpha();
}
#endif

void ForwardingSurface::set_alpha(float alpha)
{
    surface_->set_alpha(alpha);
}

void ForwardingSurface::set_orientation(MirOrientation orientation)
{
    surface_->set_orientation(orientation);
}

void ForwardingSurface::set_cursor_image(const std::shared_ptr<mir::graphics::CursorImage>& image)
{
    surface_->set_cursor_image(image);
}

std::shared_ptr<mir::graphics::CursorImage> ForwardingSurface::cursor_image() const
{
    return surface_->cursor_image();
}

void ForwardingSurface::set_reception_mode(mir::input::InputReceptionMode mode)
{
    surface_->set_reception_mode(mode);
}

void ForwardingSurface::request_client_surface_close()
{
    surface_->request_client_surface_close();
}

std::shared_ptr<mir::scene::Surface> ForwardingSurface::parent() const
{
    return surface_->parent();
}

int ForwardingSurface::configure(MirWindowAttrib attrib, int value)
{
    return surface_->configure(attrib, value);
}

int ForwardingSurface::query(MirWindowAttrib attrib) const
{
    return surface_->query(attrib);
}

void ForwardingSurface::rename(const std::string& title)
{
    surface_->rename(title);
}

void ForwardingSurface::set_streams(const std::list<mir::scene::StreamInfo>& streams)
{
    surface_->set_streams(streams);
}

std::list<mir::scene::StreamInfo> ForwardingSurface::get_streams() const
{
    return surface_->get_streams();
}

void ForwardingSurface::set_confine_pointer_state(MirPointerConfinementState state)
{
    surface_->set_confine_pointer_state(state);
}

MirPointerConfinementState ForwardingSurface::confine_pointer_state() const
{
    return surface_->confine_pointer_state();
}
void ForwardingSurface::placed_relative(const mir::geometry::Rectangle& placement)
{
    surface_->placed_relative(placement);
}

MirDepthLayer ForwardingSurface::depth_layer() const
{
    return surface_->depth_layer();
}

void ForwardingSurface::set_depth_layer(MirDepthLayer depth_layer)
{
    surface_->set_depth_layer(depth_layer);
}

void ForwardingSurface::set_visible_on_lock_screen(bool visible)
{
    surface_->set_visible_on_lock_screen(visible);
}

std::optional<mir::geometry::Rectangle> ForwardingSurface::clip_area() const
{
    return surface_->clip_area();
}

void ForwardingSurface::set_clip_area(const std::optional<mir::geometry::Rectangle>& area)
{
    surface_->set_clip_area(area);
}

MirWindowFocusState ForwardingSurface::focus_state() const
{
    return surface_->focus_state();
}

void ForwardingSurface::set_focus_state(MirWindowFocusState focus_state)
{
    surface_->set_focus_state(focus_state);
}

auto ForwardingSurface::application_id() const -> std::string
{
    return surface_->application_id();
}

void ForwardingSurface::set_application_id(const std::string& application_id)
{
    surface_->set_application_id(application_id);
}

std::weak_ptr<mir::scene::Session> ForwardingSurface::session() const
{
    return surface_->session();
}

void ForwardingSurface::set_window_margins(mir::geometry::DeltaY top, mir::geometry::DeltaX left, mir::geometry::DeltaY bottom, mir::geometry::DeltaX right)
{
    surface_->set_window_margins(top, left, bottom, right);
}

MirFocusMode ForwardingSurface::focus_mode() const
{
    return surface_->focus_mode();
}

void ForwardingSurface::set_focus_mode(MirFocusMode focus_mode)
{
    surface_->set_focus_mode(focus_mode);
}
