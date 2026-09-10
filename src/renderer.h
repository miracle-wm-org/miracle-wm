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

#ifndef MIR_RENDERER_GL_RENDERER_H_
#define MIR_RENDERER_GL_RENDERER_H_

#include "mir_version_manager.h"
#include "primitive.h"
#include "program_factory.h"
#include "render_data_manager.h"
#include "scene_override.h"
#include "tessellation_helpers.h"

#include <GLES2/gl2.h>
#include <memory>
#include <mir/geometry/displacement.h>
#include <mir/geometry/rectangle.h>
#include <mir/graphics/renderable.h>
#include <mir/renderer/renderer.h>
#include <miral/window_manager_tools.h>
#include <unordered_map>
#include <vector>

namespace mir
{
namespace graphics
{
    class GLRenderingProvider;
}
namespace graphics::gl
{
    class OutputSurface;
    class Texture;
}
}

namespace miracle
{
class Config;
class CompositorState;
class WindowToolsAccessor;
class Animator;

class Renderer : public mir::renderer::Renderer
{
public:
    Renderer(std::shared_ptr<mir::graphics::GLRenderingProvider> gl_interface,
        std::unique_ptr<mir::graphics::gl::OutputSurface> output,
        std::shared_ptr<Config> const& config,
        std::shared_ptr<CompositorState> const& compositor_state,
        std::shared_ptr<SamplerRegistry> const& sampler_registry);
    ~Renderer() override;

    // These are called with a valid GL context:
    void set_viewport(mir::geometry::Rectangle const& rect) override;
    void set_output_transform(glm::mat2 const&) override;
    auto render(mir::graphics::RenderableList const&) const -> std::unique_ptr<mir::graphics::Framebuffer> override;
    void set_output_filter(MirOutputFilter filter) override;

    // This is called _without_ a GL context:
    void suspend() override;

private:
    static void tessellate(std::vector<mir::gl::Primitive>& primitives,
        mir::graphics::Renderable const& renderable,
        bool const is_flipped,
        std::optional<mir::geometry::Rectangle> const& clip_area,
        std::optional<mir::gl::Stretch> const& stretch = std::nullopt);

    struct DrawData
    {
        bool enabled = false;
        float alpha = 1.f;
        RenderData data;
        /// When set, the scene override wants the surface drawn at this
        /// placement instead of its real position.
        std::optional<SceneOverridePlacement> placement;
        /// The real screen position of the surface group; only valid when
        /// [placement] is set. Used to derive the mapping onto the placement.
        mir::geometry::Rectangle override_real;
        /// The surface the renderable belongs to, if any.
        mir::scene::Surface const* surface = nullptr;
        /// The window rectangle the surface group's committed buffer corresponds to; see
        /// committed_window_rect. A stretch maps this onto the animated size, which is what
        /// lands the content on the rectangle draw_border is sized from by construction.
        /// Shared across the group so a subsurface is scaled by its parent window's factor
        /// rather than by its own size. Only meaningful when [surface] is set.
        mir::geometry::Rectangle group_natural;
    };

    struct Vertex
    {
        glm::vec3 position;
        glm::vec2 texcoord;
    };

    class Mesh
    {
    public:
        Mesh(std::vector<Vertex>&& vertices, std::vector<unsigned int>&& indices) :
            vertices(std::move(vertices)),
            indices(std::move(indices))
        {
        }

        static Mesh rectangle(glm::vec3 position, glm::vec2 size)
        {
            glm::vec3 bottomLeft = position;
            glm::vec3 bottomRight = position + glm::vec3(size.x, 0.f, 0.f);
            glm::vec3 topLeft = position + glm::vec3(0.0f, size.y, 0.f);
            glm::vec3 topRight = position + glm::vec3(size.x, size.y, 0.f);
            std::vector<Vertex> vertices = {
                { bottomLeft,  glm::vec2(0.0f, 0.0f) },
                { bottomRight, glm::vec2(1,    0)    },
                { topRight,    glm::vec2(1,    1)    },
                { topLeft,     glm::vec2(0,    1)    }
            };

            std::vector<unsigned int> indices = {
                0, 1, 2, // First triangle
                2, 3, 0 // Second triangle
            };

            return Mesh(std::move(vertices), std::move(indices));
        }

