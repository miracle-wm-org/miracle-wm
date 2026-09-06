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

#include "compositor_state.h"
#include "mir_version_manager.h"
#include "scene_override.h"
#include "scene_override_compositor.h"

#include <glm/gtc/matrix_transform.hpp>
#include <gtest/gtest.h>
#include <mir/compositor/scene_element.h>
#include <mir/graphics/renderable.h>

using namespace miracle;

namespace
{
class StubSceneOverride : public SceneOverride
{
public:
    void handle_keyboard_event(MirKeyboardEvent const*) override { }
    void handle_pointer_event(MirPointerEvent const*) override { }
    void place(
        mir::scene::Surface const&,
        mir::geometry::Rectangle const&,
        std::vector<SceneOverridePlacement>&) override
    {
    }
    void handle_output_changed() override { }
};

class StubRenderable : public mir::graphics::Renderable
{
public:
    ID id() const override { return this; }
    std::shared_ptr<mir::graphics::Buffer> buffer() const override { return nullptr; }
    mir::geometry::Rectangle screen_position() const override { return screen_position_; }
    mir::geometry::RectangleD src_bounds() const override { return src_bounds_; }
    std::optional<mir::geometry::Rectangle> clip_area() const override { return clip_area_; }
    float alpha() const override { return alpha_; }
    glm::mat4 transformation() const override { return transformation_; }
    MirOrientation orientation() const override { return mir_orientation_left; }
    MirMirrorMode mirror_mode() const override { return mir_mirror_mode_horizontal; }
    bool shaped() const override { return shaped_; }
    auto surface_if_any() const -> std::optional<mir::scene::Surface const*> override
    {
        return surface_;
    }

#ifdef MIR_VERSION_2_24_OR_GREATER
    auto opaque_region() const -> std::optional<mir::geometry::Rectangles> override
    {
        return opaque_region_;
    }
#endif

    glm::mat4 transformation_ = glm::mat4(1.f);
    float alpha_ = 1.f;
    bool shaped_ = false;
    mir::geometry::Rectangle screen_position_ {
        { 10,  20  },
        { 300, 400 }
    };
    mir::geometry::RectangleD src_bounds_ {
        { 1.0, 2.0 },
        { 3.0, 4.0 }
    };
    std::optional<mir::geometry::Rectangle> clip_area_ = mir::geometry::Rectangle {
        { 5, 6 },
        { 7, 8 }
    };
    std::optional<mir::scene::Surface const*> surface_ = reinterpret_cast<mir::scene::Surface const*>(0xdeadbeef);
#ifdef MIR_VERSION_2_24_OR_GREATER
    std::optional<mir::geometry::Rectangles> opaque_region_ = mir::geometry::Rectangles {
        { mir::geometry::Rectangle { { 0, 0 }, { 1, 1 } } }
    };
#endif
};

class StubSceneElement : public mir::compositor::SceneElement
{
public:
    explicit StubSceneElement(std::shared_ptr<StubRenderable> r) :
        stub { std::move(r) }
    {
    }

    std::shared_ptr<mir::graphics::Renderable> renderable() const override { return stub; }
    void rendered() override { ++rendered_count; }
    void occluded() override { ++occluded_count; }

    std::shared_ptr<StubRenderable> const stub;
    int rendered_count = 0;
    int occluded_count = 0;
};

class RecordingCompositor : public mir::compositor::DisplayBufferCompositor
{
public:
    bool composite(mir::compositor::SceneElementSequence&& sequence) override
    {
        captured = std::move(sequence);
        ++composite_count;
        return true;
    }

    mir::compositor::SceneElementSequence captured;
    int composite_count = 0;
};

struct SceneOverrideCompositorTest : testing::Test
{
    SceneOverrideCompositorTest()
    {
        auto recording = std::make_unique<RecordingCompositor>();
        wrapped = recording.get();
        compositor = std::make_unique<SceneOverrideDisplayBufferCompositor>(std::move(recording), state);
    }

    /// Sends a single element through the compositor and returns the renderable
    /// the wrapped compositor was ultimately handed.
    std::shared_ptr<mir::graphics::Renderable> composite_one()
    {
        mir::compositor::SceneElementSequence sequence { element };
        compositor->composite(std::move(sequence));
        return wrapped->captured.front()->renderable();
    }

    std::shared_ptr<CompositorState> const state = std::make_shared<CompositorState>();
    std::shared_ptr<StubRenderable> const renderable = std::make_shared<StubRenderable>();
    std::shared_ptr<StubSceneElement> const element = std::make_shared<StubSceneElement>(renderable);
    RecordingCompositor* wrapped = nullptr;
    std::unique_ptr<SceneOverrideDisplayBufferCompositor> compositor;
};
}

