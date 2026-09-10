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

#ifndef OVERVIEW_SCENE_OVERRIDE_DELEGATE_H
#define OVERVIEW_SCENE_OVERRIDE_DELEGATE_H

#include <cstdint>

namespace miracle
{

class OverviewSceneOverrideDelegate
{
public:
    virtual ~OverviewSceneOverrideDelegate() = default;

    /// Runs on the calling (main) thread as soon as the overview starts going
    /// away, whether or not an exit animation follows.
    virtual void on_exit_started() = 0;

    /// Runs on the main thread when the user picks a workspace out of
    /// [OverviewSceneOverride::Level::workspaces].
    virtual void on_workspace_selected(uint32_t workspace_id) = 0;

    /// Runs on the main thread when the overview is dismissed, with the id of
    /// the output the dismissal acted on.
    virtual void on_output_selected(int output_id) = 0;

    /// Runs when the override may be released, which for an animated exit is on
    /// the animator thread.
    virtual void on_done() = 0;
};

} // miracle

#endif // OVERVIEW_SCENE_OVERRIDE_DELEGATE_H
