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

#define MIR_LOG_COMPONENT "workspace_content"
#define GLM_ENABLE_EXPERIMENTAL

#include "workspace.h"
#include "compositor_state.h"
#include "config.h"
#include "leaf_container.h"
#include "math_helpers.h"
#include "output_interface.h"
#include "output_manager.h"
#include "parent_container.h"
#include "shell_component_container.h"
#include "workspace_observer.h"

#include <cassert>
#include <glm/gtx/transform.hpp>
#include <mir/log.h>
#include <mir/scene/surface.h>
#include <miral/zone.h>

using namespace miracle;

namespace
{
std::shared_ptr<ParentContainer> handle_remove_container(std::shared_ptr<Container> const& container)
{
    auto parent = Container::as_parent(container->get_parent().lock());
    if (parent == nullptr)
        return nullptr;

    if (parent->num_nodes() == 1 && parent->get_parent().lock())
    {
        // Remove the entire parent if this parent is now empty
        auto prev_active = parent;
        parent = Container::as_parent(parent->get_parent().lock());
        parent->remove(prev_active);
    }
    else
    {
        parent->remove(container);
    }

    return parent;
}

LayoutScheme from_direction(Direction direction)
{
    switch (direction)
    {
    case Direction::up:
    case Direction::down:
        return LayoutScheme::vertical;
    case Direction::right:
    case Direction::left:
        return LayoutScheme::horizontal;
    default:
        mir::log_error(
            "from_direction: somehow we are trying to create a LayoutScheme from an incorrect Direction");
        return LayoutScheme::horizontal;
    }
}

std::shared_ptr<Container> foreach_node_internal(
    std::function<bool(std::shared_ptr<Container> const&)> const& f,
    std::shared_ptr<Container> const& parent)
{
    if (f(parent))
        return parent;

    if (parent->is_leaf())
        return nullptr;

    for (auto& node : Container::as_parent(parent)->get_sub_nodes())
    {
        if (auto result = foreach_node_internal(f, node))
            return result;
    }

    return nullptr;
}

geom::Rectangle get_output_area(std::shared_ptr<OutputInterface> const& output)
{
    auto const& zones = output->get_app_zones();
    if (!zones.empty())
        return zones[0].extents();

    return output->get_area();
}

class WorkspaceAnimation : public MultiBuiltInAnimation
{
public:
    WorkspaceAnimation(
        AnimationHandle handle,
        AnimationDefinition const& definition,
        mir::geometry::Rectangle const& from,
        mir::geometry::Rectangle const& to,
        mir::geometry::Rectangle const& current,
        float opacity_start,
        float opacity_end,
        std::shared_ptr<Workspace> const& workspace,
        std::shared_ptr<CompositorState> const& state,
        bool is_hiding) :
        MultiBuiltInAnimation(handle, definition, from, to, current, opacity_start, opacity_end),
        workspace(workspace),
        state(state),
        is_hiding(is_hiding)
    {
    }

    AnimationFrameResult init() override
    {
        auto const lock = state->lock();
        auto const locked = workspace.lock();
        if (!locked)
            return {};

        locked->on_animation_start(is_hiding);
        return MultiBuiltInAnimation::init();
    }

    void on_tick(AnimationFrameResult const& asr) override
    {
        auto const lock = state->lock();
        auto const locked = workspace.lock();
        if (!locked)
            return;

        glm::mat4 matrix(1.f);
        if (asr.transform)
            matrix = matrix * asr.transform.value();
        if (asr.rectangle)
        {
            matrix = glm::translate(
                matrix,
                glm::vec3(
                    asr.rectangle->top_left.x.as_value(),
                    asr.rectangle->top_left.y.as_value(),
                    0));
        }

        float const alpha = asr.opacity ? *asr.opacity : 1.f;
        locked->transform(matrix);
        locked->alpha(alpha);
        if (asr.is_complete)
            locked->on_animation_end(is_hiding);
    }

private:
    std::weak_ptr<Workspace> workspace;
    std::shared_ptr<CompositorState> state;
    bool is_hiding;
};
}

