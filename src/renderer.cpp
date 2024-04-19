/* Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define MIR_LOG_COMPONENT "GLRenderer"

#include "renderer.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/display_sink.h"
#include "tessellation_helpers.h"
#include "mir/log.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/texture.h"
#include "mir/graphics/program_factory.h"
#include "mir/graphics/program.h"
#include "mir/renderer/gl/gl_surface.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <EGL/egl.h>

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <mutex>

namespace mg = mir::graphics;
namespace mgl = mir::gl;
namespace mrg = mir::renderer::gl;
namespace geom = mir::geometry;

namespace
{
template<void (* deleter)(GLuint)>
class GLHandle
{
public:
    explicit GLHandle(GLuint id)
    : id{id}
    {
    }

    ~GLHandle()
    {
        if (id)
            (*deleter)(id);
    }

    GLHandle(GLHandle const&) = delete;

    GLHandle& operator=(GLHandle const&) = delete;

    GLHandle(GLHandle&& from)
    : id{from.id}
    {
        from.id = 0;
    }

    operator GLuint() const
    {
        return id;
    }

private:
    GLuint id;
};

using ProgramHandle = GLHandle<&glDeleteProgram>;
using ShaderHandle = GLHandle<&glDeleteShader>;

struct Program : public mir::graphics::gl::Program
{
public:
    Program(ProgramHandle&& opaque_shader, ProgramHandle&& alpha_shader)
    : opaque_handle(std::move(opaque_shader)),
    alpha_handle(std::move(alpha_shader)),
    opaque{opaque_handle},
    alpha{alpha_handle}
    {
    }

    ProgramHandle opaque_handle, alpha_handle;
    mir::renderer::gl::Renderer::Program opaque, alpha;
};

const GLchar* const vertex_shader_src =
{
"attribute vec3 position;\n"
"attribute vec2 texcoord;\n"
"uniform mat4 screen_to_gl_coords;\n"
"uniform mat4 display_transform;\n"
"uniform mat4 transform;\n"
"uniform vec2 centre;\n"
"varying vec2 v_texcoord;\n"
"void main() {\n"
"   vec4 mid = vec4(centre, 0.0, 0.0);\n"
"   vec4 transformed = (transform * (vec4(position, 1.0) - mid)) + mid;\n"
"   gl_Position = display_transform * screen_to_gl_coords * transformed;\n"
"   v_texcoord = texcoord;\n"
"}\n"
};
}

class mrg::Renderer::ProgramFactory : public mir::graphics::gl::ProgramFactory
{
public:
    // NOTE: This must be called with a current GL context
    ProgramFactory()
    : vertex_shader{compile_shader(GL_VERTEX_SHADER, vertex_shader_src)}
    {
    }

    mir::graphics::gl::Program&
    compile_fragment_shader(
    void const* id,
    char const* extension_fragment,
    char const* fragment_fragment) override
    {
        /* NOTE: This does not lock the programs vector as there is one ProgramFactory instance
         * per rendering thread.
         */

        for (auto const& pair : programs)
        {
            if (pair.first == id)
            {
                return *pair.second;
            }
        }

        std::stringstream opaque_fragment;
        opaque_fragment
        << extension_fragment
        << "\n"
        <<
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        << "\n"
        << fragment_fragment
        << "\n"
        <<
        "varying vec2 v_texcoord;\n"
        "void main() {\n"
        "    gl_FragColor = sample_to_rgba(v_texcoord);\n"
        "}\n";

        std::stringstream alpha_fragment;
        alpha_fragment
        << extension_fragment
        << "\n"
        <<
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        << "\n"
        << fragment_fragment
        << "\n"
        <<
        "varying vec2 v_texcoord;\n"
        "uniform float alpha;\n"
        "void main() {\n"
        "    gl_FragColor = alpha * sample_to_rgba(v_texcoord);\n"
        "}\n";

        // GL shader compilation is *not* threadsafe, and requires external synchronisation
        std::lock_guard lock{compilation_mutex};

        ShaderHandle const opaque_shader{
        compile_shader(GL_FRAGMENT_SHADER, opaque_fragment.str().c_str())};
        ShaderHandle const alpha_shader{
        compile_shader(GL_FRAGMENT_SHADER, alpha_fragment.str().c_str())};

        programs.emplace_back(id, std::make_unique<::Program>(
        link_shader(vertex_shader, opaque_shader),
        link_shader(vertex_shader, alpha_shader)));

        return *programs.back().second;

        // We delete opaque_shader and alpha_shader here. This is fine; it only marks them
        // for deletion. GL will only delete them once the GL Program they're linked in is destroyed.
    }

