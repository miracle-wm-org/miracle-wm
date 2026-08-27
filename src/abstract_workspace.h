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

#ifndef MIRACLE_WM_WORKSPACE_INTERFACE_H
#define MIRACLE_WM_WORKSPACE_INTERFACE_H

#include "container.h"
#include "direction.h"

#include <memory>
#include <miracle/cpp/gaps.h>

namespace miracle
{
class AbstractOutput;
class Container;
class ParentContainer;

class AbstractWorkspace : public std::enable_shared_from_this<AbstractWorkspace>
{
public:
    virtual ~AbstractWorkspace() = default;

    virtual void recalculate_area() = 0;

    virtual void delete_container(std::shared_ptr<Container> const& container) = 0;
    virtual bool move_container(Direction direction, Container&) = 0;
    virtual bool add_to_root(Container& to_move) = 0;

    /// Show the workspace.
    ///
    /// \param origin the position that the animation should happen from.
    virtual void show(mir::geometry::Point const& origin) = 0;

    /// Hide the workspace.
    ///
    /// \param end the position that the workspace will end up at.
    virtual void hide(mir::geometry::Point const& end) = 0;

    /// Iterates all containers on this workspace that represent a window until the predicate is satisfied.
    /// Returns true if the predicate returned true.
    virtual bool for_each_window(std::function<bool(std::shared_ptr<WindowContainer>)> const&) const = 0;

    virtual void advise_focus_gained(std::shared_ptr<Container> const& container) = 0;

    /// Focus a window on this workspace: the last selected container if it still
    /// exists, otherwise the first window in tree order. If the workspace has no
    /// windows, focus is cleared.
    virtual void select_window() = 0;

    [[nodiscard]] virtual std::shared_ptr<AbstractOutput> get_output() const = 0;

    virtual void set_output(std::shared_ptr<AbstractOutput> const&) = 0;

    [[nodiscard]] virtual bool is_empty() const = 0;
    virtual void graft(std::shared_ptr<Container> const&) = 0;

    [[nodiscard]] virtual uint32_t id() const = 0;
    [[nodiscard]] virtual std::optional<int> num() const = 0;
    virtual void num(std::optional<int> n) = 0;
    [[nodiscard]] virtual std::optional<std::string> const& name() const = 0;
    virtual void name(std::optional<std::string> const&) = 0;

    /// Returns the current area of the workspace.
    [[nodiscard]] virtual mir::geometry::Rectangle area() const = 0;

    [[nodiscard]] virtual std::optional<Gaps> outer_gaps() const = 0;
    virtual void outer_gaps(std::optional<Gaps> const& gaps) = 0;

    [[nodiscard]] virtual std::optional<Gaps> inner_gaps() const = 0;
    virtual void inner_gaps(std::optional<Gaps> const& gaps) = 0;

    /// Sets the transformation for this workspace.
    virtual void transform(glm::mat4 const&) = 0;

    /// Retrieve the transformation for this workspace.
    ///
    /// \returns the workspace transforms
    virtual glm::mat4 transform() const = 0;

    /// Set the opacity on every window in the workspace.
    ///
    /// \param a the alpha value
    virtual void alpha(float a) = 0;

    /// Retrieve the alpha for this workspace.
    ///
    /// \returns the alpha value
    virtual float alpha() const = 0;

    /// Returns the container that is implicitly being used as a reference to add
    /// new containers to this workspace.
    [[nodiscard]] virtual ParentContainer* get_layout_container() const = 0;

    /// Json returned to IPC GET_WORKSPACES command.
    [[nodiscard]] virtual nlohmann::json get_workspaces_json(bool is_output_focused) const = 0;
    [[nodiscard]] virtual nlohmann::json to_json(bool is_output_focused) const = 0;
    [[nodiscard]] virtual std::string display_name() const = 0;
    [[nodiscard]] virtual std::shared_ptr<ParentContainer> get_root() const = 0;
    virtual void add_other_container(std::shared_ptr<Container> const& container, bool is_active) = 0;
    virtual void remove_other_container(std::shared_ptr<Container> const& container) = 0;
};
}

#endif // MIRACLE_WM_WORKSPACE_INTERFACE_H
