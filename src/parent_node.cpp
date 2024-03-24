#include "leaf_node.h"
#include "parent_node.h"
#include "miracle_config.h"
#include "node.h"
#include <cmath>

using namespace miracle;

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
    size_t node_count,
    std::function<int(int)> const& get_node_size,
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

ParentNode::ParentNode(
    std::shared_ptr<NodeInterface> const& node_interface,
    geom::Rectangle area,
    std::shared_ptr<MiracleConfig> const& config,
    Tree* tree,
    Node* parent)
    : node_interface{node_interface},
      logical_area{std::move(area)},
      tree{tree},
      config{config},
      parent{parent}
{
}

geom::Rectangle ParentNode::get_logical_area() const
{
    if (parent == nullptr)
    {
        auto x = config->get_outer_gaps_x();
        auto y = config->get_outer_gaps_y();

        auto modified_logical_area = geom::Rectangle(
            geom::Point(logical_area.top_left.x.as_int() + x, logical_area.top_left.y.as_int() + y),
            geom::Size(logical_area.size.width.as_int() - 2 * x, logical_area.size.height.as_int() - 2 * y)
        );

        return modified_logical_area;
    }

    return logical_area;
}

size_t ParentNode::num_nodes() const
{
    return sub_nodes.size();
}

void ParentNode::create_space_for_window(int pending_index)
{
    auto placement_area = get_logical_area();
    geom::Rectangle pending_logical_rect;
    if (direction == NodeLayoutDirection::horizontal)
    {
        auto result = insert_node_internal(
            placement_area.size.width.as_int(),
            placement_area.top_left.x.as_int(),
            pending_index,
            sub_nodes.size(),
            [&](int index) { return sub_nodes[index]->get_logical_area().size.width.as_int();},
            [&](int index, int size, int pos) {
                sub_nodes[index]->set_logical_area({
                    geom::Point{
                        pos,
                        placement_area.top_left.y.as_int()
                    },
                    geom::Size{
                        size,
                        placement_area.size.height.as_int()
                    }});
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
    }
    else
    {
        auto result = insert_node_internal(
            placement_area.size.height.as_int(),
            placement_area.top_left.y.as_int(),
            pending_index,
            sub_nodes.size(),
            [&](int index) { return sub_nodes[index]->get_logical_area().size.height.as_int();},
            [&](int index, int size, int pos) {
                sub_nodes[index]->set_logical_area({
                    geom::Point{
                        placement_area.top_left.x.as_int(),
                        pos
                    },
                    geom::Size{
                        placement_area.size.width.as_int(),
                        size
                    }});
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
    }

    pending_node = Node::from_leaf(std::make_unique<LeafNode>(
        node_interface,
        pending_logical_rect,
        config,
        tree,
        parent));
    sub_nodes.insert(sub_nodes.begin() + pending_index, pending_node);
}

void ParentNode::confirm_window(miral::Window const& window)
{
    if (pending_node == nullptr)
    {
        mir::fatal_error("Unable to add the window to the scene. Was create_space_for_window called?");
        return;
    }

    pending_node->as_leaf().lock()->associate_to_window(window);
    for (auto const& node : sub_nodes)
        node->commit_changes();
    pending_node = nullptr;
}

void ParentNode::set_logical_area(const geom::Rectangle &target_rect)
{
    // We are setting the size of the lane, but each window might have an idea of how
    // its own height relates to the lane (e.g. I take up 300px of 900px lane while my
    // neighbor takes up the remaining 600px, horizontally).
    // We need to look at the target dimension and scale everyone relative to that.
    // However, the "non-main-axis" dimension will be consistent across each node.
    auto current_logical_area = get_logical_area();
    logical_area = target_rect;
    auto target_placement_area = get_logical_area();
    std::vector<geom::Rectangle> pending_size_updates;
    if (direction == NodeLayoutDirection::horizontal)
    {
        int total_width = 0;
        for (size_t idx = 0; idx < sub_nodes.size(); idx++)
        {
            auto item = sub_nodes[idx];
            auto item_rect = item->get_logical_area();
            double percent_width_taken = (double)item_rect.size.width.as_int() / (double)current_logical_area.size.width.as_int();
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
            double percent_height_taken = static_cast<double>(item_rect.size.height.as_int()) / current_logical_area.size.height.as_int();
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

void ParentNode::commit_changes()
{
    for (auto& node : sub_nodes)
        node->commit_changes();
}

void ParentNode::set_direction(miracle::NodeLayoutDirection new_direction)
{
    direction = new_direction;
}

void ParentNode::swap_nodes(std::shared_ptr<Node> const& first, std::shared_ptr<Node> const& second)
{
    auto first_index = get_index_of_node(first);
    auto second_index = get_index_of_node(second);
    sub_nodes[second_index] = first;
    sub_nodes[first_index] = second;
    set_logical_area(logical_area);
    constrain();
}

void ParentNode::remove(const std::shared_ptr<Node> &node)
{
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
}

int ParentNode::get_index_of_node(std::shared_ptr<Node> const& node) const
{
    for (int i = 0; i < sub_nodes.size(); i++)
        if (sub_nodes[i] == node)
            return i;

    return -1;
}

void ParentNode::constrain()
{
    for (auto& node : sub_nodes)
        node->constrain();
}

void ParentNode::relayout()
{
    auto placement_area = get_logical_area();
    if (direction == NodeLayoutDirection::horizontal)
    {
        int total_width = 0;
        for (auto const& node : sub_nodes)
        {
            total_width += node->get_logical_area().size.width.as_int();
        }

        int diff_width = placement_area.size.width.as_value() - total_width;
        int diff_per_node = floor((double)diff_width / (double)sub_nodes.size());
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

        int diff_width = placement_area.size.height.as_value() - total_height;
        int diff_per_node = floor((double)diff_width / (double)sub_nodes.size());
        for (auto const& node : sub_nodes)
        {
            auto rectangle = node->get_logical_area();
            rectangle.size.width = geom::Width {placement_area.size.width};
            rectangle.size.height = geom::Height {rectangle.size.height.as_int() + diff_per_node};
            node->set_logical_area(rectangle);
        }
    }

    set_logical_area(placement_area);
}