/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_RENDERER_GL_RENDERER_H_
#define MIR_RENDERER_GL_RENDERER_H_

#include <mir/renderer/renderer.h>
#include <mir/geometry/rectangle.h>
#include <mir/graphics/buffer_id.h>
#include <mir/graphics/renderable.h>
#include "primitive.h"

#include <GLES2/gl2.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mir
{
namespace graphics { class GLRenderingProvider; }
namespace graphics::gl { class OutputSurface; }
namespace renderer
{
namespace gl
{

class Renderer : public renderer::Renderer
{
public:
    Renderer(std::shared_ptr<graphics::GLRenderingProvider> gl_interface, std::unique_ptr<graphics::gl::OutputSurface> output);
    virtual ~Renderer();

    // These are called with a valid GL context:
    void set_viewport(geometry::Rectangle const& rect) override;
    void set_output_transform(glm::mat2 const&) override;
    auto render(graphics::RenderableList const&) const -> std::unique_ptr<graphics::Framebuffer> override;

    // This is called _without_ a GL context:
    void suspend() override;

    struct Program
    {
        GLuint id = 0;
        /* 8 is the minimum number of texture units a GL implementation can provide
         * and should comfortably provide enough textures for any conceivable buffer
         * format
         */
        std::array<GLint, 8> tex_uniforms;
        GLint position_attr = -1;
        GLint texcoord_attr = -1;
        GLint centre_uniform = -1;
        GLint display_transform_uniform = -1;
        GLint transform_uniform = -1;
        GLint screen_to_gl_coords_uniform = -1;
        GLint alpha_uniform = -1;
        GLint outline_color_uniform = -1;
        mutable long long last_used_frameno = 0;

        Program(GLuint program_id);
    };
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
    virtual void tessellate(std::vector<mir::gl::Primitive>& primitives,
                            graphics::Renderable const& renderable) const;

    struct OutlineContext
    {
        glm::vec4 color;
    };
    virtual void draw(graphics::Renderable const& renderable, OutlineContext* context = nullptr) const;
    void update_gl_viewport();

    std::unique_ptr<graphics::gl::OutputSurface> const output_surface;
    GLfloat clear_color[4];
    mutable long long frameno = 0;
    class ProgramFactory;
    std::unique_ptr<ProgramFactory> const program_factory;
    geometry::Rectangle viewport;
    glm::mat4 screen_to_gl_coords;
    glm::mat4 display_transform;
    std::vector<mir::gl::Primitive> mutable primitives;
    std::shared_ptr<graphics::GLRenderingProvider> const gl_interface;
};

}
}
}

#endif // MIR_RENDERER_GL_RENDERER_H_
