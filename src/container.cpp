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

#define MIR_LOG_COMPONENT "container"
#define GLM_ENABLE_EXPERIMENTAL

#include "container.h"
#include "container_group_container.h"
#include "container_listener.h"
#include "layout_scheme.h"
#include "leaf_container.h"
#include "output_interface.h"
#include "parent_container.h"
#include <glm/gtx/transform.hpp>
#include <mir/log.h>

using namespace miracle;

ContainerType miracle::container_type_from_string(std::string const& str)
{
    if (str == "tiled")
        return ContainerType::leaf;
    else if (str == "shell")
        return ContainerType::shell;
    else
        return ContainerType::none;
}

glm::mat4 Container::get_workspace_transform() const
{
    auto const workspace = get_workspace();
    if (!workspace)
        return glm::mat4(1.f);

    return workspace->transform();
}

glm::mat4 Container::get_output_transform() const
{
    auto const output = get_output();
    if (!output)
        return glm::mat4(1.f);

    return output->get_transform();
}

std::shared_ptr<LeafContainer> Container::as_leaf(std::shared_ptr<Container> const& container)
{
    return std::dynamic_pointer_cast<LeafContainer>(container);
}

std::shared_ptr<ParentContainer> Container::as_parent(std::shared_ptr<Container> const& container)
{
    return std::dynamic_pointer_cast<ParentContainer>(container);
}

std::shared_ptr<ContainerGroupContainer> Container::as_group(std::shared_ptr<Container> const& container)
{
    return std::dynamic_pointer_cast<ContainerGroupContainer>(container);
}

bool Container::is_leaf()
{
    return get_type() == ContainerType::leaf;
}

bool Container::is_lane()
{
    return get_type() == ContainerType::parent;
}

float Container::get_percent_of_parent() const
{
    float percent = 1.f;

    if (auto locked_parent = get_parent().lock())
    {
        switch (locked_parent->get_scheme())
        {
        case LayoutScheme::horizontal:
            percent = static_cast<float>(get_logical_area().size.width.as_int())
                / static_cast<float>(locked_parent->get_logical_area().size.width.as_int());
            break;
        case LayoutScheme::vertical:
            percent = static_cast<float>(get_logical_area().size.height.as_int())
                / static_cast<float>(locked_parent->get_logical_area().size.height.as_int());
            break;
        case LayoutScheme::tabbing:
        case LayoutScheme::stacking:
            percent = is_focused() ? 1.f : 0.f;
            break;
        default:
            break;
        }
    }

    return percent;
}

namespace
{
bool has_neighbor(Container const* container, LayoutScheme direction, size_t cannot_be_index)
{
    auto const parent = container->get_parent().lock();
    if (!parent)
        return false;

    auto const parent_container = Container::as_parent(parent);
    if (!parent_container)
        return false;

    if (parent_container->get_direction() != direction)
        return has_neighbor(parent_container.get(), direction, cannot_be_index);

    auto const index = parent_container->get_index_of_node(container);
    return (parent_container->num_nodes() > 1 && (index != cannot_be_index))
        || has_neighbor(parent_container.get(), direction, cannot_be_index);
}

bool has_right_neighbor(Container const* container)
{
    auto const shared_parent = container->get_parent().lock();
    if (!shared_parent)
        return false;

    auto const parent_container = Container::as_parent(shared_parent);
    if (!parent_container)
        return false;

    return has_neighbor(container, LayoutScheme::horizontal, parent_container->num_nodes() - 1);
}

bool has_bottom_neighbor(Container const* container)
{
    auto const shared_parent = container->get_parent().lock();
    if (!shared_parent)
        return false;

    auto const parent_container = Container::as_parent(shared_parent);
    if (!parent_container)
        return false;

    return has_neighbor(container, LayoutScheme::vertical, parent_container->num_nodes() - 1);
}

bool has_left_neighbor(Container const* container)
{
    auto const shared_parent = container->get_parent().lock();
    if (!shared_parent)
        return false;
    return has_neighbor(container, LayoutScheme::horizontal, 0);
}

bool has_top_neighbor(Container const* container)
{
    auto shared_parent = container->get_parent().lock();
    if (!shared_parent)
        return false;
    return has_neighbor(container, LayoutScheme::vertical, 0);
}
}

std::array<bool, static_cast<size_t>(Direction::MAX)> Container::get_neighbors() const
{
    return {
        has_top_neighbor(this),
        has_left_neighbor(this),
        has_bottom_neighbor(this),
        has_right_neighbor(this)
    };
}

void Container::mark(
    std::string const& mark,
    bool add,
    bool toggle)
{
    if (toggle)
    {
        // If we're toggling and the mark is already available,
        // we can erase it and return. If we are toggling and
        // the mark is not available, we can drop through to
        // see whether or not we are adding the new mark.
        auto const it = std::ranges::find(marks, mark);
        if (it != marks.end())
        {
            marks.erase(it);
            return;
        }
    }

    if (add)
        marks.push_back(mark);
    else
        marks = { mark };

    for_each_observer([this](ContainerListener* listener)
    {
        listener->on_container_mark(*this);
    });
}

void Container::unmark(std::string const& mark)
{
    auto const it = std::ranges::find(marks, mark);
    if (it != marks.end())
        marks.erase(it);

    for_each_observer([this](ContainerListener* listener)
    {
        listener->on_container_mark(*this);
    });
}

void Container::unmark_all()
{
    marks.clear();

    for_each_observer([this](ContainerListener* listener)
    {
        listener->on_container_mark(*this);
    });
}

std::vector<std::string> const& Container::get_marks() const
{
    return marks;
}
