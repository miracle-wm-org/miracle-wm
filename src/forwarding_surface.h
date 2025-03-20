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

#ifndef ANIMATING_SURFACE_H
#define ANIMATING_SURFACE_H

#include <mir/scene/surface.h>
#include <mir/version.h>

namespace miracle
{
class CompositorState;

/// This is a surface that forwards all of its functionality
/// to the provided surface. It is useful when you want everything
/// about an existing surface except for the Renderable will
/// resolve to this surface instead.
///
/// This class is specifically useful with the [DyingSurfaceManager].
/// Other than that, its use seems minimal at best.
class ForwardingSurface : public mir::scene::Surface
{
public:
    explicit ForwardingSurface(std::shared_ptr<mir::scene::Surface> const&);
    ~ForwardingSurface() override;
    bool input_area_contains(const mir::geometry::Point& point) const override;
    mir::input::InputReceptionMode reception_mode() const override;
    void consume(const std::shared_ptr<MirEvent const>& event) override;
    auto visible_on_lock_screen() const -> bool override;
    auto content_offset() const -> mir::geometry::Displacement override;
    auto primary_buffer_stream() const -> std::shared_ptr<mir::frontend::BufferStream> override;
    auto wayland_surface() -> const mir::wayland::Weak<mir::frontend::WlSurface>& override;
    void register_interest(const std::weak_ptr<mir::scene::SurfaceObserver>& observer) override;
    void register_interest(const std::weak_ptr<mir::scene::SurfaceObserver>& observer, mir::Executor& executor) override;
    void register_early_observer(const std::weak_ptr<mir::scene::SurfaceObserver>& observer, mir::Executor& executor) override;
    void unregister_interest(const mir::scene::SurfaceObserver& observer) override;
#if (MIR_SERVER_MAJOR_VERSION > 2) || (MIR_SERVER_MAJOR_VERSION == 2 && MIR_SERVER_MINOR_VERSION > 19)
    void initial_placement_done() override;
#endif
    std::string name() const override;
    mir::geometry::Size content_size() const override;
    mir::geometry::Rectangle input_bounds() const override;
    mir::geometry::Point top_left() const override;
    mir::geometry::Size window_size() const override;
    mir::graphics::RenderableList generate_renderables(mir::compositor::CompositorID id) const override;
    MirWindowType type() const override;
    MirWindowState state() const override;
    auto state_tracker() const -> mir::scene::SurfaceStateTracker override;
    void hide() override;
    void show() override;
    bool visible() const override;
    void move_to(const mir::geometry::Point& top_left) override;
    void set_input_region(const std::vector<mir::geometry::Rectangle>& region) override;
    std::vector<mir::geometry::Rectangle> get_input_region() const override;
    void resize(const mir::geometry::Size& window_size) override;
    void set_transformation(const glm::mat4& t) override;
    void set_alpha(float alpha) override;
    void set_orientation(MirOrientation orientation) override;
    void set_cursor_image(const std::shared_ptr<mir::graphics::CursorImage>& image) override;
    std::shared_ptr<mir::graphics::CursorImage> cursor_image() const override;
    void set_reception_mode(mir::input::InputReceptionMode mode) override;
    void request_client_surface_close() override;
    std::shared_ptr<Surface> parent() const override;
    int configure(MirWindowAttrib attrib, int value) override;
    int query(MirWindowAttrib attrib) const override;
    void rename(const std::string& title) override;
    void set_streams(const std::list<mir::scene::StreamInfo>& streams) override;
    void set_confine_pointer_state(MirPointerConfinementState state) override;
    MirPointerConfinementState confine_pointer_state() const override;
    void placed_relative(const mir::geometry::Rectangle& placement) override;
    auto depth_layer() const -> MirDepthLayer override;
    void set_depth_layer(MirDepthLayer depth_layer) override;
    void set_visible_on_lock_screen(bool visible) override;
    std::optional<mir::geometry::Rectangle> clip_area() const override;
    void set_clip_area(const std::optional<mir::geometry::Rectangle>& area) override;
    auto focus_state() const -> MirWindowFocusState override;
    void set_focus_state(MirWindowFocusState focus_state) override;
    auto application_id() const -> std::string override;
    void set_application_id(const std::string& application_id) override;
    auto session() const -> std::weak_ptr<mir::scene::Session> override;
    void set_window_margins(mir::geometry::DeltaY top, mir::geometry::DeltaX left, mir::geometry::DeltaY bottom, mir::geometry::DeltaX right) override;
    auto focus_mode() const -> MirFocusMode override;
    void set_focus_mode(MirFocusMode focus_mode) override;

private:
    std::shared_ptr<mir::scene::Surface> surface_;
};
}

#endif // ANIMATING_SURFACE_H