private:
    static GLuint compile_shader(GLenum type, GLchar const* src)
    {
        GLuint id = glCreateShader(type);
        if (!id)
        {
            BOOST_THROW_EXCEPTION(mg::gl_error("Failed to create shader"));
        }

        glShaderSource(id, 1, &src, NULL);
        glCompileShader(id);
        GLint ok;
        glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            GLchar log[1024] = "(No log info)";
            glGetShaderInfoLog(id, sizeof log, NULL, log);
            glDeleteShader(id);
            BOOST_THROW_EXCEPTION(
            std::runtime_error(
            std::string("Compile failed: ") + log + " for:\n" + src));
        }
        return id;
    }

    static ProgramHandle link_shader(
    ShaderHandle const& vertex_shader,
    ShaderHandle const& fragment_shader)
    {
        ProgramHandle program{glCreateProgram()};
        glAttachShader(program, fragment_shader);
        glAttachShader(program, vertex_shader);
        glLinkProgram(program);
        GLint ok;
        glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            GLchar log[1024];
            glGetProgramInfoLog(program, sizeof log - 1, NULL, log);
            log[sizeof log - 1] = '\0';
            BOOST_THROW_EXCEPTION(
            std::runtime_error(
            std::string("Linking GL shader failed: ") + log));
        }

        return program;
    }

    ShaderHandle const vertex_shader;
    std::vector<std::pair<void const*, std::unique_ptr<::Program>>> programs;
    // GL requires us to synchronise multi-threaded access to the shader APIs.
    std::mutex compilation_mutex;
};

mrg::Renderer::Program::Program(GLuint program_id)
{
    id = program_id;
    position_attr = glGetAttribLocation(id, "position");
    texcoord_attr = glGetAttribLocation(id, "texcoord");
    for (auto i = 0u; i < tex_uniforms.size() ; ++i)
    {
        /* You can reference uniform arrays as tex[0], tex[1], tex[2], … until you
         * hit the end of the array, which will return -1 as the location.
         */
        auto const uniform_name = std::string{"tex["} + std::to_string(i) + "]";
        tex_uniforms[i] = glGetUniformLocation(id, uniform_name.c_str());
    }
    centre_uniform = glGetUniformLocation(id, "centre");
    display_transform_uniform = glGetUniformLocation(id, "display_transform");
    transform_uniform = glGetUniformLocation(id, "transform");
    screen_to_gl_coords_uniform = glGetUniformLocation(id, "screen_to_gl_coords");
    alpha_uniform = glGetUniformLocation(id, "alpha");
}

namespace
{
auto make_output_current(std::unique_ptr<mg::gl::OutputSurface> output) -> std::unique_ptr<mg::gl::OutputSurface>
{
    output->make_current();
    return output;
}
}

