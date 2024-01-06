#include "node.h"
#include <cmath>
#include <iostream>

using namespace miracle;

Node::Node(miral::WindowManagerTools const& tools, geom::Rectangle area, int gap_x, int gap_y)
    : tools{tools},
      state{NodeState::lane},
      logical_area{area},
      gap_x{gap_x},
      gap_y{gap_y}
{}

Node::Node(miral::WindowManagerTools const& tools,geom::Rectangle area, std::shared_ptr<Node> parent, miral::Window &window, int gap_x, int gap_y)
    : tools{tools},
      parent{parent},
      window{window},
      state{NodeState::window},
      logical_area{area},
      gap_x{gap_x},
      gap_y{gap_y}
{
}

geom::Rectangle Node::get_logical_area()
{
    return logical_area;
}

geom::Rectangle Node::get_visible_area()
{
    return _get_visible_from_logical(logical_area, gap_x, gap_y);
}

geom::Rectangle Node::new_node_position(int index)
{
    if (is_window())
    {
        // TODO: Error
        std::cerr << "Cannot create a new node position on a window node\n";
        return {};
    }

    if (index < 0)
        index = sub_nodes.size();

    pending_index = index;

    // TODO: Can we do something better here? The copy-paste is unfortunate
    if (direction == NodeLayoutDirection::horizontal)
    {
        auto new_item_width = logical_area.size.width.as_int() / static_cast<float>(sub_nodes.size() + 1);
        geom::Rectangle new_node_logical_rect = {
            geom::Point{
                logical_area.top_left.x.as_int() + (index * new_item_width),
                logical_area.top_left.y.as_int()
            },
            geom::Size{
                new_item_width,
                logical_area.size.height.as_int()
            }};
        auto new_node_visible_rect = _get_visible_from_logical(
            new_node_logical_rect,
            gap_x,
            gap_y);

        int width_to_lose = ceil(new_node_logical_rect.size.width.as_int() / static_cast<float>(sub_nodes.size()));
        std::shared_ptr<Node> prev_node = nullptr;
        for (int i = 0; i < sub_nodes.size(); i++)
        {
            auto node = sub_nodes[i];
            auto node_logical_area = node->get_logical_area();
            node_logical_area.size.width = geom::Width{node_logical_area.size.width.as_int() - width_to_lose};

            if (prev_node)
            {
                node_logical_area.top_left.x = geom::X{
                    prev_node->get_logical_area().top_left.x.as_int() + prev_node->get_logical_area().size.width.as_int()};
            }

            if (i == index)
            {
                node_logical_area.top_left.x = geom::X{node_logical_area.top_left.x.as_int() + new_item_width};
            }

            node->set_rectangle(node_logical_area);
            prev_node = node;
        }

        return new_node_visible_rect;
    }
    else
    {
        auto new_item_height = logical_area.size.height.as_int() / static_cast<float>(sub_nodes.size() + 1);
        geom::Rectangle new_node_logical_rect = {
            geom::Point{
                logical_area.top_left.x.as_int(),
                logical_area.top_left.y.as_int() + (index * new_item_height)
            },
            geom::Size{
                logical_area.size.width.as_int(),
                new_item_height
            }};

        auto new_node_visible_rect = _get_visible_from_logical(new_node_logical_rect, gap_x, gap_y);
        int height_to_lose = ceil(new_node_logical_rect.size.height.as_int() / static_cast<float>(sub_nodes.size()));
        std::shared_ptr<Node> prev_node = nullptr;
        for (int i = 0; i < sub_nodes.size(); i++)
        {
            auto node = sub_nodes[i];
            auto node_logical_area = node->get_logical_area();
            node_logical_area.size.height = geom::Height {node_logical_area.size.height.as_int() - height_to_lose};

            if (prev_node)
            {
                node_logical_area.top_left.y = geom::Y{
                    prev_node->get_logical_area().top_left.y.as_int() + prev_node->get_logical_area().size.height.as_int()};
            }

            if (i == index)
            {
                node_logical_area.top_left.y = geom::Y{node_logical_area.top_left.y.as_int() + new_item_height};
            }

            node->set_rectangle(node_logical_area);
            prev_node = node;
        }

        return new_node_visible_rect;
    }
}

void Node::add_window(miral::Window& new_window)
{
    if (pending_index < 0)
        pending_index = sub_nodes.size();

    geom::Rectangle new_logical_area = {
        geom::Point{
            new_window.top_left().x.as_int() - gap_x,
            new_window.top_left().y.as_int() - gap_y
        },
        geom::Size{
            new_window.size().width.as_int() + 2 * gap_x,
            new_window.size().height.as_int() + 2 * gap_y
        }
    };

    auto node = std::make_shared<Node>(
        tools,
        new_logical_area,
        shared_from_this(),
        new_window,
        gap_x,
        gap_y);

    sub_nodes.insert(sub_nodes.begin() + pending_index, node);
    pending_index = -1;
}

