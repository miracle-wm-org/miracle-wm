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

#include <GLES2/gl2.h>
#include <mir/geometry/rectangle.h>
#include <mir/graphics/renderable.h>
#include <mir/renderer/renderer.h>
#include <miral/window_manager_tools.h>
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
        bool const is_flipped);

    struct DrawData
    {
        bool enabled = false;
        float alpha = 1.f;
        RenderData data;
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

        ~PassTarget();
        /// Ensures the target is allocated at least as large as `requested`.
        /// Reallocates only when requested exceeds the current size.
        void ensure(mir::geometry::Size requested);
    };

    /// Finds the [RenderData] tracked for \p surface in [render_data_cache], if any.
    RenderData const* find_render_data(mir::scene::Surface const* surface) const;
    DrawData get_draw_data(mir::graphics::Renderable const&,
        mir::scene::Surface const* surface,
        RenderData const* tracked) const;
    /// Draws the current renderable and returns a follow-up draw if required.
    void draw(mir::graphics::Renderable const& renderable, DrawData const& data) const;
    void draw_border(mir::scene::Surface const& surface, DrawData const& data) const;
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

    GLfloat clear_color[4];
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
};

}

#endif // MIR_RENDERER_GL_RENDERER_H_
