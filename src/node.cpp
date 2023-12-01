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

using namespace miracle;

Node::Node() : state{NodeState::lane}
{}

Node::Node(std::shared_ptr<Node> parent, miral::Window &window)
    : parent{parent},
      window{window},
      state{NodeState::window}
{
}

geom::Rectangle Node::get_rectangle()
{
    if (is_window())
    {
        return geom::Rectangle{window.top_left(), window.size()};
    }
    else
    {
        geom::Point top_left{INT_MAX, INT_MAX};
        geom::Point bottom_right{0, 0};

        for (auto item : sub_nodes)
        {
            auto item_rectangle = item->get_rectangle();
            top_left.x = std::min(top_left.x, item_rectangle.top_left.x);
            top_left.y = std::min(top_left.y, item_rectangle.top_left.y);
            bottom_right.x = geom::X{
                std::max(
                    bottom_right.x.as_int(),
                    item_rectangle.top_left.x.as_int() + item_rectangle.size.width.as_int())
            };
            bottom_right.y = geom::Y{
                std::max(
                    bottom_right.y.as_int(),
                    item_rectangle.top_left.y.as_int() + item_rectangle.size.height.as_int())
            };
        }


        return geom::Rectangle(
            top_left,
            geom::Size {
                geom::Width{bottom_right.x.as_int() - top_left.x.as_value()},
                geom::Height{bottom_right.y.as_int() - top_left.y.as_value()}
            });
    }
}

void Node::set_rectangle(geom::Rectangle rect)
{
    if (is_window())
    {
        window.move_to(rect.top_left);
        window.resize(rect.size);
    }
    else
    {
        // TODO: This needs to respect current window sizes and give everyone a fraction
        geom::Size divied_size;
        geom::Point top_left_jump;

        if (direction == NodeDirection::horizontal)
        {
            divied_size = geom::Size{
                geom::Width{rect.size.width.as_value() / static_cast<float>(sub_nodes.size())},
                geom::Height{rect.size.height.as_value()}
            };
            top_left_jump = geom::Point{
                geom::X{divied_size.width.as_value()},
                geom::Y{0}
            };
        }
        else if (direction == NodeDirection::vertical)
        {
            divied_size = geom::Size{
                geom::Width{rect.size.width.as_value()},
                geom::Height{rect.size.height.as_value() / static_cast<float>(sub_nodes.size())}
            };
            top_left_jump = geom::Point{
                geom::X{0},
                geom::Y{divied_size.height.as_value()},
            };
        }

        for (size_t idx = 0; idx < sub_nodes.size(); idx++)
        {
            auto item = sub_nodes[idx];
            auto position = geom::Point{
                geom::X{top_left_jump.x.as_int() * idx + rect.top_left.x.as_int()},
                geom::Y{top_left_jump.y.as_int() * idx + rect.top_left.y.as_int()}
            };
            geom::Rectangle item_rect = {
                position,
                divied_size
            };
            item->set_rectangle(item_rect);
        }
    }
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

    sub_nodes.push_back(std::make_shared<Node>(shared_from_this(), window));
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