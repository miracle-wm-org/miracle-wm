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
#include "abstract_output.h"
#include "abstract_workspace.h"

#include <algorithm>

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

        // The active workspace is already in the scene, and revealing it would
        // clobber the state of windows that were never hidden in the first place.
        auto const sh_output = workspace->get_output();
        if (sh_output && sh_output->active() == workspace)
            continue;

        auto const already_acquired = std::ranges::find_if(revealed, [&](auto const& r)
        {
            return workspace == r.workspace.lock();
        }) != revealed.end();

        if (already_acquired)
            continue;

        revealed.push_back({ workspace, workspace->transform(), workspace->alpha() });

        // A hidden workspace was left translated off screen and fully transparent
        // by its hide animation, so snap it back before un-hiding its windows.
        workspace->transform(glm::mat4(1.f));
        workspace->alpha(1.f);
        workspace->set_containers_shown(true);
    }
}

void WorkspacePreview::release()
{
    auto const to_conceal = std::move(revealed);
    revealed.clear();

    for (auto const& entry : to_conceal)
    {
        auto const workspace = entry.workspace.lock();
        if (!workspace)
            continue;

        // A workspace that became the active one while it was being previewed is
        // legitimately in the scene now, so the reveal is simply dropped: hiding
        // it again would undo the switch that adopted it.
        auto const sh_output = workspace->get_output();
        if (sh_output && sh_output->active() == workspace)
            continue;

        workspace->set_containers_shown(false);
        workspace->transform(entry.transform);
        workspace->alpha(entry.alpha);
    }
}

bool WorkspacePreview::held() const
{
    return !revealed.empty();
}
