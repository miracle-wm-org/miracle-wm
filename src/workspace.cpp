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

#define MIR_LOG_COMPONENT "workspace_content"
#define GLM_ENABLE_EXPERIMENTAL

#include "workspace.h"
#include "abstract_output.h"
#include "compositor_state.h"
#include "config.h"
#include "freestyle_window_container.h"
#include "leaf_container.h"
#include "math_helpers.h"
#include "parent_container.h"
#include "plugin_bridge.h"
#include "shell_component_container.h"
#include "window_container.h"
#include "workspace_observer.h"

#include <cassert>
#include <glm/gtx/transform.hpp>
#include <mir/log.h>
#include <mir/scene/surface.h>
#include <miral/zone.h>

#include "shell_application_manager.h"

using namespace miracle;

namespace
{
std::shared_ptr<ParentContainer> handle_remove_container(std::shared_ptr<Container> const& container)
{
    auto parent = Container::as_parent(container->get_parent().lock());
    if (parent == nullptr)
        return nullptr;

    if (parent->num_children() == 1 && parent->get_parent().lock())
    {
        // Remove the entire parent if this parent is now empty
        auto prev_active = parent;
        parent = Container::as_parent(parent->get_parent().lock());
        parent->remove_child(prev_active);
    }
    else
    {
        parent->remove_child(container);
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

    auto const as_parent = Container::as_parent(parent);
    if (!as_parent)
        return nullptr;

    for (auto& node : as_parent->children())
    {
        if (auto result = foreach_node_internal(f, node))
            return result;
    }

    return nullptr;
}

geom::Rectangle get_output_area(std::shared_ptr<AbstractOutput> const& output)
{
    auto const& zones = output->get_app_zones();
    if (!zones.empty())
        return zones[0].extents();

    return output->get_area();
}
}

Workspace::Workspace(
    std::shared_ptr<ShellApplicationManager> const& shell_application_manager,
    std::shared_ptr<AbstractOutput> const& output,
    uint32_t id,
    std::optional<int> num,
    std::optional<std::string> name,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<WindowController> const& window_controller,
    std::shared_ptr<CompositorState> const& state,
    std::shared_ptr<WorkspaceObserverRegistrar> const& registry,
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<PluginManager> const& plugin_manager) :
    shell_application_manager {
        shell_application_manager
},
    output { output },
    id_ { id },
    window_controller { window_controller },
    state { state },
    registry { registry },
    config { config },
    animator { animator },
    plugin_manager { plugin_manager },
    animation_handle { animator->register_animateable() },
    sync { State {
        num,
        std::move(name),
    } }
{
}

Workspace::~Workspace()
{
    animator->remove_by_animation_handle(animation_handle);
}

std::shared_ptr<ParentContainer> Workspace::root() const
{
    if (!root_)
    {
        auto mutable_ws = std::const_pointer_cast<AbstractWorkspace>(shared_from_this());
        root_ = std::make_shared<ParentContainer>(
            shell_application_manager,
            state,
            window_controller,
            config,
            get_output_area(output.lock()),
            mutable_ws,
            nullptr);
    }

    return root_;
}

geom::Rectangle Workspace::area() const
{
    if (auto const sh_output = output.lock())
        return get_output_area(sh_output);
    return {};
}

void Workspace::recalculate_area()
{
    auto const sh_output = output.lock();

    // A workspace that isn't being shown has all of its containers hidden, so resizing
    // them now would push geometry at unmapped clients (and the windows may drift from
    // their containers in the meantime). Instead, we note that a recalculation is pending
    // and apply it the moment that this workspace is shown again.
    if (sh_output && sh_output->active().get() != this)
    {
        needs_area_recalculation_ = true;
        return;
    }

    needs_area_recalculation_ = false;
    if (sh_output)
    {
        root()->set_logical_area(get_output_area(sh_output), true);
        root()->commit_changes();
    }

    // The output's position/size may have changed, so refresh each window's
    // cached output area in the render data. Otherwise the renderer culls
    // windows whose stale output area no longer overlaps the viewport.
    for_each_window([](std::shared_ptr<WindowContainer> const& container)
    {
        container->update_output_area();
        return false;
    });

    registry->advise_area_changed(id());
}

void Workspace::delete_container(std::shared_ptr<Container> const& container)
{
    if (auto const leaf = Container::as_leaf(container))
    {
        auto const parent = handle_remove_container(leaf);
        parent->commit_changes();
    }
    else
    {
        remove_other_container(container);
    }

    if (is_empty())
        registry->advise_empty(id());
}

void Workspace::advise_focus_gained(std::shared_ptr<Container> const& container)
{
    auto const lock = sync.lock();
    if (!lock->is_showing)
        lock->last_selected_container = container;
}

void Workspace::show(geom::Point const& origin)
{
    // The output area may have changed while we were hidden, in which case the
    // recalculation was deferred until now.
    if (needs_area_recalculation_)
        recalculate_area();

    if (!config->are_animations_enabled() || origin == geom::Point(0, 0))
    {
        // No animation will run to settle these to their final shown values,
        // so reset them now. Otherwise stale state from a prior animated hide
        // (alpha_=0, offscreen transform) keeps the windows invisible.
        transform(glm::mat4(1.f));
        alpha(1.f);
        on_animation_start(false);
        return;
    }

    auto const area = root()->get_logical_area();
    std::weak_ptr<Workspace> const that = std::dynamic_pointer_cast<Workspace>(shared_from_this());
    AnimationData show_anim_data(AnimateableEvent::workspace_switch,
        geom::Rectangle(origin, area.size),
        geom::Rectangle(geom::Point(0, 0), area.size),
        0.f, 1.f);
    show_anim_data.workspace = from_workspace(std::dynamic_pointer_cast<AbstractWorkspace>(shared_from_this()));
    if (name().has_value())
        show_anim_data.workspace_name = name().value();
    animator->append(Animation(
        animation_handle,
        config->get_animation_definition(AnimateableEvent::workspace_switch),
        std::move(show_anim_data),
        [that = that](AnimationFrameResult const& asr)
    {
        if (auto const locked = that.lock())
        {
            locked->window_controller->invoke_under_lock([asr = asr, that = that]()
            {
                auto const locked = that.lock();
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
                    locked->on_animation_end(false);
            });
        }
    }, plugin_manager));
    on_animation_start(false);
}

void Workspace::hide(geom::Point const& end)
{
    if (!config->are_animations_enabled())
    {
        on_animation_end(true);
        return;
    }

    auto const area = root()->get_logical_area();
    AnimationData hide_anim_data(AnimateableEvent::workspace_switch,
        geom::Rectangle(geom::Point(0, 0), area.size),
        geom::Rectangle(end, area.size),
        1.f, 0.f);
    hide_anim_data.workspace = from_workspace(std::dynamic_pointer_cast<AbstractWorkspace>(shared_from_this()));
    if (name().has_value())
        hide_anim_data.workspace_name = name().value();
    animator->append(Animation(
        animation_handle,
        config->get_animation_definition(AnimateableEvent::workspace_switch),
        std::move(hide_anim_data),
        [this](AnimationFrameResult const& asr)
    {
        window_controller->invoke_under_lock([asr = asr, this]()
        {
            auto const locked = std::dynamic_pointer_cast<Workspace>(shared_from_this());
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
                locked->on_animation_end(true);
        });
    }, plugin_manager));
    on_animation_start(true);
}

bool Workspace::for_each_window(std::function<bool(std::shared_ptr<WindowContainer>)> const& f) const
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