void Node::redistribute_size()
{
    if (direction == NodeLayoutDirection::horizontal)
    {
        int total_width = 0;
        for (auto node : sub_nodes)
        {
            total_width += node->get_logical_area().size.width.as_int();
        }

        float diff_width = logical_area.size.width.as_value() - total_width;
        int diff_per_node = diff_width / sub_nodes.size();
        for (auto node : sub_nodes)
        {
            auto rectangle = node->get_logical_area();
            rectangle.size.width = geom::Width{rectangle.size.width.as_int() + diff_per_node};
            rectangle.size.height = geom::Height{logical_area.size.height};
            node->set_rectangle(rectangle);
        }
    }
    else
    {
        int total_height = 0;
        for (auto node : sub_nodes)
        {
            total_height += node->get_logical_area().size.height.as_int();
        }

        float diff_width = logical_area.size.height.as_value() - total_height;
        int diff_per_node = diff_width / sub_nodes.size();
        for (auto node : sub_nodes)
        {
            auto rectangle = node->get_logical_area();
            rectangle.size.width = geom::Width {logical_area.size.width};
            rectangle.size.height = geom::Height {rectangle.size.height.as_int() + diff_per_node};
            node->set_rectangle(rectangle);
        }
    }

    set_rectangle(logical_area);
}

