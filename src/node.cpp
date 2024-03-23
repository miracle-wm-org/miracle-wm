#include "window_metadata.h"
#include <memory>
#define MIR_LOG_COMPONENT "node"

#include "node.h"
#include "window_helpers.h"
#include "miracle_config.h"
#include <cmath>
#include <iostream>
#include <mir/log.h>

using namespace miracle;

Node::Node(
    miral::WindowManagerTools const& tools_,
    geom::Rectangle const& area,
    std::shared_ptr<MiracleConfig> const& config,
    Tree* tree)
    : tools{tools_},
      state{NodeState::lane},
      logical_area{area},
      config{config},
      tree{tree}
{
}

Node::Node(
    miral::WindowManagerTools const& tools_,
    geom::Rectangle const& area,
    std::shared_ptr<Node> parent,
    miral::Window const& window,
    std::shared_ptr<MiracleConfig> const& config,
    Tree* tree)
    : tools{tools_},
      parent{std::move(parent)},
      window{window},
      state{NodeState::window},
      logical_area{area},
      config{config},
      tree{tree}
{
}

geom::Rectangle Node::get_logical_area_internal(geom::Rectangle const& rectangle)
{
    if (parent == nullptr)
    {
        auto x = config->get_outer_gaps_x();
        auto y = config->get_outer_gaps_y();

        auto modified_logical_area = geom::Rectangle(
            geom::Point(rectangle.top_left.x.as_int() + x, rectangle.top_left.y.as_int() + y),
            geom::Size(rectangle.size.width.as_int() - 2 * x, rectangle.size.height.as_int() - 2 * y)
        );

        return modified_logical_area;
    }

    return rectangle;
}

geom::Rectangle Node::get_logical_area()
{
    return get_logical_area_internal(logical_area);
}

geom::Rectangle Node::get_visible_area()
{
    return _get_visible_from_logical(
        get_logical_area(),
        _has_right_neighbor(),
        _has_bottom_neighbor(),
        config);
}

namespace
{
struct InsertNodeInternalResult
{
    int size;
    int position;
};

InsertNodeInternalResult insert_node_internal(
    int lane_size,
    int lane_pos,
    int index,
    int node_count,
    std::function<int(int)> const& get_node_size,
    std::function<int(int)> const& get_node_position,
    std::function<void(int, int, int)> const& set_node_size_position)
{
    int new_item_size = floor((double)lane_size / (double)(node_count + 1));
    int new_item_position = lane_pos + index * new_item_size;

    int size_lost = 0;
    int prev_pos = lane_pos;
    int prev_size = 0;
    for (int i = 0; i < node_count; i++)
    {
        int node_size = get_node_size(i);
        int node_pos = get_node_position(i);

        // Each node will lose a percentage of its width that corresponds to what it can give
        // (meaning that larger nodes give more width, and lesser nodes give less width)
        double percent_size_lost =
            ((double)node_size / (double)lane_size);
        int width_to_lose = (int)floor(percent_size_lost * new_item_size);
        size_lost += width_to_lose;

        if (i == index)
        {
            prev_size = new_item_size;
            prev_pos = new_item_position;
        }

        int changed_node_size = node_size - width_to_lose;
        int changed_node_pos = prev_pos + prev_size;
        set_node_size_position(i, changed_node_size, changed_node_pos);

        prev_pos = changed_node_pos;
        prev_size = changed_node_size;
    }

    if (node_count)
    {
        new_item_size += size_lost - new_item_size;
        new_item_position -= size_lost - new_item_size;
    }

    return {new_item_size, new_item_position};
}
}