mrg::Renderer::Renderer(
std::shared_ptr<graphics::GLRenderingProvider> gl_interface,
std::unique_ptr<graphics::gl::OutputSurface> output)
: output_surface{make_output_current(std::move(output))},
clear_color{0.0f, 0.0f, 0.0f, 1.0f},
program_factory{std::make_unique<ProgramFactory>()},
display_transform(1),
gl_interface{std::move(gl_interface)}
{
    eglBindAPI(EGL_OPENGL_ES_API);
    EGLDisplay disp = eglGetCurrentDisplay();
    if (disp != EGL_NO_DISPLAY)
    {
        struct {GLint id; char const* label;} const eglstrings[] =
        {
        {EGL_VENDOR,      "EGL vendor"},
        {EGL_VERSION,     "EGL version"},
        {EGL_CLIENT_APIS, "EGL client APIs"},
        {EGL_EXTENSIONS,  "EGL extensions"},
        };
        for (auto& s : eglstrings)
        {
            auto val = eglQueryString(disp, s.id);
            mir::log_info(std::string(s.label) + ": " + (val ? val : ""));
        }
    }

    struct {GLenum id; char const* label;} const glstrings[] =
    {
    {GL_VENDOR,   "GL vendor"},
    {GL_RENDERER, "GL renderer"},
    {GL_VERSION,  "GL version"},
    {GL_SHADING_LANGUAGE_VERSION,  "GLSL version"},
    {GL_EXTENSIONS, "GL extensions"},
    };

    for (auto& s : glstrings)
    {
        auto val = reinterpret_cast<char const*>(glGetString(s.id));
        mir::log_info(std::string(s.label) + ": " + (val ? val : ""));
    }

    GLint max_texture_size = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    mir::log_info("GL max texture size = %d", max_texture_size);

    GLint rbits = 0, gbits = 0, bbits = 0, abits = 0, dbits = 0, sbits = 0;
    glGetIntegerv(GL_RED_BITS, &rbits);
    glGetIntegerv(GL_GREEN_BITS, &gbits);
    glGetIntegerv(GL_BLUE_BITS, &bbits);
    glGetIntegerv(GL_ALPHA_BITS, &abits);
    glGetIntegerv(GL_DEPTH_BITS, &dbits);
    glGetIntegerv(GL_STENCIL_BITS, &sbits);
    mir::log_info("GL framebuffer bits: RGBA=%d%d%d%d, depth=%d, stencil=%d",
                  rbits, gbits, bbits, abits, dbits, sbits);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

mrg::Renderer::~Renderer()
{
}

void mrg::Renderer::tessellate(std::vector<mgl::Primitive>& primitives,
                               mg::Renderable const& renderable) const
{
    primitives.resize(1);
    primitives[0] = mgl::tessellate_renderable_into_rectangle(renderable, geom::Displacement{0,0});
}

auto mrg::Renderer::render(mg::RenderableList const& renderables) const -> std::unique_ptr<mg::Framebuffer>
{
    output_surface->make_current();
    output_surface->bind();

    glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT);

    ++frameno;
    for (auto const& r : renderables)
    {
        draw(*r);
    }

    auto output = output_surface->commit();

    // Report any GL errors after commit, to catch any *during* commit
    while (auto const gl_error = glGetError())
        mir::log_debug("GL error: %d", gl_error);

    return output;
}