        void upload_to_gpu()
        {
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);

            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)), indices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }

        void destroy()
        {
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ebo);
        }

        std::vector<Vertex> const vertices;
        std::vector<unsigned int> const indices;

        GLuint vbo = 0;
        GLuint ebo = 0;
    };

    /// Offscreen framebuffer target for intermediate rendering passes.
    struct PassTarget
    {
        mir::geometry::Size size = { 0, 0 };
        GLuint texture_id = 0;
        GLuint framebuffer_id = 0;

        PassTarget() = default;
        /// Owns its GL objects outright: copying one would delete them twice.
        PassTarget(PassTarget const&) = delete;
        PassTarget& operator=(PassTarget const&) = delete;
        ~PassTarget();
        /// Ensures the target is allocated at least as large as `requested`.
        /// Reallocates only when requested exceeds the current size. \p filter is the
        /// sampling mode, which only takes effect on a (re)allocation: NEAREST for a pass
        /// chain that samples texel-for-texel, LINEAR for anything that gets scaled.
        void ensure(mir::geometry::Size requested, GLenum filter = GL_NEAREST);
    };

    /// The content a window was showing when its resize animation began, kept so the live
    /// surface can be cross-faded in over it on the frame the client swaps buffers - the
    /// frame where the content cuts.
    ///
    /// An offscreen copy rather than the client's own buffer: holding a buffer across frames
    /// delays wl_buffer.release and stalls a double-buffered client. The copy has to be taken
    /// eagerly, on the first animated frame, because by the frame we can *detect* the swap
    /// the content we wanted is already gone.
    struct ResizeGhost
    {
        PassTarget target;
        /// The renderable's rectangle at capture, so the ghost is mapped by the same rule
        /// the live layer is rather than by the live surface's since-changed geometry.
        mir::geometry::Rectangle screen_position;
        /// The window rectangle that content corresponded to; see committed_window_rect.
        mir::geometry::Rectangle source;
        /// The buffer size the capture was taken from. A change is the client swapping,
        /// which is what starts the fade.
        mir::geometry::Size captured_buffer_size;
        /// The animation progress at the swap, or nothing while it has not happened.
        std::optional<float> progress_at_swap;
        bool shaped = false;
        bool captured = false;
        /// The window this belongs to, so [prune_retained_state] can reach it.
        RenderDataManagerId owner = 0;
    };

    /// Finds the [RenderData] tracked for \p surface in [render_data_cache], if any.
    RenderData const* find_render_data(mir::scene::Surface const* surface) const;
    DrawData get_draw_data(mir::graphics::Renderable const&,
        RenderData const* tracked,
        std::optional<SceneOverridePlacement> const& placement,
        mir::geometry::Rectangle const& placement_real,
        mir::geometry::Rectangle const& group_natural) const;
    /// Draws the current renderable and returns a follow-up draw if required.
    void draw(mir::graphics::Renderable const& renderable, DrawData const& data) const;

    /// Uploads the uniforms that describe the layer being drawn - texture, alpha, and the
    /// size the rounded-corner SDF measures against - and draws \p primitive. Everything
    /// else is uploaded once by draw() before this is called.
    ///
    /// \p offscreen_result_target is the index into [pass_targets] holding the result of a
    /// multi-pass shader chain, or -1 when the texture is drawn directly.
    /// \p texture_override, when non-zero, is bound on unit 0 in place of \p texture: a
    /// cross-fade's lower layer is an offscreen copy rather than anything Mir owns.
    void draw_layer(
        ProgramData const& prog,
        mir::gl::Primitive const& primitive,
        mir::graphics::gl::Texture& texture,
        glm::vec2 const& surface_size,
        float alpha,
        bool shaped,
        int offscreen_result_target,
        GLuint texture_override = 0) const;

    /// Copies \p renderable's current content into \p ghost's offscreen target, at its own
    /// size and with none of the screen's placement, rotation or rounding - those are applied
    /// when the ghost is drawn, so it goes through the very same map the live layer does.
    ///
    /// Leaves the program's screen-global uniforms dirty; the caller re-uploads them.
    void capture_ghost(
        ProgramData const& prog,
        mir::graphics::gl::Texture& texture,
        mir::graphics::Renderable const& renderable,
        DrawData const& data,
        ResizeGhost& ghost) const;
    void draw_border(mir::scene::Surface const& surface, DrawData const& data) const;
    /// Drops the retained state of windows that have gone away. Checked against
    /// [render_data_cache] rather than a per-frame list of what was drawn, so a window
    /// culled for a frame keeps what has already been measured for it.
    void prune_retained_state() const;

    /// How far \p presented overshoots the window it belongs to, for the window \p data
    /// tracks; see [Inset]. Updates the retained value as a side effect, which is why it is
    /// called exactly once per surface group per frame.
    auto learn_inset(
        RenderData const& data,
        mir::geometry::Size const& presented,
        mir::geometry::Size const& window_size) const -> mir::geometry::Displacement;
    void update_gl_viewport();

    /// Runs intermediate off-screen passes 0 .. pass_count-2 for a multi-pass
    /// custom shader, leaving the result in pass_targets[last_target].
    /// Returns the index of the ping-pong target that holds the final result.
    int run_offscreen_passes(
        mir::graphics::gl::Texture& texture,
        uint8_t shader_id,
        size_t pass_count,
        mir::geometry::Size buf_size,
        bool source_is_top_row_first) const;

    class OutputFilter;
    std::unique_ptr<OutputFilter> const output_surface;
    mutable PassTarget pass_targets[2];

    mutable long long frameno = 0;
    std::unique_ptr<ProgramFactory> const program_factory;
    mir::geometry::Rectangle viewport;
    glm::mat4 screen_to_gl_coords;
    glm::mat4 display_transform;
    enum class OutputRotation
    {
        normal,
        left_90,
        inverted_180,
        right_270
    };
    OutputRotation output_rotation = OutputRotation::normal;
    double x_scale = 1.f;
    double y_scale = 1.f;
    std::vector<mir::gl::Primitive> mutable primitives;
    std::shared_ptr<mir::graphics::GLRenderingProvider> const gl_interface;
    mutable Mesh border_model;
    std::shared_ptr<Config> config;
    std::shared_ptr<CompositorState> compositor_state;
    std::shared_ptr<SamplerRegistry> sampler_registry;
    /// Per-renderer copy of the shared render data, refreshed in render()
    /// only when the manager's generation has advanced since the last frame.
    mutable std::vector<RenderData> render_data_cache;
    mutable uint64_t render_data_generation = 0;
    /// Scratch buffer that [SceneOverride::place] fills, reused between frames
    /// so that asking for placements costs no allocation in the steady state.
    mutable std::vector<SceneOverridePlacement> group_placements;
    /// How far a client's committed buffer overshoots the window rectangle it was drawn
    /// for: positive for a CSD client that draws its own drop shadow past its xdg window
    /// geometry, negative for a server-decorated one whose buffer is only the content inside
    /// its frame, zero for everything else.
    ///
    /// This is a property of the client, not of any animation, so it is learned once and
    /// kept. It cannot be read off a frame during a resize - the surface's size is what the
    /// compositor asked for and the buffer is what the client last drew, so their difference
    /// is the animation delta rather than the shadow - and the frame immediately after an
    /// animation ends is still such a frame. Hence [last_presented]: a measurement is only
    /// taken once the buffer has held still for two frames.
    struct Inset
    {
        mir::geometry::Displacement value;
        /// The buffer size seen on the previous frame, or nothing on the first.
        std::optional<mir::geometry::Size> last_presented;
        /// Whether [value] has ever been established. Until it has, an animation seeds it
        /// from the pre-resize size it carries rather than leaving the shadow unaccounted
        /// for - which would stretch the shadow across the whole tile.
        bool known = false;
    };

    mutable std::unordered_map<RenderDataManagerId, Inset> insets;

    /// The retained pre-resize content of every layer currently animating, keyed by
    /// renderable so a window drawing more than one buffer layer fades each against its own
    /// capture rather than all of them against the first's.
    mutable std::unordered_map<mir::graphics::Renderable::ID, ResizeGhost> ghosts;
};

}

#endif // MIR_RENDERER_GL_RENDERER_H_