Workspace::Workspace(
    std::shared_ptr<OutputInterface> const& output,
    uint32_t id,
    std::optional<int> num,
    std::optional<std::string> name,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<CompositorState> const& state,
    std::shared_ptr<WorkspaceObserverRegistrar> const& registry,
    std::shared_ptr<Animator> const& animator) :
    output { output },
    id_ { id },
    num_ { num },
    name_ { name },
    window_controller { window_controller },
    state { state },
    registry { registry },
    config { config },
    animator { animator },
    animation_handle { animator->register_animateable() }
{
}

std::shared_ptr<ParentContainer> Workspace::root() const
{
    if (!root_)
    {
        auto mutable_ws = std::const_pointer_cast<Workspace>(shared_from_this());
        root_ = std::make_shared<ParentContainer>(
            state, window_controller, config, get_output_area(output.lock()), mutable_ws, nullptr, true);
    }

    return root_;
}

void Workspace::set_area(mir::geometry::Rectangle const& area)
{
    // TODO: This is wort of weird.
    root()->set_workspace(shared_from_this());
    root()->set_logical_area(area);
    root()->commit_changes();
}

void Workspace::recalculate_area()
{
    if (auto const sh_output = output.lock())
    {
        root()->set_logical_area(get_output_area(sh_output));
        root()->commit_changes();
    }
}

AllocationHint Workspace::allocate_position(
    miral::ApplicationInfo const& app_info,
    miral::WindowSpecification& requested_specification,
    AllocationHint const& hint)
{
    // We will figure out which parent to add this window to before placing it.
    //
    // The parent is either:
    // 1. The one provided in the hint
    // 2. The same parent as the currently selected (or root)
    // 3. A wholly new parent, in the event that we are initializing a floating window.
    auto const& workspace_config = config->get_workspace_config(num_, name_);
    std::shared_ptr<ParentContainer> parent;
    if (hint.parent)
        parent = hint.parent.value();
    else
    {
        if (workspace_config.window_layout_strategy)
        {
            switch (*workspace_config.window_layout_strategy)
            {

            case WindowLayoutStrategy::floating:
            {
                auto const output_area = get_output()->get_area();
                geom::Rectangle const floating_area = {
                    geom::Point {
                                 as_float(output_area.top_left.x) + as_float(output_area.size.width) * 0.1f,
                                 as_float(output_area.top_left.y) + as_float(output_area.size.height) * 0.1f },
                    geom::Size {
                                 as_float(output_area.size.width) * 0.8f,
                                 as_float(output_area.size.height) * 0.8f                                    }
                };
                parent = create_floating_tree(floating_area);
                break;
            }
            default:
                parent = get_layout_container();
                break;
            }
        }
        else
            parent = get_layout_container();
    }

    requested_specification = parent->place_new_window(requested_specification);
    return { ContainerType::leaf, parent };
}

std::shared_ptr<Container> Workspace::create_container(
    miral::WindowInfo const& window_info,
    AllocationHint const& hint)
{
    std::shared_ptr<Container> container = nullptr;
    miral::WindowSpecification spec;
    switch (hint.container_type)
    {
    case ContainerType::leaf:
    {
        assert(hint.parent.has_value());
        container = hint.parent.value()->confirm_window(window_info.window());
        break;
    }
    case ContainerType::shell:
        container = std::make_shared<ShellComponentContainer>(window_info.window(), window_controller);
        break;
    default:
        mir::log_error("Unsupported window type: %d", (int)hint.container_type);
        break;
    }

    spec.userdata() = container;
    spec.min_width() = mir::geometry::Width(0);
    spec.min_height() = mir::geometry::Height(0);
    window_controller->modify(window_info.window(), spec);
    return container;
}

void Workspace::delete_container(std::shared_ptr<Container> const& container)
{
    switch (container->get_type())
    {
    case ContainerType::leaf:
    {
        auto const parent = handle_remove_container(container);
        parent->commit_changes();

        // If we're deleting a container and it is the final container in a
        // floating tree, then we need to remove the tree entirely.
        if (parent->num_nodes() == 0 && parent != root())
        {
            floating_trees.erase(
                std::remove(floating_trees.begin(), floating_trees.end(), parent),
                floating_trees.end());
        }
        break;
    }
    default:
        mir::log_error("Unsupported window type: %d", (int)container->get_type());
        return;
    }

    if (is_empty())
        registry->advise_empty(id());
}

