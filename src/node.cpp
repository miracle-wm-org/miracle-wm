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
        if (direction == NodeDirection::horizontal)
        {
            for (size_t idx = 0; idx < sub_nodes.size(); idx++)
            {
                auto item = sub_nodes[idx];
                auto item_rect = item->get_rectangle();
                float percent_width_taken = static_cast<float>(item_rect.size.width.as_int()) / area.size.width.as_int();
                int new_width = floor(target_rect.size.width.as_int() * percent_width_taken);

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