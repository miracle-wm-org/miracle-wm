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

#include "scene_override.h"

using namespace miracle;

std::optional<uint32_t> SceneOverrideManager::try_override(std::unique_ptr<SceneOverride> override)
{
    std::lock_guard lock(mutex);
    if (current)
        return std::nullopt;

    current = std::move(override);
    current_token = ++next_token;
    return current_token;
}

bool SceneOverrideManager::try_release_override(uint32_t token)
{
    std::lock_guard lock(mutex);
    if (!current || token != current_token)
        return false;

    current.reset();
    return true;
}

std::shared_ptr<SceneOverride> SceneOverrideManager::try_resolve()
{
    std::lock_guard lock(mutex);
    return current;
}