geom::Rectangle Node::create_new_node_position(int index)
{
    if (is_window())
    {
        std::cerr << "Cannot create a new node position on a window node\n";
        return {};
    }

    if (index < 0)
        index = (int)sub_nodes.size();

    pending_index = index;

    auto placement_area = get_logical_area();
    if (direction == NodeLayoutDirection::horizontal)
    {
        auto result = insert_node_internal(
            placement_area.size.width.as_int(),
            placement_area.top_left.x.as_int(),
            index,
            sub_nodes.size(),
            [&](int index) { return sub_nodes[index]->get_logical_area().size.width.as_int();},
            [&](int index) { return sub_nodes[index]->get_logical_area().top_left.x.as_int();},
            [&](int index, int size, int pos) {
                sub_nodes[index]->pending_logical_rect = {
                    geom::Point{
                        pos,
                        placement_area.top_left.y.as_int()
                    },
                    geom::Size{
                        size,
                        placement_area.size.height.as_int()
                    }};
            });
        geom::Rectangle new_node_logical_rect = {
            geom::Point{
                result.position,
                placement_area.top_left.y.as_int()
            },
            geom::Size{
                result.size,
                placement_area.size.height.as_int()
            }};
        pending_logical_rect = new_node_logical_rect;

        auto new_node_visible_rect = _get_visible_from_logical(
            new_node_logical_rect,
            (num_nodes() > 0 && pending_index != num_nodes()) || _has_right_neighbor(),
            _has_bottom_neighbor(),
            config);
        return new_node_visible_rect;
    }
    else
    {
        auto result = insert_node_internal(
            placement_area.size.height.as_int(),
            placement_area.top_left.y.as_int(),
            index,
            sub_nodes.size(),
            [&](int index) { return sub_nodes[index]->get_logical_area().size.height.as_int();},
            [&](int index) { return sub_nodes[index]->get_logical_area().top_left.y.as_int();},
            [&](int index, int size, int pos) {
                sub_nodes[index]->pending_logical_rect =  {
                    geom::Point{
                        placement_area.top_left.x.as_int(),
                        pos
                    },
                    geom::Size{
                        placement_area.size.width.as_int(),
                        size
                    }};
            });
        geom::Rectangle new_node_logical_rect = {
            geom::Point{
                placement_area.top_left.x.as_int(),
                result.position
            },
            geom::Size{
                placement_area.size.width.as_int(),
                result.size
            }};
        pending_logical_rect = new_node_logical_rect;
        auto new_node_visible_rect = _get_visible_from_logical(
            new_node_logical_rect,
            _has_right_neighbor(),
            (num_nodes() > 0 && pending_index != num_nodes()) || _has_bottom_neighbor(),
            config);
        return new_node_visible_rect;
    }
}

std::shared_ptr<Node> Node::add_window(miral::Window& new_window)
{
    if (pending_index < 0)
    {
        mir::fatal_error("Unable to add the window to the scene. Was create_new_node_position called?");
        return nullptr;
    }

    auto node = std::make_shared<Node>(
        tools,
        pending_logical_rect,
        shared_from_this(),
        new_window,
        config,
        tree);

    sub_nodes.insert(sub_nodes.begin() + pending_index, node);
    pending_index = -1;

    for (auto& other_node : sub_nodes)
    {
        if (other_node == node)
            continue;
        other_node->set_logical_area(other_node->pending_logical_rect);
    }

    return node;
}

void Node::refit_node_to_area()
{
    auto placement_area = get_logical_area();
    if (direction == NodeLayoutDirection::horizontal)
    {
        int total_width = 0;
        for (auto const& node : sub_nodes)
        {
            total_width += node->get_logical_area().size.width.as_int();
        }

        float diff_width = placement_area.size.width.as_value() - total_width;
        int diff_per_node = diff_width / sub_nodes.size();
        for (auto const& node : sub_nodes)
        {
            auto rectangle = node->get_logical_area();
            rectangle.size.width = geom::Width{rectangle.size.width.as_int() + diff_per_node};
            rectangle.size.height = geom::Height{placement_area.size.height};
            node->set_logical_area(rectangle);
        }
    }
    else
    {
        int total_height = 0;
        for (auto const& node : sub_nodes)
        {
            total_height += node->get_logical_area().size.height.as_int();
        }

        float diff_width = placement_area.size.height.as_value() - total_height;
        int diff_per_node = diff_width / sub_nodes.size();
        for (auto const& node : sub_nodes)
        {
            auto rectangle = node->get_logical_area();
            rectangle.size.width = geom::Width {placement_area.size.width};
            rectangle.size.height = geom::Height {rectangle.size.height.as_int() + diff_per_node};
            node->set_logical_area(rectangle);
        }
    }

    set_logical_area(logical_area);
}