TEST_F(SceneOverrideCompositorTest, PassesElementsThroughUntouchedWhenNoOverrideIsActive)
{
    mir::compositor::SceneElementSequence sequence { element };
    ASSERT_TRUE(compositor->composite(std::move(sequence)));

    ASSERT_EQ(1, wrapped->composite_count);
    ASSERT_EQ(1u, wrapped->captured.size());
    ASSERT_EQ(element, wrapped->captured.front());
    ASSERT_EQ(glm::mat4(1.f), wrapped->captured.front()->renderable()->transformation());
}

TEST_F(SceneOverrideCompositorTest, ExemptsIdentityTransformedRenderablesWhileAnOverrideIsActive)
{
    state->scene_override_manager()->try_override(std::make_unique<StubSceneOverride>());

    ASSERT_NE(glm::mat4(1.f), composite_one()->transformation());
}

TEST_F(SceneOverrideCompositorTest, TheExemptTransformDoesNotMoveAnythingOnScreen)
{
    // The load-bearing property: Mir must think the renderable is transformed
    // while the transform is the identity on every vertex Miracle actually
    // draws. This mirrors the vertex shader, which pivots about a point whose z
    // and w are zero and whose vertices all have z == 0.
    for (auto const& center : {
             glm::vec2 { 0.f,    0.f   },
             glm::vec2 { 640.f,  360.f },
             glm::vec2 { -12.5f, 7.25f }
    })
    {
        for (auto const& position : {
                 glm::vec3 { 0.f,    0.f,    0.f },
                 glm::vec3 { 1920.f, 1080.f, 0.f },
                 glm::vec3 { -3.5f,  4.75f,  0.f }
        })
        {
            glm::vec4 const p { center, 0.f, 0.f };
            glm::vec4 const expected = (glm::mat4(1.f) * (glm::vec4(position, 1.f) - p)) + p;
            glm::vec4 const actual = (occlusion_exempt_transform * (glm::vec4(position, 1.f) - p)) + p;
            ASSERT_EQ(expected, actual);
        }
    }
}

TEST_F(SceneOverrideCompositorTest, LeavesAnAlreadyTransformedRenderableAlone)
{
    auto const scaled = glm::scale(glm::mat4(1.f), glm::vec3(0.5f, 0.5f, 1.f));
    renderable->transformation_ = scaled;
    state->scene_override_manager()->try_override(std::make_unique<StubSceneOverride>());

    ASSERT_EQ(scaled, composite_one()->transformation());
}

TEST_F(SceneOverrideCompositorTest, ForwardsEverythingButTheTransformation)
{
    renderable->alpha_ = 0.25f;
    renderable->shaped_ = true;
    state->scene_override_manager()->try_override(std::make_unique<StubSceneOverride>());

    auto const exempt = composite_one();
    ASSERT_EQ(renderable->id(), exempt->id());
    ASSERT_EQ(renderable->screen_position_, exempt->screen_position());
    ASSERT_EQ(renderable->src_bounds_, exempt->src_bounds());
    ASSERT_EQ(renderable->clip_area_, exempt->clip_area());
    ASSERT_EQ(0.25f, exempt->alpha());
    ASSERT_TRUE(exempt->shaped());
    ASSERT_EQ(renderable->surface_, exempt->surface_if_any());
    ASSERT_EQ(mir_orientation_left, exempt->orientation());
    ASSERT_EQ(mir_mirror_mode_horizontal, exempt->mirror_mode());
#ifdef MIR_VERSION_2_24_OR_GREATER
    ASSERT_EQ(renderable->opaque_region_, exempt->opaque_region());
#endif
}

TEST_F(SceneOverrideCompositorTest, ForwardsRenderedAndOccludedToTheWrappedElement)
{
    state->scene_override_manager()->try_override(std::make_unique<StubSceneOverride>());

    mir::compositor::SceneElementSequence sequence { element };
    compositor->composite(std::move(sequence));

    wrapped->captured.front()->rendered();
    wrapped->captured.front()->occluded();
    ASSERT_EQ(1, element->rendered_count);
    ASSERT_EQ(1, element->occluded_count);
}

TEST_F(SceneOverrideCompositorTest, StopsExemptingOnceTheOverrideIsReleased)
{
    auto const token = state->scene_override_manager()->try_override(std::make_unique<StubSceneOverride>());
    ASSERT_TRUE(token.has_value());
    ASSERT_NE(glm::mat4(1.f), composite_one()->transformation());

    ASSERT_TRUE(state->scene_override_manager()->try_release_override(token.value()));
    ASSERT_EQ(element, [&]
    {
        mir::compositor::SceneElementSequence sequence { element };
        compositor->composite(std::move(sequence));
        return wrapped->captured.front();
    }());
}
