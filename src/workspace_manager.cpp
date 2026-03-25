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

#define MIR_LOG_COMPONENT "workspace_manager"

#include "workspace_manager.h"
#include "abstract_output.h"
#include "config.h"
#include "output_manager.h"
#include "vector_helpers.h"
#include <mir/log.h>

using namespace mir::geometry;
using namespace miracle;

WorkspaceManager::WorkspaceManager(
    std::shared_ptr<WorkspaceObserverRegistrar> const& registry,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<OutputManager> const& output_manager) :
    registry { registry },
    config { config },
    output_manager { output_manager }
{
}

bool WorkspaceManager::focus_existing(AbstractWorkspace const* existing, bool back_and_forth)
{
    mir::log_info("focus_existing: %s focused", existing->display_name().c_str());
    auto const& active_workspace = output_manager->focused()->active();
    if (active_workspace.get() == existing)
    {
        if (!last_selected.expired())
        {
            /// If back_and_forth isn't true and we have a last_output, don't visit it.
            if (!back_and_forth)
                return false;

            return request_focus(last_selected.lock()->id());
        }
        else
            return false;
    }

    request_focus(existing->id());
    return true;
}

bool WorkspaceManager::request_workspace(
    AbstractOutput* output_hint,
    int num,
    bool allow_back_and_forth)
{
    if (auto const& existing = workspace(num))
    {
        const bool back_and_forth = allow_back_and_forth ? config->get_workspace_back_and_forth() : false;
        return focus_existing(existing, back_and_forth);
    }

    mir::log_info("request_workspace: %d being created", num);
    uint32_t id = next_id++;
    auto const& workspace_config = config->get_workspace_config(num, std::nullopt);
    output_hint->advise_new_workspace({ .id = id,
        .num = num,
        .name = workspace_config.name,
        .registrar = registry });
    request_focus(id);
    registry->advise_created(id);
    return true;
}

bool WorkspaceManager::request_workspace(
    AbstractOutput* output_hint,
    std::string const& name,
    bool allow_back_and_forth)
{
    if (auto const& existing = workspace(name))
    {
        const bool back_and_forth = allow_back_and_forth ? config->get_workspace_back_and_forth() : false;
        return focus_existing(existing, back_and_forth);
    }

    mir::log_info("request_workspace: %s being created", name.c_str());
    uint32_t id = next_id++;
    output_hint->advise_new_workspace({ .id = id,
        .num = std::nullopt,
        .name = name,
        .registrar = registry });
    request_focus(id);
    registry->advise_created(id);
    return true;
}

int WorkspaceManager::request_first_available_workspace(AbstractOutput* output)
{
    for (int i = 1; i < NUM_DEFAULT_WORKSPACES; i++)
    {
        if (workspace(i))
            continue;

        request_workspace(output, i, true);
        return i;
    }

    if (workspace(0) == nullptr)
    {
        request_workspace(output, 0, true);
        return 0;
    }

    return -1;
}

bool WorkspaceManager::request_next(AbstractOutput* output)
{
    auto const& active = output->active();
    if (!active)
        return false;

    auto const l = workspaces();
    for (auto it = l.begin(); it != l.end(); it++)
    {
        if (*it == active)
        {
            it++;
            if (it == l.end())
                it = l.begin();

            return focus_existing(it->get(), false);
        }
    }

    return false;
}

bool WorkspaceManager::request_prev(AbstractOutput* output)
{
    auto const& active = output->active();
    if (!active)
        return false;

    auto const l = workspaces();
    for (auto it = l.rbegin(); it != l.rend(); it++)
    {
        if (*it == active)
        {
            it++;
            if (it == l.rend())
                it = l.rbegin();

            return focus_existing(it->get(), false);
        }
    }

    return false;
}

bool WorkspaceManager::request_back_and_forth()
{
    if (!last_selected.expired())
    {
        request_focus(last_selected.lock()->id());
        return true;
    }

    return false;
}

bool WorkspaceManager::request_next_on_output(AbstractOutput const& output)
{
    auto const& active = output.active();
    if (!active)
        return false;

    auto const& workspaces = output.get_workspaces();
    for (auto it = workspaces.begin(); it != workspaces.end(); it++)
    {
        if (*it == active)
        {
            it++;
            if (it == workspaces.end())
                it = workspaces.begin();

            return focus_existing(it->get(), false);
        }
    }

    return false;
}

bool WorkspaceManager::request_prev_on_output(AbstractOutput const& output)
{
    auto const& active = output.active();
    if (!active)
        return false;

    auto const& workspaces = output.get_workspaces();
    for (auto it = workspaces.rbegin(); it != workspaces.rend(); it++)
    {
        if (*it == active)
        {
            it++;
            if (it == workspaces.rend())
                it = workspaces.rbegin();

            return focus_existing(it->get(), false);
        }
    }

    return false;
}

bool WorkspaceManager::delete_workspace(uint32_t id)
{
    auto const* w = workspace(id);
    if (!w)
        return false;

    registry->advise_removed(id);
    auto const output = w->get_output();
    output->advise_workspace_deleted(*this, id);
    return true;
}

