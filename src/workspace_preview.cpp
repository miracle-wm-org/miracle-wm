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

#include "workspace_preview.h"

#include "abstract_workspace.h"

using namespace miracle;

WorkspacePreview::~WorkspacePreview()
{
    release();
}

void WorkspacePreview::acquire(std::vector<std::shared_ptr<AbstractWorkspace>> const& workspaces)
{
    for (auto const& workspace : workspaces)
    {
        if (!workspace)
            continue;

        // [begin_preview] refuses workspaces that are already in the scene, so
        // only the ones we are actually responsible for end up on the list.
        if (workspace->begin_preview())
            revealed.push_back(workspace);
    }
}

void WorkspacePreview::release()
{
    // Moved out first so that a re-entrant release - or one that runs while a
    // workspace is being deleted - has nothing left to do.
    auto const to_conceal = std::move(revealed);
    revealed.clear();

    for (auto const& weak : to_conceal)
    {
        if (auto const workspace = weak.lock())
            workspace->end_preview();
    }
}

bool WorkspacePreview::held() const
{
    return !revealed.empty();
}