void Node::set_logical_area(geom::Rectangle const& target_rect)
{
    if (is_window())
    {
        auto& info = tools.info_for(get_window());
        if (!window_helpers::is_window_fullscreen(info.state()))
        {
            _set_window_rectangle(target_rect);
        }
    }
    else
    {
        // We are setting the size of the lane, but each window might have an idea of how
        // its own height relates to the lane (e.g. I take up 300px of 900px lane while my
        // neighbor takes up the remaining 600px, horizontally).
        // We need to look at the target dimension and scale everyone relative to that.
        // However, the "non-main-axis" dimension will be consistent across each node.
        auto placement_area = get_logical_area();
        auto target_placement_area = get_logical_area_internal(target_rect);
        std::vector<geom::Rectangle> pending_size_updates;
        if (direction == NodeLayoutDirection::horizontal)
        {
            int total_width = 0;
            for (size_t idx = 0; idx < sub_nodes.size(); idx++)
            {
                auto item = sub_nodes[idx];
                auto item_rect = item->get_logical_area();
                double percent_width_taken = (double)item_rect.size.width.as_int() / (double)placement_area.size.width.as_int();
                int new_width = (int)ceil((double)target_placement_area.size.width.as_int() * percent_width_taken);

                geom::Rectangle new_item_rect;
                new_item_rect.size = geom::Size{
                    geom::Width{new_width},
                    target_placement_area.size.height
                };
                if (idx == 0)
                {
                    new_item_rect.top_left = geom::Point{
                        target_placement_area.top_left.x,
                        target_placement_area.top_left.y
                    };
                }
                else
                {
                    auto const& prev_rect = pending_size_updates[idx - 1];
                    new_item_rect.top_left = geom::Point{
                        geom::X{prev_rect.top_left.x.as_int() + prev_rect.size.width.as_int()},
                        target_placement_area.top_left.y
                    };
                }

                pending_size_updates.push_back(new_item_rect);
                total_width += new_width;
            }

            if (!pending_size_updates.empty())
            {
                int leftover_width = target_placement_area.size.width.as_int() - total_width;
                pending_size_updates.back().size.width = geom::Width {pending_size_updates.back().size.width.as_int() + leftover_width};
            }
        }
        else
        {
            int total_height = 0;
            for (size_t idx = 0; idx < sub_nodes.size(); idx++)
            {
                auto item = sub_nodes[idx];
                auto item_rect = item->get_logical_area();
                double percent_height_taken = static_cast<double>(item_rect.size.height.as_int()) / placement_area.size.height.as_int();
                int new_height = (int)floor((double)target_placement_area.size.height.as_int() * percent_height_taken);

                geom::Rectangle new_item_rect;
                new_item_rect.size = geom::Size{
                    target_placement_area.size.width,
                    geom::Height{new_height},
                };
                if (idx == 0)
                {
                    new_item_rect.top_left = geom::Point{
                        target_placement_area.top_left.x,
                        target_placement_area.top_left.y
                    };
                }
                else
                {
                    auto const& prev_rect = pending_size_updates[idx - 1];
                    new_item_rect.top_left = geom::Point{
                        target_placement_area.top_left.x,
                        geom::Y{prev_rect.top_left.y.as_int() + prev_rect.size.height.as_int()},
                    };
                }

                pending_size_updates.push_back(new_item_rect);
                total_height += new_height;
            }

            if (!pending_size_updates.empty())
            {
                int leftover_height = target_placement_area.size.height.as_int() - total_height;
                pending_size_updates.back().size.height = geom::Height {pending_size_updates.back().size.height.as_int() + leftover_height};
            }
        }

        for (size_t i = 0; i < sub_nodes.size(); i++)
        {
            sub_nodes[i]->set_logical_area(pending_size_updates[i]);
        }
    }


    // Important that we update the area _after_ changes have taken place!
    logical_area = target_rect;
}