bool WorkspaceManager::request_focus(uint32_t id)
{
    auto const& existing = workspace(id);
    if (!existing)
    {
        mir::log_error("request_focus: cannot find workspace with id %d", id);
        return false;
    }

    auto const active_screen = output_manager->focused();
    if (active_screen)
    {
        if (auto current = active_screen->active())
            last_selected = current;
        else
            last_selected.reset();
    }
    else
        last_selected.reset();

    std::optional<uint32_t> previous_id;
    if (active_screen != nullptr && !last_selected.expired())
        previous_id = last_selected.lock()->id();

    existing->get_output()->advise_workspace_active(*this, id);

    // Note: the focus event is sent AFTER activation so that IPC clients
    // receive the EMPTY event for the deleted old workspace before the
    // FOCUS event, preventing a brief flash of both workspaces.
    // workspace_to_json handles the case where previous_id no longer exists.
    registry->advise_focused(previous_id, id);
    return true;
}

AbstractWorkspace* WorkspaceManager::workspace(int num) const
{
    for (auto const& output : output_manager->outputs())
    {
        for (auto const& workspace : output->get_workspaces())
        {
            if (workspace->num() == num)
                return workspace.get();
        }
    }

    return nullptr;
}

AbstractWorkspace* WorkspaceManager::workspace(uint32_t id) const
{
    for (auto const& output : output_manager->outputs())
    {
        for (auto const& workspace : output->get_workspaces())
        {
            if (workspace->id() == id)
                return workspace.get();
        }
    }

    return nullptr;
}

AbstractWorkspace* WorkspaceManager::workspace(std::string const& name) const
{
    for (auto const& output : output_manager->outputs())
    {
        for (auto const& workspace : output->get_workspaces())
        {
            if (workspace->name() == name)
                return workspace.get();
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<AbstractWorkspace>> WorkspaceManager::workspaces() const
{
    std::vector<std::shared_ptr<AbstractWorkspace>> result;
    for (auto const& output : output_manager->outputs())
    {
        for (auto const& w : output->get_workspaces())
        {
            insert_sorted(result, w, [](std::shared_ptr<AbstractWorkspace> const& a, std::shared_ptr<AbstractWorkspace> const& b)
            {
                if (a->num() && b->num())
                    return a->num().value() < b->num().value();
                else if (a->num())
                    return true;
                else if (b->num())
                    return false;
                else
                    return true;
            });
        }
    }
    return result;
}

void WorkspaceManager::move_workspace_to_output(uint32_t id, AbstractOutput* hint)
{
    auto const w = workspace(id);
    if (!w)
    {
        mir::log_error("move_workspace_to_output: cannot find workspace with id %d", id);
        return;
    }

    hint->move_workspace_to(*this, w);
}

bool WorkspaceManager::set_workspace_num(uint32_t id, std::optional<int> const& num)
{
    std::shared_ptr<AbstractWorkspace> workspace_to_set;
    for (auto const& workspace : workspaces())
    {
        if (workspace->id() == id)
        {
            workspace_to_set = workspace;
            if (workspace_to_set->num() == num)
                return true; // Already set
        }
        else if (workspace->num() == num)
        {
            mir::log_error("set_workspace_num: Cannot steal another workspace's number");
            return false;
        }
    }

    workspace_to_set->num(num);
    registry->advise_renamed(id);
    return true;
}

AbstractWorkspace* WorkspaceManager::request_workspace(
    AbstractOutput* output_hint,
    std::optional<int> num,
    std::optional<std::string> const& name,
    bool focus)
{
    if (num)
    {
        if (auto const existing = workspace(*num))
        {
            if (focus)
                focus_existing(existing, false);
            return existing;
        }
    }

    if (name)
    {
        if (auto const existing = workspace(*name))
        {
            if (focus)
                focus_existing(existing, false);
            return existing;
        }
    }

    mir::log_info("request_workspace: creating with num=%s, name=%s",
        num ? std::to_string(*num).c_str() : "none",
        name ? name->c_str() : "none");
    uint32_t id = next_id++;
    output_hint->advise_new_workspace({ .id = id,
        .num = num,
        .name = name,
        .registrar = registry });
    if (focus)
        request_focus(id);
    registry->advise_created(id);
    return workspace(id);
}

bool WorkspaceManager::set_workspace_name(uint32_t id, std::optional<std::string> const& name)
{
    std::shared_ptr<AbstractWorkspace> workspace_to_set;
    for (auto const& workspace : workspaces())
    {
        if (workspace->id() == id)
        {
            workspace_to_set = workspace;
            if (workspace_to_set->name() == name)
                return true; // Already set
        }
        else if (workspace->name() == name)
        {
            mir::log_error("set_workspace_name: Cannot steal another workspace's name");
            return false;
        }
    }

    workspace_to_set->name(name);
    registry->advise_renamed(id);
    return true;
}