void mrg::Renderer::draw(mg::Renderable const& renderable) const
{
    auto const texture = gl_interface->as_texture(renderable.buffer());
    auto const clip_area = renderable.clip_area();
    if (clip_area)
    {
        glEnable(GL_SCISSOR_TEST);
        auto clip_x = clip_area.value().top_left.x.as_int();
        // The Y-coordinate is always relative to the top, so we make it relative to the bottom.
        auto clip_y = viewport.top_left.y.as_int() +
                      viewport.size.height.as_int() -
                      clip_area.value().top_left.y.as_int() -
                      clip_area.value().size.height.as_int();
        glm::vec4 clip_pos(clip_x, clip_y, 0, 1);
        clip_pos = display_transform * clip_pos;

        glScissor(
        (int)clip_pos.x - viewport.top_left.x.as_int(),
        (int)clip_pos.y,
        clip_area.value().size.width.as_int(),
        clip_area.value().size.height.as_int()
        );
    }

    // All the programs are held by program_factory through its lifetime. Using pointers avoids
    // -Wdangling-reference.
    auto const* const prog =
    [this, &texture](bool alpha) -> Program const*
    {
        auto const& family = static_cast<::Program const&>(texture->shader(*program_factory));
        if (alpha)
        {
            return &family.alpha;
        }
        return &family.opaque;
    }(renderable.alpha() < 1.0f);

    glUseProgram(prog->id);
    if (prog->last_used_frameno != frameno)
    {   // Avoid reloading the screen-global uniforms on every renderable
        // TODO: We actually only need to bind these *once*, right? Not once per frame?
        prog->last_used_frameno = frameno;
        for (auto i = 0u; i < prog->tex_uniforms.size(); ++i)
        {
            if (prog->tex_uniforms[i] != -1)
            {
                glUniform1i(prog->tex_uniforms[i], i);
            }
        }
        glUniformMatrix4fv(prog->display_transform_uniform, 1, GL_FALSE,
                           glm::value_ptr(display_transform));
        glUniformMatrix4fv(prog->screen_to_gl_coords_uniform, 1, GL_FALSE,
                           glm::value_ptr(screen_to_gl_coords));
    }

    glActiveTexture(GL_TEXTURE0);

    auto const& rect = renderable.screen_position();
    GLfloat centrex = rect.top_left.x.as_int() +
                      rect.size.width.as_int() / 2.0f;
    GLfloat centrey = rect.top_left.y.as_int() +
                      rect.size.height.as_int() / 2.0f;
    glUniform2f(prog->centre_uniform, centrex, centrey);

    glm::mat4 transform = renderable.transformation();
    if (texture->layout() == mg::gl::Texture::Layout::TopRowFirst)
    {
        // GL textures have (0,0) at bottom-left rather than top-left
        // We have to invert this texture to get it the way up GL expects.
        transform *= glm::mat4{
        1.0, 0.0, 0.0, 0.0,
        0.0, -1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0
        };
    }

    glUniformMatrix4fv(prog->transform_uniform, 1, GL_FALSE,
                       glm::value_ptr(transform));

    if (prog->alpha_uniform >= 0)
        glUniform1f(prog->alpha_uniform, renderable.alpha());

    glEnableVertexAttribArray(prog->position_attr);
    glEnableVertexAttribArray(prog->texcoord_attr);

    primitives.clear();
    tessellate(primitives, renderable);

    // if we fail to load the texture, we need to carry on (part of lp:1629275)
    try
    {
        typedef struct  // Represents parameters of glBlendFuncSeparate()
        {
            GLenum src_rgb, dst_rgb, src_alpha, dst_alpha;
        } BlendSeparate;

        BlendSeparate client_blend;

        // These renderable method names could be better (see LP: #1236224)
        if (renderable.shaped())  // Client is RGBA:
        {
            client_blend = {GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
                            GL_ONE, GL_ONE_MINUS_SRC_ALPHA};
        }
        else if (renderable.alpha() == 1.0f)  // RGBX and no window translucency:
        {
            client_blend = {GL_ONE,  GL_ZERO,
                            GL_ZERO, GL_ONE};  // Avoid using src_alpha!
        }
        else
        {   // Client is RGBX but we also have window translucency.
            // The texture alpha channel is possibly uninitialized so we must be
            // careful and avoid using SRC_ALPHA (LP: #1423462).
            client_blend = {GL_ONE,  GL_ONE_MINUS_CONSTANT_ALPHA,
                            GL_ZERO, GL_ONE};
            glBlendColor(0.0f, 0.0f, 0.0f, renderable.alpha());
        }

        for (auto const& p : primitives)
        {
            BlendSeparate blend;

            blend = client_blend;
            texture->bind();

            glVertexAttribPointer(prog->position_attr, 3, GL_FLOAT,
                                  GL_FALSE, sizeof(mgl::Vertex),
                                  &p.vertices[0].position);
            glVertexAttribPointer(prog->texcoord_attr, 2, GL_FLOAT,
                                  GL_FALSE, sizeof(mgl::Vertex),
                                  &p.vertices[0].texcoord);

            if (blend.dst_rgb == GL_ZERO)
            {
                glDisable(GL_BLEND);
            }
            else
            {
                glEnable(GL_BLEND);
                glBlendFuncSeparate(blend.src_rgb,   blend.dst_rgb,
                                    blend.src_alpha, blend.dst_alpha);
            }

            glDrawArrays(p.type, 0, p.nvertices);

            // We're done with the texture for now
            texture->add_syncpoint();
        }
    }
    catch (std::exception const& ex)
    {
    }

    glDisableVertexAttribArray(prog->texcoord_attr);
    glDisableVertexAttribArray(prog->position_attr);
    if (renderable.clip_area())
    {
        glDisable(GL_SCISSOR_TEST);
    }
}