std::shared_ptr<Node> Node::to_lane()
{
    if (is_lane())
        return nullptr;

    state = NodeState::lane;

    // If we want to make a new node, but our parent only has one window, and it's us...
    // then we can just return the parent
    if (parent != nullptr && parent->sub_nodes.size() == 1)
        return parent->sub_nodes[0];

    auto window_node = std::make_shared<Node>(
        tools,
        logical_area,
        shared_from_this(),
        get_window(),
        config,
        tree);
    sub_nodes.push_back(window_node);
    auto metadata = window_helpers::get_metadata(window, tools);
    metadata->associate_to_node(window_node);
    window = miral::Window();
    return window_node;
}

void Node::insert_node(std::shared_ptr<Node> const& node, int index)
{
    auto area_with_gaps = create_new_node_position(index);
    node->parent = shared_from_this();
    node->set_logical_area(pending_logical_rect);
    sub_nodes.insert(sub_nodes.begin() + index, node);
    refit_node_to_area();
    constrain();
}

void Node::swap_nodes(std::shared_ptr<Node> const& first, std::shared_ptr<Node> const& second)
{
    auto first_index = get_index_of_node(first);
    auto second_index = get_index_of_node(second);
    sub_nodes[second_index] = first;
    sub_nodes[first_index] = second;
    set_logical_area(logical_area);
    constrain();
}

void Node::remove_node(std::shared_ptr<Node> const& node)
{
    if (is_window())
    {
        std::cerr << "Cannot remove a node from a window\n";
        return;
    }

    sub_nodes.erase(
        std::remove_if(sub_nodes.begin(), sub_nodes.end(), [&](std::shared_ptr<Node> const& content) {
            return content == node;
        }),
        sub_nodes.end()
    );

    // If we have one child AND it is a lane, THEN we can absorb all of it's children
    if (sub_nodes.size() == 1 && sub_nodes[0]->is_lane())
    {
        auto dying_lane = sub_nodes[0];
        sub_nodes.clear();
        for (auto const& sub_node : dying_lane->get_sub_nodes())
        {
            sub_nodes.push_back(sub_node);
            sub_node->parent = shared_from_this();
        }
        set_direction(dying_lane->get_direction());
    }

    refit_node_to_area();
    constrain();
}

int Node::get_index_of_node(std::shared_ptr<Node> const& node) const
{
    for (int i = 0; i < sub_nodes.size(); i++)
        if (sub_nodes[i] == node)
            return i;

    return -1;
}

int Node::get_index_of_node(Node const* node) const
{
    for (int i = 0; i < sub_nodes.size(); i++)
        if (sub_nodes[i].get() == node)
            return i;

    return -1;
}

int Node::num_nodes() const
{
    return sub_nodes.size();
}

std::shared_ptr<Node> Node::node_at(int i) const
{
    if (i < 0 || i >= num_nodes())
        return nullptr;

    return sub_nodes[i];
}

std::shared_ptr<Node> Node::find_nth_window_child(int i) const
{
    if (i < 0 || i >= sub_nodes.size())
        return nullptr;

    if (sub_nodes[i]->is_window())
        return sub_nodes[i];

    // The lane is correct, so let's get the first window in that lane.
    return sub_nodes[i]->find_nth_window_child(0);
}

void Node::scale_area(double x_scale, double y_scale)
{
    logical_area.size.width = geom::Width{ceil(x_scale * logical_area.size.width.as_int())};
    logical_area.size.height = geom::Height {ceil(y_scale * logical_area.size.height.as_int())};

    for (auto const& node : sub_nodes)
    {
        node->scale_area(x_scale, y_scale);
    }

    refit_node_to_area();
    constrain();
}

