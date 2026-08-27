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

#ifndef WORKSPACE_MANAGER_H
#define WORKSPACE_MANAGER_H

#include "workspace_observer.h"

#include <memory>
#include <vector>

namespace miracle
{

class AbstractOutput;
class Config;
class OutputManager;

/// A helper class for performing operations on workspaces.
///
/// True references to [WorkspaceInterface]s are held by their respective
/// [OutputInterface]s. This class is a helper that allows the caller
/// to tread workspaces as a uniform list of workspace objects.
///
/// See also:
///  - [miracle::WorkspaceInterface], the interface for workspaces
///  - [miracle::Workspace], the primary implementation of this workspace
class WorkspaceManager
{
public:
    WorkspaceManager(
        std::shared_ptr<WorkspaceObserverRegistrar> const& registry,
        std::shared_ptr<Config> const& config,
        std::shared_ptr<OutputManager> const& output_manager);
    WorkspaceManager(WorkspaceManager const&) = delete;
    virtual ~WorkspaceManager() = default;

    /// Request workspace by number.
    ///
    /// If it does not yet exist, then one is created on the provided output.
    ///
    /// If it does exist, we navigate  to the screen containing that workspace and show it if it
    /// isn't already shown.
    ///
    /// \param output_hint output that we want to show on if creating a new workspace
    /// \param key workspace number that we want to create
    /// \param allow_back_and_forth
    /// \returns true if we focused a new workspace, otherwise false
    bool request_workspace(
        AbstractOutput* output_hint,
        int key,
        bool allow_back_and_forth = true);

    /// Request the workspace by name.
    ///
    /// If it does not yet exist, then one is created on the provided output.
    ///
    /// If the workspace does exist, then we navigate to the screen containing
    /// the workspace and show the workspace.
    bool request_workspace(
        AbstractOutput* output_hint,
        std::string const& name,
        bool allow_back_and_forth = true);

    /// Returns any available workspace with the lowest numerical value starting with 1.
    int request_first_available_workspace(AbstractOutput* output);

    /// Selects the next workspace after the current selected one.
    bool request_next(AbstractOutput* output);

    /// Selects the workspace before the current selected one
    bool request_prev(AbstractOutput* output);

    bool request_back_and_forth();

    bool request_next_on_output(AbstractOutput const&);

    bool request_prev_on_output(AbstractOutput const&);

    bool delete_workspace(uint32_t id);

    /// Focuses the workspace with the provided id
    bool request_focus(uint32_t id);

    /// Returns the workspace with the provided [id], if any.
    AbstractWorkspace* workspace(uint32_t id) const;

    /// Builds and returns a sorted array of all active workspaces.
    std::vector<std::shared_ptr<AbstractWorkspace>> workspaces() const;

    /// Moves the workspace associated with [id] to the [hint].
    ///
    /// If the workspace is already on the output, then nothing happens.
    ///
    /// If the workspace is the final workspace on its previous output,
    /// then that output will have a new workspace assigned to it.
    void move_workspace_to_output(uint32_t id, AbstractOutput* hint);

    bool set_workspace_num(uint32_t id, std::optional<int> const& num);
    bool set_workspace_name(uint32_t id, std::optional<std::string> const& name);

    /// Request a workspace by optional number and/or name.
    ///
    /// If a workspace with the given number or name already exists, it is returned.
    /// Otherwise, a new workspace is created on the provided output.
    ///
    /// \param focus if true, the workspace will be focused after creation/lookup
    /// \returns the workspace, or nullptr if it could not be created
    AbstractWorkspace* request_workspace(
        AbstractOutput* output_hint,
        std::optional<int> num,
        std::optional<std::string> const& name,
        bool focus);

    /// The number of default workspaces
    static constexpr int NUM_DEFAULT_WORKSPACES = 10;

private:
    bool focus_existing(AbstractWorkspace const*, bool back_and_forth);

    /// As [request_workspace], but [focus_output] may be false to activate the
    /// workspace without moving output focus onto it. Used when a workspace is
    /// created for bookkeeping reasons (e.g. a new output was plugged in)
    /// rather than because the user asked for it.
    bool request_workspace(
        AbstractOutput* output_hint,
        int key,
        bool allow_back_and_forth,
        bool focus_output);

    /// As [request_focus], but [focus_output] may be false to activate the
    /// workspace without moving output focus onto it.
    bool request_focus(uint32_t id, bool focus_output);

    // Note: `0` is a special workspace number meaning "no workspace" in some contexts.
    uint32_t next_id = 1;

    AbstractWorkspace* workspace(int num) const;
    AbstractWorkspace* workspace(std::string const& name) const;

    std::shared_ptr<WorkspaceObserverRegistrar> registry;
    std::shared_ptr<Config> config;
    std::shared_ptr<OutputManager> output_manager;
    std::weak_ptr<AbstractWorkspace> last_selected;
};
}

#endif
