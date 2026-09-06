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

#ifndef SCENE_OVERRIDE_COMPOSITOR_H
#define SCENE_OVERRIDE_COMPOSITOR_H

#include <glm/glm.hpp>
#include <memory>
#include <mir/compositor/display_buffer_compositor.h>
#include <mir/compositor/display_buffer_compositor_factory.h>

namespace mir
{
class Server;
namespace graphics
{
    class DisplaySink;
}
}

namespace miracle
{
class CompositorState;

/// A transformation that maps every vertex Miracle draws to itself, and that
/// Mir's occlusion pass nonetheless refuses to reason about.
///
/// Mir culls a scene element when an opaque, fully opaque, *untransformed*
/// element in front of it covers it, and it decides that from the scene graph
/// alone. A [SceneOverride] draws surfaces somewhere else entirely - a
/// fullscreen window becomes a thumbnail, and the windows it used to cover
/// become thumbnails beside it - so that judgement is wrong for every surface
/// on screen while an override is up. Reporting a transformation Mir does not
/// recognise opts a renderable out of the pass entirely: it is never culled,
/// and it never contributes coverage that culls anything else.
///
/// Every vertex Miracle draws has z == 0, and the vertex shader pivots about a
/// point whose z and w are zero, so scaling z alone is the identity on the
/// geometry that actually exists. That matters because an untracked surface - a
/// panel or a wallpaper - has no render data, and the renderer falls back to the
/// renderable's own transformation for it.
inline glm::mat4 const occlusion_exempt_transform = glm::mat4 {
    1.f, 0.f, 0.f, 0.f,
    0.f, 1.f, 0.f, 0.f,
    0.f, 0.f, 2.f, 0.f,
    0.f, 0.f, 0.f, 1.f
};

/// Wraps Mir's per-output compositor so that Mir's occlusion pass cannot cull a
/// surface that a [SceneOverride] is drawing somewhere other than where the
/// scene says it is.
///
/// While no override is active this is a straight pass-through, so the normal
/// case keeps every bit of Mir's culling.
class SceneOverrideDisplayBufferCompositor : public mir::compositor::DisplayBufferCompositor
{
public:
    SceneOverrideDisplayBufferCompositor(
        std::unique_ptr<mir::compositor::DisplayBufferCompositor> wrapped,
        std::shared_ptr<CompositorState> compositor_state);

    bool composite(mir::compositor::SceneElementSequence&& sequence) override;

private:
    std::unique_ptr<mir::compositor::DisplayBufferCompositor> const wrapped;
    std::shared_ptr<CompositorState> const compositor_state;
};

/// Builds [SceneOverrideDisplayBufferCompositor]s around the compositors of the
/// factory it wraps.
class SceneOverrideDisplayBufferCompositorFactory : public mir::compositor::DisplayBufferCompositorFactory
{
public:
    SceneOverrideDisplayBufferCompositorFactory(
        std::shared_ptr<mir::compositor::DisplayBufferCompositorFactory> wrapped,
        std::shared_ptr<CompositorState> compositor_state);

    std::unique_ptr<mir::compositor::DisplayBufferCompositor> create_compositor_for(
        mir::graphics::DisplaySink& display_sink) override;

private:
    std::shared_ptr<mir::compositor::DisplayBufferCompositorFactory> const wrapped;
    std::shared_ptr<CompositorState> const compositor_state;
};

/// A miral-style server component that installs the wrapper.
struct SceneOverrideCompositor
{
    std::shared_ptr<CompositorState> compositor_state;

    void operator()(mir::Server& server) const;
};
}

#endif // SCENE_OVERRIDE_COMPOSITOR_H