            auto container = window_controller->get_window_container(leaf->window().value());
            if (container && f(container))
                return true;
        }

        return false;
    };

    for (auto const& other : other_containers)
    {
        if (auto const locked = other.lock())
        {
            if (foreach_node_internal(_for_each_window, locked))
                return true;
        }
    }

    if (foreach_node_internal(_for_each_window, root()))
        return true;

    return false;
}

bool Workspace::move_container(miracle::Direction direction, Container& container)
{
    auto traversal_result = handle_move(container, direction);
    switch (traversal_result.traversal_type)
    {
    case MoveResult::traversal_type_insert:
    {
        if (auto const wc = Container::as_window_container(container.shared_from_this()))
            wc->move_to(*traversal_result.node);
        break;
    }
    case MoveResult::traversal_type_append:
    {
        auto lane_node = Container::as_parent(traversal_result.node);
        auto moving_node = container.shared_from_this();
        handle_remove_container(moving_node);
        lane_node->add_child(moving_node, lane_node->num_children());
        lane_node->commit_changes();
        break;
    }
    case MoveResult::traversal_type_prepend:
    {
        auto lane_node = Container::as_parent(traversal_result.node);
        auto moving_node = container.shared_from_this();
        handle_remove_container(moving_node);
        lane_node->add_child(moving_node, 0);
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
    root()->add_child(to_move.shared_from_this(), root()->num_children());
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
        auto const new_layout_direction = from_direction(direction);
        if (new_layout_direction == root()->get_scheme())
            return {};

        auto const after_root_lane = std::make_shared<ParentContainer>(
            shell_application_manager,
            state,
            window_controller,
            config,
            get_output_area(output.lock()),
            shared_from_this(),
            nullptr);
        after_root_lane->set_layout(new_layout_direction);
        after_root_lane->add_child(root(), 0);
        root_ = after_root_lane;
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

std::shared_ptr<AbstractOutput> Workspace::get_output() const
{
    return output.lock();
}

void Workspace::set_output(std::shared_ptr<AbstractOutput> const& new_output)
{
    this->output = new_output;
    recalculate_area();
}

bool Workspace::is_empty() const
{
    return root()->num_children() == 0 && other_containers.empty();
}

void Workspace::graft(std::shared_ptr<Container> const& container)
{
    if (container->plugin_handle().has_value() || std::dynamic_pointer_cast<FreestyleWindowContainer>(container))
    {
        // TODO: Assuming that this is always called on the active workspace
        add_other_container(container, true);
    }
    else if (auto const leaf = Container::as_leaf(container))
    {
        root()->add_child(leaf, root()->num_children());
        root()->commit_changes();
        container->set_workspace(shared_from_this());
    }
}

void Workspace::num(std::optional<int> n)
{
    sync.lock()->num_ = n;
}

void Workspace::name(std::optional<std::string> const& name)
{
    sync.lock()->name_ = name;
}

std::optional<Gaps> Workspace::outer_gaps() const
{
    return sync.lock()->workspace_outer_gaps;
}

void Workspace::outer_gaps(std::optional<Gaps> const& gaps)
{
    sync.lock()->workspace_outer_gaps = gaps;
    recalculate_area();
}

std::optional<Gaps> Workspace::inner_gaps() const
{
    return sync.lock()->workspace_inner_gaps;
}

void Workspace::inner_gaps(std::optional<Gaps> const& gaps)
{
    sync.lock()->workspace_inner_gaps = gaps;
    recalculate_area();
}

void Workspace::transform(glm::mat4 const& transform)
{
    sync.lock()->transform_ = transform;
    for_each_container([transform](auto const& container)
    {
        container->set_workspace_transform(transform);
    });
}

glm::mat4 Workspace::transform() const
{
    return sync.lock()->transform_;
}

void Workspace::alpha(float a)
{
    sync.lock()->alpha_ = a;
    for_each_container([a](auto const& container)
    {
        container->set_workspace_alpha(a);
    });
}

float Workspace::alpha() const
{
    return sync.lock()->alpha_;
}

void Workspace::on_animation_start(bool is_hiding)
{
    if (!is_hiding)
    {
        // HACK: miral will try to select a newly visible window if none is currently
        // selected. In most instances, we do not want this, as we would rather
        // select our [last_selected_container] instead. To work around this, we set
        // a flag that tells miral not to select the last focused container while we
        // are in the process of becoming visible.
        sync.lock()->is_showing = true;
        for_each_container([](auto const& container)
        {
            container->show();
        });
        sync.lock()->is_showing = false;

        if (auto const sh_last_selected = sync.lock()->last_selected_container.lock())
        {
            if (sh_last_selected->window().has_value())
                window_controller->select_active_window(sh_last_selected->window().value());
            return;
        }

        if (!for_each_window([&](std::shared_ptr<WindowContainer> const& container)
        {
            if (container->window().has_value())
            {
                window_controller->select_active_window(container->window().value());
                return true;
            }

            return false;
        }))
        {
            window_controller->select_active_window({});
        }
    }
}

void Workspace::on_animation_end(bool is_hiding)
{
    if (is_hiding)
    {
        for_each_container([](auto const& container)
        {
            container->hide();
        });
    }
}

ParentContainer* Workspace::get_layout_container() const
{
    if (!state->focused_container())
        return root().get();

    auto parent = state->focused_container()->get_parent().lock();
    if (!parent)
        return root().get();

    if (parent->get_workspace().get() != this)
        return root().get();

    return parent.get();
}

void Workspace::add_other_container(std::shared_ptr<Container> const& container, bool is_active)
{
    container->set_workspace(shared_from_this());
    other_containers.push_back(container);
    if (is_active)
        container->show();
    else
        container->hide();
}

void Workspace::remove_other_container(std::shared_ptr<Container> const& container)
{
    std::erase_if(other_containers, [container](auto const& other)
    {
        return other.lock() == container;
    });
}

void Workspace::for_each_container(std::function<void(std::shared_ptr<Container> const&)> const& f)
{
    if (root_)
        f(root_);

    for (auto const& other : other_containers)
    {
        if (auto const lock = other.lock())
            f(lock);
    }
}

std::shared_ptr<ParentContainer> Workspace::get_root() const
{
    return root();
}

std::string Workspace::display_name() const
{
    std::stringstream ss;
    auto const lock = sync.lock();
    if (lock->num_ && lock->name_)
        ss << lock->num_.value() << ":" << lock->name_.value();
    else if (lock->name_)
        return lock->name_.value();
    else if (lock->num_)
        return std::to_string(lock->num_.value());
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
    auto const lock = sync.lock();
    return {
        {
         "num",
         lock->num_ ? lock->num_.value() : -1,
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
    nlohmann::json nodes = nlohmann::json::array();
    for (auto const& container : root()->children())
        nodes.push_back(container->to_json(is_active_on_output));

    for (auto const& container : other_containers)
    {
        if (auto const locked = container.lock())
            floating_nodes.push_back(locked->to_json(is_active_on_output));
    }

    auto const num_ = sync.lock()->num_;
    return {
        {
         "num",
         num_ != std::nullopt ? num_.value() : -1,
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
