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

#include "scene_override_compositor.h"
#include "compositor_state.h"
#include "mir_version_manager.h"
#include "scene_override.h"

#include <mir/compositor/display_buffer_compositor.h>
#include <mir/compositor/scene_element.h>
#include <mir/graphics/renderable.h>
#include <mir/server.h>
#include <utility>

using namespace miracle;

namespace
{
/// Forwards everything to the wrapped renderable except an identity
/// transformation, which becomes [occlusion_exempt_transform].
///
/// A renderable that is already transformed is left alone: Mir exempts it from
/// the occlusion pass for exactly the same reason, and the renderer may well be
/// reading that transformation.
class ExemptRenderable : public mir::graphics::Renderable
{
public:
    explicit ExemptRenderable(std::shared_ptr<mir::graphics::Renderable> r) :
        renderable { std::move(r) }
    {
    }

    ID id() const override
    {
        return renderable->id();
    }

    std::shared_ptr<mir::graphics::Buffer> buffer() const override
    {
        return renderable->buffer();
    }

    mir::geometry::Rectangle screen_position() const override
    {
        return renderable->screen_position();
    }

    mir::geometry::RectangleD src_bounds() const override
    {
        return renderable->src_bounds();
    }

    std::optional<mir::geometry::Rectangle> clip_area() const override
    {
        return renderable->clip_area();
    }

    float alpha() const override
    {
        return renderable->alpha();
    }

    glm::mat4 transformation() const override
    {
        static glm::mat4 const identity(1.f);
        auto const real = renderable->transformation();
        return real == identity ? occlusion_exempt_transform : real;
    }

    bool shaped() const override
    {
        return renderable->shaped();
    }

    auto surface_if_any() const -> std::optional<mir::scene::Surface const*> override
    {
        return renderable->surface_if_any();
    }

    MirOrientation orientation() const override
    {
        return renderable->orientation();
    }

    auto mirror_mode() const -> MirMirrorMode override
    {
        return renderable->mirror_mode();
    }

#ifdef MIR_VERSION_2_24_OR_GREATER
    auto opaque_region() const -> std::optional<mir::geometry::Rectangles> override
    {
        return renderable->opaque_region();
    }
#endif

private:
    std::shared_ptr<mir::graphics::Renderable> const renderable;
};

class ExemptSceneElement : public mir::compositor::SceneElement
{
public:
    explicit ExemptSceneElement(std::shared_ptr<mir::compositor::SceneElement> e) :
        element { std::move(e) },
        exempt { std::make_shared<ExemptRenderable>(element->renderable()) }
    {
    }

    std::shared_ptr<mir::graphics::Renderable> renderable() const override
    {
        return exempt;
    }

    // Both are forwarded so that Mir's rendering tracker keeps driving the
    // client's frame callbacks exactly as it would have.
    void rendered() override
    {
        element->rendered();
    }

    void occluded() override
    {
        element->occluded();
    }

private:
    std::shared_ptr<mir::compositor::SceneElement> const element;
    std::shared_ptr<mir::graphics::Renderable> const exempt;
};

}

SceneOverrideDisplayBufferCompositor::SceneOverrideDisplayBufferCompositor(
    std::unique_ptr<mir::compositor::DisplayBufferCompositor> wrapped,
    std::shared_ptr<CompositorState> compositor_state) :
    wrapped { std::move(wrapped) },
    compositor_state { std::move(compositor_state) }
{
}

bool SceneOverrideDisplayBufferCompositor::composite(mir::compositor::SceneElementSequence&& sequence)
{
    // Resolving pins the override for the duration of the call, exactly as the
    // renderer does, so a release on another thread cannot land halfway through
    // the sequence and exempt only part of it.
    if (compositor_state->scene_override_manager()->try_resolve())
    {
        for (auto& element : sequence)
            element = std::make_shared<ExemptSceneElement>(std::move(element));
    }

    return wrapped->composite(std::move(sequence));
}

SceneOverrideDisplayBufferCompositorFactory::SceneOverrideDisplayBufferCompositorFactory(
    std::shared_ptr<mir::compositor::DisplayBufferCompositorFactory> wrapped,
    std::shared_ptr<CompositorState> compositor_state) :
    wrapped { std::move(wrapped) },
    compositor_state { std::move(compositor_state) }
{
}

std::unique_ptr<mir::compositor::DisplayBufferCompositor>
SceneOverrideDisplayBufferCompositorFactory::create_compositor_for(
    mir::graphics::DisplaySink& display_sink)
{
    return std::make_unique<SceneOverrideDisplayBufferCompositor>(
        wrapped->create_compositor_for(display_sink), compositor_state);
}

void SceneOverrideCompositor::operator()(mir::Server& server) const
{
    server.wrap_display_buffer_compositor_factory(
        [state = compositor_state](std::shared_ptr<mir::compositor::DisplayBufferCompositorFactory> const& wrapped)
            -> std::shared_ptr<mir::compositor::DisplayBufferCompositorFactory>
    {
        return std::make_shared<SceneOverrideDisplayBufferCompositorFactory>(wrapped, state);
    });
}