void Node::translate_by(int x, int y)
{
    logical_area.top_left.x = geom::X{logical_area.top_left.x.as_int() + x};
    logical_area.top_left.y = geom::Y{logical_area.top_left.y.as_int() + y};
    for (auto const& node : sub_nodes)
    {
        node->translate_by(x, y);
    }

    refit_node_to_area();
    constrain();
}

geom::Rectangle Node::_get_visible_from_logical(
    geom::Rectangle const& logical_area,
    bool has_right_neighbor,
    bool has_bottom_neighbor,
    std::shared_ptr<MiracleConfig> const& config)
{
    int half_gap_x = has_right_neighbor ? (int)(ceil((double) config->get_inner_gaps_x() / 2.0)) : 0;
    int half_gap_y = has_bottom_neighbor ? (int)(ceil((double) config->get_inner_gaps_y() / 2.0)) : 0;
    return {
        geom::Point{
            logical_area.top_left.x.as_int(),
            logical_area.top_left.y.as_int()
        },
        geom::Size{
            logical_area.size.width.as_int() - 2 * half_gap_x,
            logical_area.size.height.as_int() - 2 * half_gap_y
        }
    };
}

std::shared_ptr<Node> Node::find_where(std::function<bool(std::shared_ptr<Node> const&)> func) const
{
    for (auto node : sub_nodes)
        if (func(node))
            return node;

    for (auto const& node : sub_nodes)
        if (auto retval = node->find_where(func))
            return retval;

    return nullptr;
}

bool Node::restore(std::shared_ptr<Node> &node)
{
    for (auto hidden = hidden_nodes.begin(); hidden != hidden_nodes.end(); hidden++)
    {
        if (*hidden == node)
        {
            insert_node(node, sub_nodes.size());
            hidden_nodes.erase(hidden);
            return true;
        }
    }

    return false;
}

bool Node::minimize(std::shared_ptr<Node>& node)
{
    for (auto const& other_node: sub_nodes)
    {
        if (node == other_node)
        {
            hidden_nodes.push_back(node);
            remove_node(node);
            return true;
        }
    }

    return false;
}

int Node::get_min_width() const
{
    return 50;
}

int Node::get_min_height() const
{
    return 50;
}

void Node::_set_window_rectangle(geom::Rectangle const& area)
{
    auto visible_rect = _get_visible_from_logical(
        area,
        _has_right_neighbor(),
        _has_bottom_neighbor(),
        config);
    miral::WindowSpecification spec;
    spec.top_left() = visible_rect.top_left;
    spec.size() = visible_rect.size;
    tools.modify_window(window, spec);

    auto& window_info = tools.info_for(window);
    for (auto const& child : window_info.children())
    {
        miral::WindowSpecification sub_spec;
        sub_spec.top_left() = visible_rect.top_left;
        sub_spec.size() = visible_rect.size;
        tools.modify_window(child, sub_spec);
    }
}

void Node::constrain()
{
    if (is_window())
    {
        auto& info = tools.info_for(window);
        if (window_helpers::is_window_fullscreen(info.state()))
            info.clip_area(mir::optional_value<geom::Rectangle>());
        else
            info.clip_area(get_visible_area());
        return;
    }

    for (auto const& node : sub_nodes)
    {
        node->constrain();
    }
}

bool Node::_has_right_neighbor() const
{
    if (!parent)
        return false;

    if (parent->get_direction() != NodeLayoutDirection::horizontal)
        return parent->_has_right_neighbor();

    auto index = parent->get_index_of_node(this);
    return (parent->num_nodes() > 1 && index != parent->num_nodes() - 1)
        || parent->_has_right_neighbor();
}

bool Node::_has_bottom_neighbor() const
{
    if (!parent)
        return false;

    if (parent->get_direction() != NodeLayoutDirection::vertical)
        return parent->_has_bottom_neighbor();

    auto index = parent->get_index_of_node(this);
    return (parent->num_nodes() > 1 && index != parent->num_nodes() - 1)
           || parent->_has_bottom_neighbor();
}
