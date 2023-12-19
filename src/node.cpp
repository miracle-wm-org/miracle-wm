/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "node.h"
#include <cmath>
#include <iostream>

using namespace miracle;

Node::Node(geom::Rectangle area)
    : state{NodeState::lane},
      area{area}
{}

Node::Node(geom::Rectangle area, std::shared_ptr<Node> parent, miral::Window &window)
    : parent{parent},
      window{window},
      state{NodeState::window},
      area{area}
{
}

geom::Rectangle Node::get_rectangle()
{
    return area;
}

geom::Rectangle Node::new_node_position(int index)
{
    if (is_window())
    {
        // TODO: Error
        return {};
    }

    if (index < 0)
        index = sub_nodes.size();

    if (direction == NodeLayoutDirection::horizontal)
    {
        auto width_per_item = area.size.width.as_int() / static_cast<float>(sub_nodes.size() + 1);
        auto new_size = geom::Size{geom::Width{width_per_item}, area.size.height};
        auto new_position = geom::Point{
            area.top_left.x.as_int() + (index  * width_per_item),
            area.top_left.y
        };
        geom::Rectangle new_node_rect = {new_position, new_size};

        int divvied = ceil(new_node_rect.size.width.as_int() / static_cast<float>(sub_nodes.size()));
        std::shared_ptr<Node> prev_node;
        for (int i = 0; i < sub_nodes.size(); i++)
        {
            auto node = sub_nodes[i];
            auto node_rect = node->get_rectangle();
            node_rect.size.width = geom::Width{node_rect.size.width.as_int() - divvied};

            if (prev_node)
            {
                node_rect.top_left.x = geom::X{
                    prev_node->get_rectangle().top_left.x.as_int() + prev_node->get_rectangle().size.width.as_int()};
            }

            if (i == index)
            {
                node_rect.top_left.x = geom::X{node_rect.top_left.x.as_int() + width_per_item};
            }

            node->set_rectangle(node_rect);
            prev_node = node;
        }

        return new_node_rect;
    }
    else
    {
        auto height_per_item = area.size.height.as_int() / static_cast<float>(sub_nodes.size() + 1);
        auto new_size = geom::Size{area.size.width, height_per_item};
        auto new_position = geom::Point{
            area.top_left.x,
            area.top_left.y.as_int() + (index * height_per_item)
        };

        geom::Rectangle new_node_rect = {new_position, new_size};
        int divvied = ceil(new_node_rect.size.height.as_int() / static_cast<float>(sub_nodes.size()));
        std::shared_ptr<Node> prev_node;
        for (int i = 0; i < sub_nodes.size(); i++)
        {
            auto node = sub_nodes[i];
            auto node_rect = node->get_rectangle();
            node_rect.size.height = geom::Height {node_rect.size.height.as_int() - divvied};

            if (prev_node)
            {
                node_rect.top_left.y = geom::Y{
                    prev_node->get_rectangle().top_left.y.as_int() + prev_node->get_rectangle().size.height.as_int()};
            }

            if (i == index)
                node_rect.top_left.y = geom::Y{node_rect.top_left.y.as_int() + height_per_item};

            node->set_rectangle(node_rect);
            prev_node = node;
        }

        return new_node_rect;
    }
}

void Node::add_node(std::shared_ptr<Node> node)
{
    sub_nodes.push_back(node);
}

void Node::redistribute_size()
{
    if (direction == NodeLayoutDirection::horizontal)
    {
        int total_width = 0;
        for (auto node : sub_nodes)
        {
            total_width += node->get_rectangle().size.width.as_int();
        }

        float diff_width = area.size.width.as_value() - total_width;
        int diff_per_node = diff_width / sub_nodes.size();
        for (auto node : sub_nodes)
        {
            auto rectangle = node->get_rectangle();
            rectangle.size.width = geom::Width{rectangle.size.width.as_int() + diff_per_node};
            rectangle.size.height = geom::Height{area.size.height};
            node->set_rectangle(rectangle);
        }
    }
    else
    {
        int total_height = 0;
        for (auto node : sub_nodes)
        {
            total_height += node->get_rectangle().size.height.as_int();
        }

        float diff_width = area.size.height.as_value() - total_height;
        int diff_per_node = diff_width / sub_nodes.size();
        for (auto node : sub_nodes)
        {
            auto rectangle = node->get_rectangle();
            rectangle.size.width = geom::Width {area.size.width};
            rectangle.size.height = geom::Height {rectangle.size.height.as_int() + diff_per_node};
            node->set_rectangle(rectangle);
        }
    }

    set_rectangle(area);
}

void Node::set_rectangle(geom::Rectangle target_rect)
{
    if (is_window())
    {
        window.move_to(target_rect.top_left);
        window.resize(target_rect.size);
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
                auto item_rect = item->get_rectangle();
                float percent_width_taken = static_cast<float>(item_rect.size.width.as_int()) / area.size.width.as_int();
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
                    auto prev_rect = sub_nodes[idx - 1]->get_rectangle();
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
                auto item_rect = item->get_rectangle();
                float percent_height_taken = static_cast<float>(item_rect.size.height.as_int()) / area.size.height.as_int();
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
                    auto prev_rect = sub_nodes[idx - 1]->get_rectangle();
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
    area = target_rect;
}

void Node::to_lane()
{
    if (is_lane())
        return;

    state = NodeState::lane;
    // If we want to make a new node, but our parent only has one window and its us...
    // then we can just return the parent
    if (parent != nullptr && parent->sub_nodes.size() == 1)
        return;

    sub_nodes.push_back(std::make_shared<Node>(
        geom::Rectangle{window.top_left(), window.size()}, shared_from_this(), window));
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

    // TODO: Error
    return nullptr;
}

bool Node::move_node(int from, int to) {
    if (to < 0)
        return false;

    if (to >= sub_nodes.size())
        return false;

    if (from < to)
    {
        auto moved_node = sub_nodes[from];
        for (size_t i = from; i < to; i++)
        {
            sub_nodes[i] = sub_nodes[i + 1]; // Move the node back 1 place
        }
        sub_nodes[to] = moved_node;
    }
    else
    {
        auto moved_node = sub_nodes[from];
        for (size_t i = from; i > to; i--)
        {
            sub_nodes[i] = sub_nodes[i - 1]; // Move the node back 1 place
        }
        sub_nodes[to] = moved_node;
    }

    redistribute_size();
    return true;
}

void Node::insert_node(std::shared_ptr<Node> node, int index)
{
    auto position = new_node_position(index);
    node->parent = shared_from_this();
    node->set_rectangle(position);
    sub_nodes.insert(sub_nodes.begin() + index, node);
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

std::shared_ptr<Node> Node::find_first_window_child()
{
    if (is_window())
        return shared_from_this();
    
    for (auto node : sub_nodes)
    {
        if (node->is_window())
            return node;
    }

    for (auto node : sub_nodes)
    {
        auto first_child = node->find_first_window_child();
        if (first_child)
            return nullptr;
    }

    std::cerr << "Cannot discover a first child for this lane\n";
    return nullptr;
}