void Workspace::advise_focus_gained(std::shared_ptr<Container> const& container)
{
    if (!is_showing)
        last_selected_container = container;
}

void Workspace::show(geom::Point const& origin)
{
    if (!config->are_animations_enabled() || origin == geom::Point(0, 0))
    {
        on_animation_start(false);
        on_animation_end(false);
        return;
    }

    auto const area = root()->get_logical_area();
    auto const animation = std::make_shared<WorkspaceAnimation>(
        animation_handle,
        config->get_animation_definition(AnimateableEvent::workspace_switch),
        geom::Rectangle(origin, area.size),
        geom::Rectangle(geom::Point(0, 0), area.size),
        geom::Rectangle(origin, area.size),
        0,
        1,
        shared_from_this(),
        state,
        false);

    animator->append(animation);
}

void Workspace::hide(geom::Point const& end)
{
    if (!config->are_animations_enabled())
    {
        on_animation_end(true);
        return;
    }

    auto const area = root()->get_logical_area();
    auto const animation = std::make_shared<WorkspaceAnimation>(
        animation_handle,
        config->get_animation_definition(AnimateableEvent::workspace_switch),
        geom::Rectangle(geom::Point(0, 0), area.size),
        geom::Rectangle(end, area.size),
        geom::Rectangle(geom::Point(0, 0), area.size),
        1,
        0,
        shared_from_this(),
        state,
        true);

    animator->append(animation);
}

bool Workspace::for_each_window(std::function<bool(std::shared_ptr<Container>)> const& f) const
{
    auto _for_each_window = [&](std::shared_ptr<Container> const& node)
    {
        if (auto leaf = Container::as_leaf(node))
        {
            if (!leaf->window())
            {
                mir::log_error("MiralWorkspace::for_each_window: tiled window has no window");
                return false;
            }

            auto container = window_controller->get_container(leaf->window().value());
            if (container && f(container))
                return true;
        }

        return false;
    };

    for (auto const& other_root : floating_trees)
    {
        if (foreach_node_internal(_for_each_window, other_root))
            return true;
    }

    if (foreach_node_internal(_for_each_window, root()))
        return true;

    return false;
}

void Workspace::transfer_pinned_windows_to(std::shared_ptr<WorkspaceInterface> const& other)
{
    for (auto it = floating_trees.begin(); it != floating_trees.end();)
    {
        if (it->get()->pinned())
        {
            other->graft(*it);
            it = floating_trees.erase(it);
        }
        else
            it++;
    }
}

std::shared_ptr<ParentContainer> Workspace::create_floating_tree(mir::geometry::Rectangle const& area)
{
    auto floating = std::make_shared<ParentContainer>(
        state, window_controller, config, area, shared_from_this(), nullptr, false);
    floating_trees.push_back(floating);
    return floating;
}

bool Workspace::move_container(miracle::Direction direction, Container& container)
{
    auto traversal_result = handle_move(container, direction);
    switch (traversal_result.traversal_type)
    {
    case MoveResult::traversal_type_insert:
    {
        container.move_to(*traversal_result.node);
        break;
    }
    case MoveResult::traversal_type_append:
    {
        auto lane_node = Container::as_parent(traversal_result.node);
        auto moving_node = container.shared_from_this();
        handle_remove_container(moving_node);
        lane_node->graft_existing(moving_node, static_cast<int>(lane_node->num_nodes()));
        lane_node->commit_changes();
        break;
    }
    case MoveResult::traversal_type_prepend:
    {
        auto lane_node = Container::as_parent(traversal_result.node);
        auto moving_node = container.shared_from_this();
        handle_remove_container(moving_node);
        lane_node->graft_existing(moving_node, 0);
        lane_node->commit_changes();
        break;
    }
    default:
    {
        mir::log_error("Unable to move window");
        return false;
    }
    }

    return true;
}

bool Workspace::add_to_root(Container& to_move)
{
    root()->graft_existing(to_move.shared_from_this(), static_cast<int>(root()->num_nodes()));
    to_move.set_workspace(shared_from_this());
    return true;
}