void Node::set_rectangle(geom::Rectangle target_rect)
{
    if (is_window())
    {
        auto& info = tools.info_for(window);
        if (info.state() != mir_window_state_fullscreen && info.state() != mir_window_state_maximized)
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
        if (direction == NodeLayoutDirection::horizontal)
        {
            for (size_t idx = 0; idx < sub_nodes.size(); idx++)
            {
                auto item = sub_nodes[idx];
                auto item_rect = item->get_logical_area();
                float percent_width_taken = static_cast<float>(item_rect.size.width.as_int()) / logical_area.size.width.as_int();
                int new_width = ceil(target_rect.size.width.as_int() * percent_width_taken);

                geom::Rectangle new_item_rect;
                new_item_rect.size = geom::Size{
                    geom::Width{new_width},
                    target_rect.size.height
                };
                if (idx == 0)
                {
                    new_item_rect.top_left = geom::Point{
                        target_rect.top_left.x,
                        target_rect.top_left.y
                    };
                }
                else
                {
                    auto prev_rect = sub_nodes[idx - 1]->get_logical_area();
                    new_item_rect.top_left = geom::Point{
                        geom::X{prev_rect.top_left.x.as_int() + prev_rect.size.width.as_int()},
                        target_rect.top_left.y
                    };
                }

                item->set_rectangle(new_item_rect);
            }
        }
        else
        {
            for (size_t idx = 0; idx < sub_nodes.size(); idx++)
            {
                auto item = sub_nodes[idx];
                auto item_rect = item->get_logical_area();
                float percent_height_taken = static_cast<float>(item_rect.size.height.as_int()) / logical_area.size.height.as_int();
                int new_height = floor(target_rect.size.height.as_int() * percent_height_taken);

                geom::Rectangle new_item_rect;
                new_item_rect.size = geom::Size{
                    target_rect.size.width,
                    geom::Height{new_height},
                };
                if (idx == 0)
                {
                    new_item_rect.top_left = geom::Point{
                        target_rect.top_left.x,
                        target_rect.top_left.y
                    };
                }
                else
                {
                    auto prev_rect = sub_nodes[idx - 1]->get_logical_area();
                    new_item_rect.top_left = geom::Point{
                        target_rect.top_left.x,
                        geom::Y{prev_rect.top_left.y.as_int() + prev_rect.size.height.as_int()},
                    };
                }

                item->set_rectangle(new_item_rect);
            }
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

    auto seed_node = std::make_shared<Node>(
        tools,
        logical_area,
        shared_from_this(),
        window,
        gap_x,
        gap_y);
    sub_nodes.push_back(seed_node);
    return seed_node;
}

std::shared_ptr<miracle::Node> Node::find_node_for_window(miral::Window &window)
{
    for (auto item : sub_nodes)
    {
        if (item->is_window())
        {
            if (item->get_window() == window)
                return item;
        }
        else
        {
            auto node = item->find_node_for_window(window);
            if (node != nullptr)
                return node;
        }
    }

    for (auto hidden : hidden_nodes)
    {
        if (hidden->is_window())
        {
            if (hidden->get_window() == window)
                return hidden;
        }
        else
        {
            auto node = hidden->find_node_for_window(window);
            if (node != nullptr)
                return node;
        }
    }

    // TODO: Error
    return nullptr;
}

void Node::insert_node(std::shared_ptr<Node> node, int index)
{
    auto area_with_gaps = new_node_position(index);
    node->parent = shared_from_this();
    node->set_rectangle(_get_logical_from_visible(area_with_gaps, gap_x, gap_y));
    sub_nodes.insert(sub_nodes.begin() + index, node);
}

void Node::remove_node(std::shared_ptr<Node> const& node)
{
    if (is_window())
    {
        std::cerr << "Cannot remove a node from a window\n";
        return;
    }

    sub_nodes.erase(
        std::remove_if(sub_nodes.begin(), sub_nodes.end(), [&](std::shared_ptr<Node> content) {
            return content == node;
        }),
        sub_nodes.end()
    );

    // If we have one child AND it is a lane, THEN we can absorb all of it's children
    if (sub_nodes.size() == 1 && sub_nodes[0]->is_lane())
    {
        auto dying_lane = sub_nodes[0];
        sub_nodes.clear();
        for (auto sub_node : dying_lane->get_sub_nodes())
        {
            add_window(sub_node->get_window());
        }
        set_direction(dying_lane->get_direction());
    }

    redistribute_size();
}

int Node::get_index_of_node(std::shared_ptr<Node> node)
{
    for (int i = 0; i < sub_nodes.size(); i++)
        if (sub_nodes[i] == node)
            return i;

    return -1;
}

int Node::num_nodes()
{
    return sub_nodes.size();
}

std::shared_ptr<Node> Node::node_at(int i)
{
    if (i < 0 || i >= num_nodes())
        return nullptr;

    return sub_nodes[i];
}

std::shared_ptr<Node> Node::find_nth_window_child(int i)
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

    for (auto node : sub_nodes)
    {
        node->scale_area(x_scale, y_scale);
    }

    redistribute_size();
}

void Node::translate_by(int x, int y)
{
    logical_area.top_left.x = geom::X{logical_area.top_left.x.as_int() + x};
    logical_area.top_left.y = geom::Y{logical_area.top_left.y.as_int() + y};
    for (auto node : sub_nodes)
    {
        node->translate_by(x, y);
    }
    redistribute_size();
}

geom::Rectangle Node::_get_visible_from_logical(geom::Rectangle const& logical_area, int gap_x, int gap_y)
{
    return {
        geom::Point{
            logical_area.top_left.x.as_int() + gap_x,
            logical_area.top_left.y.as_int() + gap_y
        },
        geom::Size{
            logical_area.size.width.as_int() - 2 * gap_x,
            logical_area.size.height.as_int() - 2 * gap_y
        }
    };
}

geom::Rectangle Node::_get_logical_from_visible(const geom::Rectangle &visible_area, int gap_x, int gap_y)
{
    return {
        geom::Point{
            visible_area.top_left.x.as_int() - gap_x,
            visible_area.top_left.y.as_int() - gap_y
        },
        geom::Size{
            visible_area.size.width.as_int() + 2 * gap_x,
            visible_area.size.height.as_int() + 2 * gap_y
        }
    };
}

std::shared_ptr<Node> Node::find_where(std::function<bool(std::shared_ptr<Node>)> func)
{
    for (auto node : sub_nodes)
        if (func(node))
            return node;

    for (auto node : sub_nodes)
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
    for (auto other_node: sub_nodes)
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

int Node::get_min_width()
{
    if (is_window())
    {
        miral::WindowInfo& info = tools.info_for(window);
        return info.min_width().as_int();
    }

    int min_width = 50;
    for (auto node : sub_nodes)
    {
        min_width = std::max(node->get_min_width(), min_width);
    }

    return min_width;
}

int Node::get_min_height()
{
    if (is_window())
    {
        miral::WindowInfo& info = tools.info_for(window);
        return info.min_height().as_int();
    }

    int min_height = 50;
    for (auto node : sub_nodes)
    {
        min_height = std::max(node->get_min_height(), min_height);
    }

    return min_height;
}

void Node::_set_window_rectangle(geom::Rectangle area)
{
    auto visible_rect = _get_visible_from_logical(area, gap_x, gap_y);
    window.move_to(visible_rect.top_left);
    window.resize(visible_rect.size);
    auto& window_info = tools.info_for(window);
    for (auto child : window_info.children())
    {
        child.move_to(visible_rect.top_left);
        child.resize(visible_rect.size);
    }
}