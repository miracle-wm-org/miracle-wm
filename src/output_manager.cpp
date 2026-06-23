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

#include "output_manager.h"
#include "abstract_output.h"
#include "output_factory_interface.h"
#include "workspace_manager.h"

using namespace miracle;

OutputManager::OutputManager(
    std::unique_ptr<OutputFactoryInterface> output_factory) :
    output_factory(std::move(output_factory))
{
}

std::shared_ptr<AbstractOutput> OutputManager::create(
    std::string name, int id, mir::geometry::Rectangle area, WorkspaceManager& workspace_manager)
{
    if (outputs_.size() == 1 && outputs_[0]->is_defunct())
    {
        outputs_[0]->unset_defunct();
        outputs_[0]->set_info(id, name);
        outputs_[0]->update_area(area);
    }
    else
    {
        auto const created = output_factory->create(name, id, area);
        outputs_.push_back(created);
        if (created)
            workspace_manager.request_first_available_workspace(created.get());
    }

    if (focused_.expired())
        focus_internal(outputs_.back()->id());

    return outputs_.back();
}

void OutputManager::update(int id, mir::geometry::Rectangle area)
{
    for (auto const& output : outputs_)
    {
        if (output->id() == id)
        {
            output->update_area(area);
            return;
        }
    }
}

bool OutputManager::remove(int id, WorkspaceManager& workspace_manager)
{
    for (auto it = outputs_.begin(); it != outputs_.end(); ++it)
    {
        auto const& output = *it;
        if (output->id() != id)
            continue;

        if (output == focused_.lock())
            unfocus_internal(id);

        if (outputs_.size() == 1)
        {
            outputs_[0]->set_defunct();
            return true;
        }

        // Find the workspace ids
        std::vector<uint32_t> workspaces;
        for (auto const& workspace : output->get_workspaces())
            workspaces.push_back(workspace->id());

        // Find the next available output
        auto next_it = it + 1;
        if (next_it == outputs_.end())
            next_it = outputs_.begin();

        // Move workspaces to the next available output
        for (auto const workspace_id : workspaces)
            workspace_manager.move_workspace_to_output(workspace_id, next_it->get());

        focus(next_it->get()->id());
        outputs_.erase(it);
        return true;
    }

    return false;
}

std::vector<std::shared_ptr<AbstractOutput>> OutputManager::outputs() const
{
    return outputs_;
}

bool OutputManager::focus(int id)
{
    return focus_internal(id);
}

bool OutputManager::focus_internal(int id)
{
    for (auto const& output : outputs_)
    {
        if (output->id() == id)
        {
            focused_ = output;
            return true;
        }
    }

    return false;
}

bool OutputManager::unfocus(int id)
{
    return unfocus_internal(id);
}

bool OutputManager::unfocus_internal(int id)
{
    if (focused_.expired())
        return false;

    if (focused_.lock()->id() != id)
        return false;

    focused_.reset();
    return true;
}

std::shared_ptr<AbstractOutput> OutputManager::focused()
{
    return focused_.lock();
}

std::shared_ptr<AbstractOutput> OutputManager::primary()
{
    for (auto const& output : outputs_)
    {
        if (output->is_primary())
            return output;
    }

    return nullptr;
}

std::shared_ptr<AbstractOutput> OutputManager::non_primary()
{
    for (auto const& output : outputs_)
    {
        if (!output->is_primary())
            return output;
    }

    return nullptr;
}

std::shared_ptr<AbstractOutput> OutputManager::prev()
{
    for (size_t i = outputs_.size() - 1;; i--)
    {
        if (outputs_[i] == focused_.lock())
        {
            auto const j = i == 0 ? outputs_.size() - 1 : i - 1;
            return outputs_[j];
        }

        if (i == 0)
            break;
    }

    return focused_.lock();
}

std::shared_ptr<AbstractOutput> OutputManager::next()
{
    for (size_t i = 0; i < outputs_.size(); i++)
    {
        auto const& output = outputs_[i];
        if (output == focused_.lock())
        {
            if (i + 1 == outputs_.size())
                return outputs_[0];
            else
                return outputs_[i + 1];
        }
    }

    return focused_.lock();
}

std::shared_ptr<AbstractOutput> OutputManager::next(Direction direction)
{
    auto const active = focused_.lock();
    if (!active)
        return nullptr;

    auto const& active_area = active->get_area();
    for (auto const& output : outputs_)
    {
        if (output == active)
            continue;

        auto const& other_area = output->get_area();
        switch (direction)
        {
        case Direction::left:
        {
            if (active_area.top_left.x.as_int() == (other_area.top_left.x.as_int() + other_area.size.width.as_int()))
            {
                return output;
            }
            break;
        }
        case Direction::right:
        {
            if (active_area.top_left.x.as_int() + active_area.size.width.as_int() == other_area.top_left.x.as_int())
            {
                return output;
            }
            break;
        }
        case Direction::up:
        {
            if (active_area.top_left.y.as_int() == (other_area.top_left.y.as_int() + other_area.size.height.as_int()))
            {
                return output;
            }
            break;
        }
        case Direction::down:
        {
            if (active_area.top_left.y.as_int() + active_area.size.height.as_int() == other_area.top_left.y.as_int())
            {
                return output;
            }
            break;
        }
        default:
            return active;
        }
    }

    return active;
}

std::shared_ptr<AbstractOutput> OutputManager::next_in_list(std::vector<std::string> const& names)
{
    if (names.empty())
        return focused();

    auto const current_name = focused()->name();
    size_t next = 0;
    for (size_t i = 0; i < names.size(); i++)
    {
        if (names[i] == current_name)
        {
            next = i + 1;
            break;
        }
    }

    if (next == names.size())
        next = 0;

    for (auto const& output : outputs_)
    {
        if (output->name() == names[next])
            return output;
    }

    return focused_.lock();
}

std::shared_ptr<AbstractOutput> OutputManager::from(int id)
{
    for (auto const& output : outputs_)
    {
        if (output->id() == id)
            return output;
    }

    return nullptr;
}