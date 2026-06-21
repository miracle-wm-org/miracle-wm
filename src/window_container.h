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

#ifndef MIRACLE_WINDOW_CONTAINER_H
#define MIRACLE_WINDOW_CONTAINER_H

#include "container.h"
#include "container_effect.h"
#include "synchronized_recursive.h"
#include <optional>

namespace miracle
{
class WindowController;
class RenderDataManager;

/// An intermediate container class for containers that directly hold a window.
///
/// This class extends [Container] with methods that are meaningful only for
/// containers that wrap an actual [miral::Window] (e.g. [LeafContainer],
/// [PluginManagedContainer], [ShellComponentContainer]).  [ParentContainer]
/// does not inherit from this class because it does not directly hold a window
/// and has no use for these methods.
class WindowContainer : public Container
{
public:
    WindowContainer(uint64_t id, std::shared_ptr<RenderDataManager> const& rdm, std::shared_ptr<WindowController> const& window_controller, bool enable_render_data = true);
    ~WindowContainer() override;
    virtual void handle_ready() = 0;

    /// Handle a client-requested modification of this container's window.
    ///
    /// \param hidden true when this container's workspace is not currently
    ///        rendered (a background workspace or a hidden scratchpad). When
    ///        hidden, a requested window-state change must be deferred until the
    ///        container is next shown, so its workspace is not revealed
    ///        prematurely. Non-state modifications are still applied immediately.
    virtual void handle_modify(miral::WindowSpecification const&, bool hidden) = 0;
    virtual void handle_request_move(MirInputEvent const* input_event) = 0;
    virtual void on_open();
    virtual bool needs_outline() const;
    virtual void on_move_to(mir::geometry::Point const& top_left) = 0;
    virtual void on_resize(mir::geometry::Size const& size) = 0;
    virtual mir::geometry::Rectangle confirm_placement(
        MirWindowState, mir::geometry::Rectangle const&)
        = 0;

    virtual bool resize(Direction direction, int pixels) = 0;
    virtual bool toggle_fullscreen() = 0;
    virtual bool select_next(Direction direction) = 0;
    virtual bool move(Direction direction) = 0;
    virtual bool move_by(Direction direction, int pixels) = 0;
    using Container::move_by; // un-hide Container::move_by(float, float)
    virtual bool move_to(Container& other) = 0;
    using Container::move_to; // un-hide Container::move_to(int, int, bool)
    virtual bool is_fullscreen() const = 0;
    virtual bool drag_start() = 0;
    virtual bool drag_stop() = 0;
    virtual void drag(int x, int y) = 0;

    uint32_t animation_handle() const override;
    void animation_handle(uint32_t) override;
    void set_workspace_transform(glm::mat4 const& transform) override;
    void set_workspace_alpha(float a) override;
    glm::mat4 get_workspace_transform() const override;
    void set_animation_transform(glm::mat4 transform) override;
    void set_animation_alpha(float a) override;
    glm::mat4 get_animation_transform() const override;
    float get_alpha() const;
    void on_focus_gained() override;
    virtual void on_focus_lost();

    /// Associate this container with a \p window.
    ///
    /// \param window to associate
    void associate_to_window(miral::Window const& window);

    [[nodiscard]] std::optional<miral::Window> window() const override { return window_sync.lock()->window_; }
    [[nodiscard]] bool has_render_data() const { return window_sync.lock()->render_id.has_value(); }

    /// Push this container's current output area into its render data.
    ///
    /// The renderer culls a window when its stored output area no longer
    /// overlaps the viewport, so this must be refreshed whenever the output's
    /// position or size changes (e.g. monitors are repositioned).
    void update_output_area();

    /// Get the window transform.
    glm::mat4 get_window_transform() const;

    /// Get the window-level alpha (does not include animation or workspace effects).
    float get_window_alpha() const;

    /// Set the transformation on the window.
    ///
    /// This transformation is applied before the animation transform.
    ///
    /// \param t
    virtual void set_window_transform(glm::mat4 const& t);

    /// Set the alpha of the window.
    ///
    /// This alpha is blended with both the animation and workspace alpha.
    ///
    /// \param alpha
    virtual void set_window_alpha(float alpha);
    virtual void set_window_shader_id(std::optional<uint8_t> shader_id);

    /// Retrieve the visible area of the container.
    ///
    /// This area may be different from the logical area and be used to clip it.
    ///
    /// \returns the visible area
    [[nodiscard]] virtual geom::Rectangle get_visible_area() const = 0;
    virtual bool matches(ContainerScope const&) const = 0;

    /// Check if animations are turned on for this type of window.
    virtual bool can_animate();

protected:
    void update_window_margins(int border_size, bool entering_fullscreen);

    struct State
    {
        miral::Window window_;
        bool resizable_ = true;
        bool movable_ = true;
        std::optional<uint32_t> render_id;
        uint32_t animation_handle_;
        ContainerEffect workspace_effect;
        ContainerEffect window_effect;
        ContainerEffect animation_effect;
    };

    void rerender();
    std::weak_ptr<RenderDataManager> rdm;
    std::shared_ptr<WindowController> window_controller_;
    SynchronisedRecursive<State> window_sync;

private:
    bool enable_render_data_ = true;
};

} // namespace miracle

#endif // MIRACLE_WINDOW_CONTAINER_H
