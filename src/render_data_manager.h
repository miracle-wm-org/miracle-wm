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

#ifndef MIRACLEWM_SURFACE_TRACKER_H
#define MIRACLEWM_SURFACE_TRACKER_H

#include <glm/glm.hpp>
#include <mir/scene/surface.h>
#include <mutex>
#include <vector>

namespace miracle
{

class Container;
typedef int RenderDataManagerId;

struct RenderData
{
    RenderDataManagerId id = 0;
    mir::scene::Surface const* surface = nullptr;
    bool needs_outline = false;
    bool is_focused = false;
    glm::mat4 transform = glm::mat4(1.f);
    glm::mat4 workspace_transform = glm::mat4(1.f);
    std::optional<mir::geometry::Rectangle> output_area;
    std::optional<uint8_t> shader_id = std::nullopt;
};

class RenderDataManager
{
public:
    RenderDataManager();
    RenderDataManagerId add(RenderData const&&);
    void remove(RenderDataManagerId id);
    void transform_change(RenderDataManagerId id, glm::mat4 const& transform);
    void workspace_transform_change(RenderDataManagerId id, glm::mat4 const& transform);
    void output_area_change(RenderDataManagerId id, mir::geometry::Rectangle const& area);
    void focus_change(RenderDataManagerId id, bool is_focused);
    void needs_outline_change(RenderDataManagerId id, bool needs_outline);
    std::vector<RenderData> const& get();

private:
    RenderDataManagerId next_id = 0;
    std::mutex mutex;
    std::vector<RenderData> render_data;
    std::vector<RenderData> copy_for_renderer;
};

} // miracle

#endif // MIRACLEWM_SURFACE_TRACKER_H
