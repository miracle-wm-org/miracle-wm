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

namespace
{
geom::Rectangle resize_internal(Container* container, MirResizeEdge edge, int x_diff, int y_diff)
{
    auto const current_rectangle = container->get_logical_area();
    int const current_x = current_rectangle.top_left.x.as_int();
    int const current_y = current_rectangle.top_left.y.as_int();
    int const current_width = current_rectangle.size.width.as_int();
    int const current_height = current_rectangle.size.height.as_int();

    int new_x = current_x;
    int new_y = current_y;
    int new_width = current_width;
    int new_height = current_height;
    int const min_height = container->get_min_height();
    int const min_width = container->get_min_width();

    auto const set_north = [&]
    {
        new_height = std::max(current_height + y_diff, min_height);
        new_y = current_y + (current_height - new_height);
    };

    auto const set_south = [&]
    {
        new_height = std::max(current_height + y_diff, min_height);
    };

    auto const set_east = [&]
    {
        new_width = std::max(current_width + x_diff, min_width);
    };

    auto const set_west = [&]
    {
        new_width = std::max(current_width + x_diff, min_width);
        new_x = current_x + (current_width - new_width);
    };

    switch (edge)
    {
    case mir_resize_edge_north:
        set_north();
        break;
    case mir_resize_edge_south:
        set_south();
        break;
    case mir_resize_edge_east:
        set_east();
        break;
    case mir_resize_edge_west:
        set_west();
        break;
    case mir_resize_edge_northeast:
        set_north();
        set_east();
        break;
    case mir_resize_edge_northwest:
        set_north();
        set_west();
        break;
    case mir_resize_edge_southeast:
        set_south();
        set_east();
        break;
    case mir_resize_edge_southwest:
        set_south();
        set_west();
        break;
    default:
        break;
    }

    return geom::Rectangle(geom::Point(new_x, new_y), geom::Size(new_width, new_height));
}
}

void Container::execute_resize(Container* container, MirResizeEdge edge, float x, float y)
{
    bool const with_animations = true;
    auto const current_rectangle = container->get_logical_area();
    auto const next_rectangle = resize_internal(container, edge, static_cast<int>(x), static_cast<int>(y));
    auto const width_diff = current_rectangle.size.width.as_int() - next_rectangle.size.width.as_int();
    auto const height_diff = current_rectangle.size.height.as_int() - next_rectangle.size.height.as_int();

    switch (edge)
    {
    case mir_resize_edge_north:
    {
        auto const north = container->neighbor_north();
        if (!north)
        {
            mir::log_info("Cannot resize container from south without north neighbor");
            return;
        }

        auto const north_rectangle = resize_internal(north.get(), mir_resize_edge_south, 0, height_diff);
        container->set_logical_area(next_rectangle, with_animations);
        north->set_logical_area(north_rectangle, with_animations);
        container->commit_changes();
        north->commit_changes();
        break;
    }
    case mir_resize_edge_south:
    {
        auto const south = container->neighbor_south();
        if (!south)
        {
            mir::log_info("Cannot resize container from south without south neighbor");
            return;
        }

        auto const south_rectangle = resize_internal(south.get(), mir_resize_edge_north, 0, height_diff);
        container->set_logical_area(next_rectangle, with_animations);
        south->set_logical_area(south_rectangle, with_animations);
        container->commit_changes();
        south->commit_changes();
        break;
    }
    case mir_resize_edge_east:
    {
        auto const east = container->neighbor_east();
        if (!east)
        {
            mir::log_info("Cannot resize container from east without east neighbor");
            return;
        }

        auto const east_rectangle = resize_internal(east.get(), mir_resize_edge_west, width_diff, 0);
        container->set_logical_area(next_rectangle, with_animations);
        east->set_logical_area(east_rectangle, with_animations);
        container->commit_changes();
        east->commit_changes();
        break;
    }
    case mir_resize_edge_west:
    {
        auto const west = container->neighbor_west();
        if (!west)
        {
            mir::log_info("Cannot resize container from west without west neighbor");
            return;
        }

        auto const west_rectangle = resize_internal(west.get(), mir_resize_edge_east, width_diff, 0);
        container->set_logical_area(next_rectangle, with_animations);
        west->set_logical_area(west_rectangle, with_animations);
        container->commit_changes();
        west->commit_changes();
        break;
    }
    case mir_resize_edge_northeast:
        break;
    case mir_resize_edge_northwest:
        break;
    case mir_resize_edge_southeast:
        break;
    case mir_resize_edge_southwest:
        break;
    default:
        break;
    }
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
std::shared_ptr<Container> get_neighbor(
    Container const* container,
    LayoutScheme direction,
    std::function<std::shared_ptr<Container>(ParentContainer const&, size_t)>&& predicate)
{
    auto const parent = container->get_parent().lock();
    if (!parent)
        return nullptr;

    if (parent->get_direction() != direction)
        return get_neighbor(parent.get(), direction, std::move(predicate));

    auto const maybe_index = parent->get_index_of_node(container);
    if (!maybe_index)
    {
        mir::log_error("get_neighbor: parent did not contain the child");
        return nullptr;
    }

    return predicate(*parent, *maybe_index);
}

std::shared_ptr<Container> get_north_neighbor(Container const* container)
{
    return get_neighbor(container, LayoutScheme::vertical, [&](ParentContainer const& parent, size_t container_index) -> std::shared_ptr<Container>
    {
        if (container_index == 0)
            return nullptr;

        if (auto const neighbor = parent.at(container_index - 1))
            return neighbor;

        return nullptr;
    });
}

std::shared_ptr<Container> get_east_neighbor(Container const* container)
{
    return get_neighbor(container, LayoutScheme::horizontal, [&](ParentContainer const& parent, size_t container_index) -> std::shared_ptr<Container>
    {
        if (auto const neighbor = parent.at(container_index + 1))
            return neighbor;

        return nullptr;
    });
}

std::shared_ptr<Container> get_south_neighbor(Container const* container)
{
    return get_neighbor(container, LayoutScheme::vertical, [&](ParentContainer const& parent, size_t container_index) -> std::shared_ptr<Container>
    {
        if (auto const neighbor = parent.at(container_index + 1))
            return neighbor;

        return nullptr;
    });
}

std::shared_ptr<Container> get_west_neighbor(Container const* container)
{
    return get_neighbor(container, LayoutScheme::horizontal, [&](ParentContainer const& parent, size_t container_index) -> std::shared_ptr<Container>
    {
        if (container_index == 0)
            return nullptr;

        if (auto const neighbor = parent.at(container_index - 1))
            return neighbor;

        return nullptr;
    });
}

bool has_right_neighbor(Container const* container)
{
    return get_east_neighbor(container) != nullptr;
}

bool has_bottom_neighbor(Container const* container)
{
    return get_south_neighbor(container) != nullptr;
}

bool has_left_neighbor(Container const* container)
{
    return get_west_neighbor(container) != nullptr;;
}

bool has_top_neighbor(Container const* container)
{
    return get_north_neighbor(container) != nullptr;
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

std::shared_ptr<Container> Container::neighbor_north() const
{
    return get_north_neighbor(this);
}

std::shared_ptr<Container> Container::neighbor_east() const
{
    return get_east_neighbor(this);
}

std::shared_ptr<Container> Container::neighbor_south() const
{
    return get_south_neighbor(this);
}

std::shared_ptr<Container> Container::neighbor_west() const
{
    return get_west_neighbor(this);
}
