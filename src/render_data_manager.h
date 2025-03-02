/**
Copyright (C) 2024  Matthew Kosarek

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

#include "mir/graphics/renderable.h"
#include <glm/glm.hpp>
#include <memory>
#include <mir/scene/surface.h>
#include <vector>

namespace mir
{
namespace input
{
class Scene;
}
}

namespace miracle
{
class Container;
class Animator;
class Config;

struct RenderData
{
    mir::scene::Surface* surface;
    bool needs_outline = false;
    bool is_focused = false;
    glm::mat4 transform = glm::mat4(1.f);
    glm::mat4 workspace_transform = glm::mat4(1.f);

    // TODO: This might be wildly inefficent

    mir::graphics::RenderableList previous;
};

class RenderDataManager
{
public:
    RenderDataManager();
    void set_config(std::shared_ptr<Config> const& config);
    void set_animator(std::shared_ptr<Animator> const&);
    void set_input_scene(std::shared_ptr<mir::input::Scene> const& scene);
    void add(Container const&);
    void remove(Container const&);
    void transform_change(Container const&);
    void workspace_transform_change(Container const&);
    void focus_change(Container const&);
    std::vector<RenderData> const& get();
    mir::graphics::RenderableList extra_renderables() const;
    void set_last(mir::graphics::RenderableList const& last);
    void remove_animating_renderable(std::shared_ptr<mir::graphics::Renderable> const& renderable);

private:
    std::mutex mutex;
    std::vector<RenderData> render_data;
    std::vector<RenderData> copy_for_renderer;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<Config> config;
    std::shared_ptr<mir::input::Scene> scene;
    mir::graphics::RenderableList animating_renderables;
};

} // miracle

#endif // MIRACLEWM_SURFACE_TRACKER_H
