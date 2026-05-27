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

#ifndef MIRACLE_CONTAINER_H
#define MIRACLE_CONTAINER_H

#include "direction.h"
#include "layout_scheme.h"
#include "observer_registrar.h"
#include "plugin_handle.h"
#include "scratchpad_state.h"
#include <glm/glm.hpp>
#include <memory>
#include <mir/geometry/rectangle.h>
#include <miral/window.h>
#include <miral/window_manager_tools.h>
#include <nlohmann/json.hpp>

namespace geom = mir::geometry;

namespace miracle
{
class Config;
class LeafContainer;
class ParentContainer;
class WindowContainer;
class AbstractWorkspace;
class AbstractOutput;
class ContainerScope;
class ContainerListener;

/// A generic container.
///
/// Aligns generally with i3's concept of containers. A container is a logical
/// rectangle on the workspace that may be shown or hidden.
class Container : public std::enable_shared_from_this<Container>, public ObserverRegistrar<ContainerListener>
{
public:
    ~Container() override = default;

    /// Show the container.
    virtual void show() = 0;

    /// Hide the container.
    virtual void hide() = 0;

    /// Commits any changes made to this node to the screen. This must
    /// be call for changes to be pushed to the scene. Additionally,
    /// it is advised that this method is only called once all changes have
    /// been made for a particular operation.
    virtual void commit_changes() = 0;

    /// Retrieve the logical area of the container.
    ///
    /// \returns the logical area
    [[nodiscard]] virtual geom::Rectangle get_logical_area() const = 0;

    /// Set the logical area of the container.
    ///
    /// \param area the new area
    /// \param with_animations whether or not to animate the change
    virtual void set_logical_area(geom::Rectangle const& area, bool with_animations) = 0;

    /// Constrain the container to its current visible area.
    ///
    /// TODO: Can we remove this?
    virtual void constrain() = 0;

    /// Retrieve the parent container of the container, if one exists.
    ///
    /// \returns the parent
    virtual std::weak_ptr<ParentContainer> get_parent() const = 0;

    /// Set the parent on the container.
    ///
    /// \param parent the parent
    virtual void set_parent(std::shared_ptr<ParentContainer> const& parent) = 0;
    virtual size_t get_min_height() const = 0;
    virtual size_t get_min_width() const = 0;
    virtual void handle_raise() = 0;
    virtual bool set_size(std::optional<int> const& width, std::optional<int> const& height) = 0;
    virtual void request_horizontal_layout() = 0;
    virtual void request_vertical_layout() = 0;
    virtual void toggle_layout(bool cycle_thru_all) = 0;
    virtual void on_focus_gained() = 0;
    virtual std::shared_ptr<AbstractWorkspace> get_workspace() const = 0;
    virtual void set_workspace(std::shared_ptr<AbstractWorkspace> const&) = 0;
    virtual std::shared_ptr<AbstractOutput> get_output() const = 0;

    /// Retrieve the animation transform for the container.
    ///
    /// This does NOT include the output and workspace transforms. This is intended
    /// for use by the animation system.
    ///
    /// \returns the transform on the node
    virtual glm::mat4 get_animation_transform() const = 0;

    /// Sets the animation transform on the container.
    ///
    ///\param transform
    virtual void set_animation_transform(glm::mat4 transform) = 0;

    virtual void set_workspace_transform(glm::mat4 const& transform) = 0;
    virtual void set_workspace_alpha(float a) = 0;
    virtual glm::mat4 get_workspace_transform() const;
    virtual glm::mat4 get_output_transform() const;

    /// Set the animation alpha on the container.
    ///
    /// \param alpha
    virtual void set_animation_alpha(float const alpha) = 0;
    virtual uint32_t animation_handle() const = 0;
    virtual void animation_handle(uint32_t) = 0;
    virtual bool is_focused() const = 0;
    virtual std::optional<miral::Window> window() const = 0;
    virtual bool pinned() const = 0;
    virtual bool pinned(bool) = 0;
    virtual bool move_to(int x, int y, bool with_animations) = 0;
    virtual bool move_by(float dx, float dy) = 0;
    virtual bool toggle_tabbing() = 0;
    virtual bool toggle_stacking() = 0;
    virtual bool set_layout(LayoutScheme scheme) = 0;
    virtual bool anchored() const = 0;
    virtual void scratchpad_state(ScratchpadState) = 0;
    virtual ScratchpadState scratchpad_state() const = 0;
    // Marks the [container] with [mark]. By default, this
    /// will replace any mark that is currently on the window
    /// unless [add] is specified. [toggle] will toggle the
    /// mark on the window.
    virtual void mark(std::string const& mark, bool add, bool toggle);
    virtual void unmark(std::string const& mark);
    virtual void unmark_all();
    virtual std::vector<std::string> const& get_marks() const;
    virtual LayoutScheme get_layout() const = 0;
    virtual nlohmann::json to_json(bool is_workspace_visible) const = 0;

    /// Returns the unique ID assigned to this container at construction time.
    [[nodiscard]] uint64_t id() const { return id_; }

    [[nodiscard]] float get_percent_of_parent() const;
    static std::shared_ptr<LeafContainer> as_leaf(std::shared_ptr<Container> const&);
    static std::shared_ptr<ParentContainer> as_parent(std::shared_ptr<Container> const&);
    static std::shared_ptr<WindowContainer> as_window_container(std::shared_ptr<Container> const&);

    /// Resizes the provided \p container on the \p edge by the \p x and \p y diffs.
    static void execute_resize(Container* container, MirResizeEdge edge, float x, float y, bool with_animations);

    /// Returns the neighbor to the north of this container, or `nullptr` if not found.
    ///
    /// \returns north neighbor
    virtual std::shared_ptr<Container> neighbor_north() const;

    /// Returns the neighbor to the east of this container, or `nullptr` if not found.
    ///
    /// \returns east neighbor
    virtual std::shared_ptr<Container> neighbor_east() const;

    /// Returns the neighbor to the south of this container, or `nullptr` if not found.
    ///
    /// \returns south neighbor
    virtual std::shared_ptr<Container> neighbor_south() const;

    /// Returns the neighbor to the west of this container, or `nullptr` if not found.
    ///
    /// \returns west neighbor
    virtual std::shared_ptr<Container> neighbor_west() const;

    /// Returns the root of this container tree.
    ///
    /// \returns the root
    virtual std::shared_ptr<Container> root();

    /// Returns the handle of the plugin that created this container, if any.
    ///
    /// \returns the plugin handle
    virtual std::optional<PluginHandle> plugin_handle() const;

    [[nodiscard]] std::array<bool, (size_t)Direction::MAX> get_neighbors() const;

protected:
    Container() = default;
    explicit Container(uint64_t id) noexcept :
        id_(id)
    {
    }
    std::vector<std::string> marks;

private:
    uint64_t id_ = 0;
};

}

#endif // MIRACLE_CONTAINER_H