void mrg::Renderer::set_viewport(geometry::Rectangle const& rect)
{
    if (rect == viewport)
        return;

    /*
     * Here we provide a 3D perspective projection with a default 30 degrees
     * vertical field of view. This projection matrix is carefully designed
     * such that any vertices at depth z=0 will fit the screen coordinates. So
     * client texels will fit screen pixels perfectly as long as the surface is
     * at depth zero. But if you want to do anything fancy, you can also choose
     * a different depth and it will appear to come out of or go into the
     * screen.
     */
    screen_to_gl_coords = glm::translate(glm::mat4(1.0f), glm::vec3{-1.0f, 1.0f, 0.0f});

    /*
     * Perspective division is one thing that can't be done in a matrix
     * multiplication. It happens after the matrix multiplications. GL just
     * scales {x,y} by 1/w. So modify the final part of the projection matrix
     * to set w ([3]) to be the incoming z coordinate ([2]).
     */
    screen_to_gl_coords[2][3] = -1.0f;

    float const vertical_fov_degrees = 30.0f;
    float const near =
    (rect.size.height.as_int() / 2.0f) /
    std::tan((vertical_fov_degrees * M_PI / 180.0f) / 2.0f);
    float const far = -near;

    screen_to_gl_coords = glm::scale(screen_to_gl_coords,
                                     glm::vec3{2.0f / rect.size.width.as_int(),
                                               -2.0f / rect.size.height.as_int(),
                                               2.0f / (near - far)});
    screen_to_gl_coords = glm::translate(screen_to_gl_coords,
                                         glm::vec3{-rect.top_left.x.as_int(),
                                                   -rect.top_left.y.as_int(),
                                                   0.0f});

    viewport = rect;
    update_gl_viewport();
}

void mrg::Renderer::update_gl_viewport()
{
    /*
     * Letterboxing: Move the glViewport to add black bars in the case that
     * the logical viewport aspect ratio doesn't match the display aspect.
     * This keeps pixels square. Note "black"-bars are really glClearColor.
     */
    auto transformed_viewport = display_transform *
                                glm::vec4(viewport.size.width.as_int(),
                                          viewport.size.height.as_int(), 0, 1);
    auto viewport_width = fabs(transformed_viewport[0]);
    auto viewport_height = fabs(transformed_viewport[1]);

    auto const output_size = output_surface->size();
    auto const output_width = output_size.width.as_value();
    auto const output_height = output_size.height.as_value();

    if (viewport_width > 0.0f && viewport_height > 0.0f &&
        output_width > 0 && output_height > 0)
    {
        GLint reduced_width = output_width, reduced_height = output_height;
        // if viewport_aspect_ratio >= output_aspect_ratio
        if (viewport_width * output_height >= output_width * viewport_height)
            reduced_height = output_width * viewport_height / viewport_width;
        else
            reduced_width = output_height * viewport_width / viewport_height;

        GLint offset_x = (output_width - reduced_width) / 2;
        GLint offset_y = (output_height - reduced_height) / 2;

        glViewport(offset_x, offset_y, reduced_width, reduced_height);
    }
}

void mrg::Renderer::set_output_transform(glm::mat2 const& t)
{
    auto new_display_transform = glm::mat4(t);

    switch (output_surface->layout())
    {
        case graphics::gl::OutputSurface::Layout::GL:
            break;
        case graphics::gl::OutputSurface::Layout::TopRowFirst:
            // GL is going to render in its own coordinate system, but the OutputSurface
            // wants the output to be the other way up. Get GL to render upside-down instead.
            new_display_transform = glm::mat4{
            1.0, 0.0, 0.0, 0.0,
            0.0, -1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0
            } * new_display_transform;
            break;
    }

    if (new_display_transform != display_transform)
    {
        display_transform = new_display_transform;
        update_gl_viewport();
    }
}

void mrg::Renderer::suspend()
{
    output_surface->release_current();
}
