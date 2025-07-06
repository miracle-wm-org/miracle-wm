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

#ifndef MIR_RENDERER_GL_RENDERER_H_
#define MIR_RENDERER_GL_RENDERER_H_

#include "primitive.h"
#include "program_factory.h"
#include "render_data_manager.h"

#include <GLES2/gl2.h>
#include <mir/geometry/rectangle.h>
#include <mir/graphics/renderable.h>
#include <mir/renderer/renderer.h>
#include <mir/version.h>
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
        std::shared_ptr<CompositorState> const& compositor_state);
    ~Renderer() override = default;

    // These are called with a valid GL context:
    void set_viewport(mir::geometry::Rectangle const& rect) override;
    void set_output_transform(glm::mat2 const&) override;
    auto render(mir::graphics::RenderableList const&) const -> std::unique_ptr<mir::graphics::Framebuffer> override;
#if (MIR_SERVER_MAJOR_VERSION > 2) || (MIR_SERVER_MAJOR_VERSION == 2 && MIR_SERVER_MINOR_VERSION >= 21)
    void set_output_filter(MirOutputFilter filter) override { }
#endif

    // This is called _without_ a GL context:
    void suspend() override;

private:
    /**
     * tessellate defines the list of triangles that will be used to render
     * the surface. By default it just returns 4 vertices for a rectangle.
     * However you can override its behaviour to tessellate more finely and
     * deform freely for effects like wobbly windows.
     *
     * \param [in,out] primitives The list of rendering primitives to be
     *                            grown and/or modified.
     * \param [in]     renderable The renderable surface being tessellated.
     *
     * \note The cohesion of this function to gl::Renderer is quite loose and it
     *       does not strictly need to reside here.
     *       However it seems a good choice under gl::Renderer while this remains
     *       the only OpenGL-specific class in the display server, and
     *       tessellation is very much OpenGL-specific.
     */
    static void tessellate(std::vector<mir::gl::Primitive>& primitives,
        mir::graphics::Renderable const& renderable);

    struct DrawData
    {
        bool enabled = false;
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
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

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

    DrawData get_draw_data(mir::graphics::Renderable const&, std::vector<RenderData> const& data) const;
    /// Draws the current renderable and returns a follow-up draw if required.
    void draw(mir::graphics::Renderable const& renderable, DrawData const& data) const;
    void draw_border(mir::scene::Surface const& surface, DrawData const& data) const;
    void update_gl_viewport();

    std::unique_ptr<mir::graphics::gl::OutputSurface> const output_surface;
    GLfloat clear_color[4];
    mutable long long frameno = 0;
    std::unique_ptr<ProgramFactory> const program_factory;
    mir::geometry::Rectangle viewport;
    glm::mat4 screen_to_gl_coords;
    glm::mat4 display_transform;
    double x_scale = 1.f;
    double y_scale = 1.f;
    std::vector<mir::gl::Primitive> mutable primitives;
    std::shared_ptr<mir::graphics::GLRenderingProvider> const gl_interface;
    mutable Mesh border_model;
    std::shared_ptr<Config> config;
    std::shared_ptr<CompositorState> compositor_state;
};

}

#endif // MIR_RENDERER_GL_RENDERER_H_