Workspace::MoveResult Workspace::handle_move(Container& from, Direction direction)
{
    // Algorithm:
    //  1. Perform the _select algorithm. If that passes, then we want to be where the selected node
    //     currently is
    //  2. If our parent layout direction does not equal the root layout direction, we can append
    //     or prepend to the root
    if (auto insert_node = LeafContainer::handle_select(from, direction))
    {
        return {
            MoveResult::traversal_type_insert,
            insert_node
        };
    }

    auto parent = from.get_parent().lock();
    if (root() == parent)
    {
        auto new_layout_direction = from_direction(direction);
        if (new_layout_direction == root()->get_direction())
            return {};

        auto after_root_lane = std::make_shared<ParentContainer>(
            state,
            window_controller,
            config,
            root()->get_logical_area(),
            shared_from_this(),
            nullptr,
            true);
        after_root_lane->set_layout(new_layout_direction);
        after_root_lane->graft_existing(root(), 0);
        root_ = after_root_lane;
        recalculate_area();
    }

    if (is_negative_direction(direction))
        return {
            MoveResult::traversal_type_prepend,
            root_
        };
    else
        return {
            MoveResult::traversal_type_append,
            root_
        };
}

std::shared_ptr<OutputInterface> Workspace::get_output() const
{
    return output.lock();
}

void Workspace::set_output(std::shared_ptr<OutputInterface> const& new_output)
{
    this->output = new_output;
    set_area(new_output->get_area());
}

bool Workspace::is_empty() const
{
    return root()->num_nodes() == 0 && floating_trees.empty();
}

void Workspace::graft(std::shared_ptr<Container> const& container)
{
    switch (container->get_type())
    {
    case ContainerType::parent:
    {
        // When we move a parent to a new workspace, we add it as a floating tree.
        auto parent = Container::as_parent(container);
        if (!parent)
        {
            mir::log_error("MiralWorkspace::graft: grafting non-parent container");
            return;
        }

        parent->set_anchored(false);
        parent->set_workspace(shared_from_this());
        floating_trees.push_back(parent);
        break;
    }
    case ContainerType::leaf:
        root()->graft_existing(container, static_cast<int>(root()->num_nodes()));
        root()->commit_changes();
        break;
    default:
        mir::log_error("Workspace::graft: ungraftable container type: %d", static_cast<int>(container->get_type()));
        break;
    }

    container->set_workspace(shared_from_this());
}

void Workspace::num(std::optional<int> n)
{
    num_ = n;
}

void Workspace::name(std::optional<std::string> const& name)
{
    name_ = name;
}

std::optional<Gaps> Workspace::outer_gaps() const
{
    return workspace_outer_gaps;
}

void Workspace::outer_gaps(std::optional<Gaps> const& gaps)
{
    workspace_outer_gaps = gaps;
    recalculate_area();
}

std::optional<Gaps> Workspace::inner_gaps() const
{
    return workspace_inner_gaps;
}

void Workspace::inner_gaps(std::optional<Gaps> const& gaps)
{
    workspace_inner_gaps = gaps;
    recalculate_area();
}

void Workspace::transform(glm::mat4 const& transform)
{
    transform_ = transform;
    for (auto const& container : state->containers())
    {
        if (auto const locked = container.lock())
        {
            if (locked->get_workspace().get() == this)
                locked->set_workspace_transform(transform);
        }
    }
}

glm::mat4 Workspace::transform() const
{
    return transform_;
}

void Workspace::alpha(float a)
{
    alpha_ = a;
    for (auto const& container : state->containers())
    {
        if (auto const locked = container.lock())
        {
            if (locked->get_workspace().get() == this)
                locked->set_workspace_alpha(a);
        }
    }
}

float Workspace::alpha() const
{
    return alpha_;
}

void Workspace::on_animation_start(bool is_hiding)
{
    // HACK: miral will try to select a newly visible window if none is currently
    // selected. In most instances, we do not want this, as we would rather
    // select our [last_selected_container] instead. To work around this, we set
    // a flag that tells miral not to select the last focused container while we
    // are in the process of becoming visible.
    is_showing = true;
    root()->show();
    for (auto const& floating : floating_trees)
        floating->show();
    is_showing = false;

    if (!is_hiding)
    {
        if (auto const sh_last_selected = last_selected_container.lock())
        {
            if (sh_last_selected->window().has_value())
                window_controller->select_active_window(sh_last_selected->window().value());
            return;
        }

        for_each_window([&](std::shared_ptr<Container> const& container)
        {
            if (container->window().has_value())
            {
                window_controller->select_active_window(container->window().value());
                return true;
            }

            return false;
        });
    }
}

