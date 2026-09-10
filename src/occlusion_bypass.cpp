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

#include "occlusion_bypass.h"
#include "window_container.h"

#include <algorithm>
#include <mir/scene/surface.h>

using namespace miracle;

namespace
{
mir::scene::Surface const* surface_key(std::shared_ptr<WindowContainer> const& container)
{
    auto const window = container->window();
    if (!window)
        return nullptr;

    return window->operator std::shared_ptr<mir::scene::Surface>().get();
}
}

OcclusionBypass::~OcclusionBypass()
{
    release();
}

void OcclusionBypass::acquire(std::vector<std::shared_ptr<WindowContainer>> const& containers)
{
    for (auto const& container : containers)
    {
        if (!container)
            continue;

        auto const already_acquired = std::ranges::find_if(flagged, [&](auto const& f)
        {
            return container == f.container.lock();
        }) != flagged.end();

        if (already_acquired)
            continue;

        flagged.push_back({ container, surface_key(container) });
        container->set_occlusion_bypass(true);
    }
}

void OcclusionBypass::forget(mir::scene::Surface const* surface)
{
    std::erase_if(flagged, [&](auto const& f)
    {
        return f.key == surface;
    });
}

void OcclusionBypass::release()
{
    auto const to_clear = std::move(flagged);
    flagged.clear();

    for (auto const& entry : to_clear)
    {
        if (auto const container = entry.container.lock())
            container->set_occlusion_bypass(false);
    }
}

bool OcclusionBypass::held() const
{
    return !flagged.empty();
}
