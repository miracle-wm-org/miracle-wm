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

#ifndef WORKSPACE_PREVIEW_H
#define WORKSPACE_PREVIEW_H

#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace miracle
{
class AbstractWorkspace;

/// Keeps workspaces that are not active present in the scene for as long as the
/// preview is held.
///
/// Workspaces remove their windows from the scene entirely when they are
/// switched away from, so an effect that wants to draw one - an output
/// overview, a workspace switcher, a wall of thumbnails - has to ask for it
/// back first. Holding one of these instead of driving
/// [AbstractWorkspace::set_containers_shown] directly means the reveal is
/// always undone exactly once, for exactly the workspaces that were revealed,
/// and that each one gets the transform and alpha it had beforehand back.
///
/// Every method must be called on the window management thread. An effect whose
/// teardown runs elsewhere (the animator thread, say) should hand the release
/// to [WindowController::invoke_under_lock].
class WorkspacePreview
{
public:
    WorkspacePreview() = default;
    WorkspacePreview(WorkspacePreview const&) = delete;
    WorkspacePreview& operator=(WorkspacePreview const&) = delete;

    /// Releases the preview, in case the holder forgot to.
    ~WorkspacePreview();

    /// Reveals every workspace in \p workspaces that is not already in the
    /// scene, remembering the ones it revealed.
    ///
    /// A hidden workspace was left translated off screen and fully transparent
    /// by its hide animation, so its transform and alpha are snapped to their
    /// shown values - and remembered for [release] - before its windows go back
    /// into the scene. No animation runs and no focus selection happens.
    ///
    /// Workspaces that are already visible - the active one on each output, and
    /// anything a previous [acquire] already revealed - are skipped, so calling
    /// this more than once is safe and additive.
    void acquire(std::vector<std::shared_ptr<AbstractWorkspace>> const& workspaces);

    /// Conceals everything [acquire] revealed. Idempotent.
    void release();

    /// Whether anything is currently revealed.
    [[nodiscard]] bool held() const;

private:
    struct Revealed
    {
        /// Held weakly: a workspace that is deleted mid-preview simply stops
        /// resolving, and there is nothing left to conceal.
        std::weak_ptr<AbstractWorkspace> workspace;

        /// What the workspace looked like before it was revealed, to be put
        /// back by [release].
        glm::mat4 transform;
        float alpha;
    };

    std::vector<Revealed> revealed;
};
}

#endif