void Workspace::on_animation_end(bool is_hiding)
{
    if (is_hiding)
    {
        root()->hide();
        for (auto const& floating : floating_trees)
            floating->hide();
    }
}

std::shared_ptr<ParentContainer> Workspace::get_layout_container()
{
    if (!state->focused_container())
        return root();

    auto parent = state->focused_container()->get_parent().lock();
    if (!parent)
        return root();

    if (parent->get_workspace().get() != this)
        return root();

    return parent;
}

std::shared_ptr<ParentContainer> Workspace::get_root() const
{
    return root();
}

std::string Workspace::display_name() const
{
    std::stringstream ss;
    if (num_ && name_)
        ss << num_.value() << ":" << name_.value();
    else if (name_)
        return name_.value();
    else if (num_)
        return std::to_string(num_.value());
    else
        ss << "Unknown #" << id_;

    return ss.str();
}

nlohmann::json Workspace::get_workspaces_json(bool is_output_focused) const
{
    auto const sh_output = output.lock();
    bool const is_active_on_output = sh_output != nullptr && sh_output->active().get() == this;

    // Note: The reported workspace area appears to be the placement
    // area of the root tree.
    //   See: https://i3wm.org/docs/ipc.html#_tree_reply
    auto const area = root()->get_logical_area();
    auto const workspace_name = display_name();
    auto const output_name = sh_output ? sh_output->name() : "N/A";

    return {
        {
         "num",
         num_ ? num_.value() : -1,
         },
        { "name", workspace_name },
        { "visible", is_active_on_output },
        { "focused", is_output_focused && is_active_on_output },
        { "urgent", false },
        { "output", output_name },
        { "rect", {
                      { "x", area.top_left.x.as_int() },
                      { "y", area.top_left.y.as_int() },
                      { "width", area.size.width.as_int() },
                      { "height", area.size.height.as_int() },
                  } }
    };
}

nlohmann::json Workspace::to_json(bool is_output_focused) const
{
    auto const sh_output = output.lock();
    bool const is_active_on_output = sh_output != nullptr && sh_output->active().get() == this;

    // Note: The reported workspace area appears to be the placement
    // area of the root tree.
    //   See: https://i3wm.org/docs/ipc.html#_tree_reply
    auto area = root()->get_logical_area();

    nlohmann::json floating_nodes = nlohmann::json::array();
    for (auto const& container : floating_trees)
        floating_nodes.push_back(container->to_json(is_active_on_output));

    nlohmann::json nodes = nlohmann::json::array();
    for (auto const& container : root()->get_sub_nodes())
        nodes.push_back(container->to_json(is_active_on_output));

    return {
        {
         "num",
         num_ ? num_.value() : -1,
         },
        { "id", reinterpret_cast<std::uintptr_t>(this) },
        { "type", "workspace" },
        { "name", display_name() },
        { "visible", is_active_on_output },
        { "focused", is_output_focused && is_active_on_output },
        { "urgent", false },
        { "output", sh_output ? sh_output->name() : "N/A" },
        { "border", "none" },
        { "current_border_width", 0 },
        { "layout", to_string(root()->get_scheme()) },
        { "orientation", "none" },
        { "window_rect", {
                             { "x", 0 },
                             { "y", 0 },
                             { "width", 0 },
                             { "height", 0 },
                         } },
        { "deco_rect", {
                           { "x", 0 },
                           { "y", 0 },
                           { "width", 0 },
                           { "height", 0 },
                       } },
        { "geometry", {
                          { "x", 0 },
                          { "y", 0 },
                          { "width", 0 },
                          { "height", 0 },
                      } },
        { "window", nullptr },
        { "floating_nodes", floating_nodes },
        { "rect", {
                                                                                   { "x", area.top_left.x.as_int() },
                                                                                   { "y", area.top_left.y.as_int() },
                                                                                   { "width", area.size.width.as_int() },
                                                                                   { "height", area.size.height.as_int() },
                                                                               } },
        { "nodes", nodes }
    };
}
