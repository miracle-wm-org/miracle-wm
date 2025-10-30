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

#define GLM_FORCE_RADIANS
#define MIR_LOG_COMPONENT "GLRenderer"

#include "renderer.h"
#include "compositor_state.h"
#include "config.h"
#include "math_helpers.h"
#include "program_factory.h"
#include "tessellation_helpers.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <mir/graphics/buffer.h>
#include <mir/graphics/display_sink.h>
#include <mir/graphics/platform.h>
#include <mir/graphics/program_factory.h>
#include <mir/graphics/renderable.h>
#include <mir/graphics/texture.h>
#include <mir/log.h>
#include <mir/renderer/gl/gl_surface.h>
#include <mir/scene/surface.h>
#include <stdexcept>

namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace mgl = mir::gl;
namespace geom = mir::geometry;
using namespace miracle;

namespace
{
auto make_output_current(std::unique_ptr<mg::gl::OutputSurface> output) -> std::unique_ptr<mg::gl::OutputSurface>
{
    output->make_current();
    return output;
}

template <void (*deleter)(GLsizei, const GLuint*)>
class GLMultiHandle
{
public:
    explicit GLMultiHandle(GLuint id) :
        id { id }
    {
    }

    ~GLMultiHandle()
    {
        if (id)
            (*deleter)(1, &id);
    }

    GLMultiHandle(GLMultiHandle const&) = delete;

    GLMultiHandle& operator=(GLMultiHandle const&) = delete;

    GLMultiHandle(GLMultiHandle&& from) :
        id { from.id }
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

using TextureHandle = GLMultiHandle<&glDeleteTextures>;
using FramebufferHandle = GLMultiHandle<&glDeleteFramebuffers>;
}

// Shader that converts colors to grayscale.
const GLchar* grayscale_src = "uniform sampler2D tex;\n"
                              "vec4 sample_to_rgba(in vec2 texcoord) {\n"
                              "   vec4 col = texture2D(tex, texcoord);\n"
                              "   float s = (col[0] + col[1] + col[2]) / 3.0;\n"
                              "   return vec4(s, s, s, col[3]);\n"
                              "}\n";

// Shader that inverts colors.
const GLchar* invert_src = "uniform sampler2D tex;\n"
                           "vec4 sample_to_rgba(in vec2 texcoord) {\n"
                           "   vec4 col = texture2D(tex, texcoord);\n"
                           "   return vec4(1.0 - col[0], 1.0 - col[1], 1.0 - col[2], col[3]);\n"
                           "}\n";

class Renderer::OutputFilter : public mg::gl::OutputSurface
{
public:
    // NOTE: This must be called with a current GL context
    explicit OutputFilter(std::unique_ptr<OutputSurface> output) :
        output { std::move(output) },
        texture { make_texture(this->output->size()) },
        framebuffer { make_framebuffer(texture) },
        program { nullptr },
        position_attrib { 0 },
        texcoord_attrib { 0 },
        tex_uniform { 0 }
    {
    }

    bool needs_refresh(std::optional<std::string> const& path) const
    {
        if (path == program_path)
        {
            if (!program_path)
                return false;

            if (!std::filesystem::exists(*program_path))
                return false;

            if (last_write_time != std::filesystem::last_write_time(*program_path))
                return true;
            return false;
            ;
        }

        return true;
    }

    void set_custom_output_filter(std::optional<std::string> const& path)
    {
        if (!needs_refresh(path))
            return;

        has_program = false;
        program_path = path;

        if (!path)
            return;

        if (!std::filesystem::exists(*path))
            return;

        last_write_time = std::filesystem::last_write_time(*path);
        std::ifstream file(*path);
        if (!file.is_open())
        {
            mir::log_error("Could not open file: %s", path->c_str());
            return;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        next_program = buffer.str();
        has_program = true;
    }

    void bind() override
    {
        // Bypass if no filter.
        if (!has_program)
        {
            output->bind();
            return;
        }

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);

        if (next_program)
        {
            try
            {
                program = std::make_unique<ProgramHandle>(compile_program(next_program.value().c_str()));
                position_attrib = glGetAttribLocation(*program, "position");
                texcoord_attrib = glGetAttribLocation(*program, "texcoord");
                tex_uniform = glGetUniformLocation(*program, "tex");
                next_program = std::nullopt;
            }
            catch (std::exception const& e)
            {
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                mir::log_error("Failed to compile custom output filter: %s", e.what());
                has_program = false;
                next_program = std::nullopt;
                output->bind();
            }
        }
    }

    void make_current() override
    {
        output->make_current();
    }

    void release_current() override
    {
        output->release_current();
    }

    auto commit() -> std::unique_ptr<mg::Framebuffer> override
    {
        // Bypass if no filter.
        if (!has_program)
            return output->commit();

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        output->bind();

        glUseProgram(*program);
        glUniform1i(tex_uniform, 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        // Draw a single right angle triangle that covers the whole output.
        GLfloat vertices[] = { -1, -1, 3, -1, -1, 3 };
        GLfloat tex_coords[] = { 0, 0, 2, 0, 0, 2 };
        glEnableVertexAttribArray(position_attrib);
        glVertexAttribPointer(position_attrib, 2, GL_FLOAT, GL_FALSE, 0, vertices);
        glEnableVertexAttribArray(texcoord_attrib);
        glVertexAttribPointer(texcoord_attrib, 2, GL_FLOAT, GL_FALSE, 0, tex_coords);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        return output->commit();
    }

    auto size() const -> mir::geometry::Size override
    {
        return output->size();
    }

    auto layout() const -> Layout override
    {
        return output->layout();
    }

private:
    static GLuint compile_shader(GLenum type, GLchar const* src)
    {
        GLuint id = glCreateShader(type);
        if (!id)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create shader"));
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

    static ProgramHandle compile_program(GLchar const* src)
    {
        const GLchar* vertex_src = "attribute vec2 position;\n"
                                   "attribute vec2 texcoord;\n"
                                   "varying vec2 v_texcoord;\n"
                                   "void main() {\n"
                                   "   gl_Position = vec4(position, 0, 1); \n"
                                   "   v_texcoord = texcoord;\n"
                                   "}\n";

        ShaderHandle vertex_shader { compile_shader(GL_VERTEX_SHADER, vertex_src) };

        std::stringstream fragment_src;
        fragment_src
            << "#ifdef GL_ES\n"
               "precision mediump float;\n"
               "#endif\n"
            << "\n"
            << src
            << "\n"
            << "varying vec2 v_texcoord;\n"
               "void main() {\n"
               "    gl_FragColor = sample_to_rgba(v_texcoord);\n"
               "}\n";

        ShaderHandle fragment_shader { compile_shader(GL_FRAGMENT_SHADER, fragment_src.str().c_str()) };

        ProgramHandle program { glCreateProgram() };
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

    static GLuint make_texture(mir::geometry::Size size)
    {
        GLuint tex;
        glGenTextures(1, &tex);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0,
            GL_RGBA,
            size.width.as_value(),
            size.height.as_value(),
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        return tex;
    }

    static GLuint make_framebuffer(GLuint tex)
    {
        GLuint fb;
        glGenFramebuffers(1, &fb);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        return fb;
    }

    std::unique_ptr<mg::gl::OutputSurface> output;
    TextureHandle const texture;
    FramebufferHandle const framebuffer;
    bool has_program = false;
    std::optional<std::string> program_path;
    std::filesystem::file_time_type last_write_time;
    std::optional<std::string> next_program;
    std::unique_ptr<ProgramHandle> program;
    GLint position_attrib;
    GLint texcoord_attrib;
    GLint tex_uniform;
};

Renderer::Renderer(
    std::shared_ptr<mir::graphics::GLRenderingProvider> gl_interface,
    std::unique_ptr<mir::graphics::gl::OutputSurface> output,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<CompositorState> const& compositor_state) :
    output_surface { std::make_unique<OutputFilter>(make_output_current(std::move(output))) },
    clear_color { 0.0f, 0.0f, 0.0f, 1.0f },
    program_factory { std::make_unique<ProgramFactory>() },
    screen_to_gl_coords(1),
    display_transform(1),
    gl_interface { std::move(gl_interface) },
    border_model(Mesh::rectangle(glm::vec3(-0.5, -0.5, 0), glm::vec2(1, 1))),
    config { config },
    compositor_state { compositor_state }
{
    // http://directx.com/2014/06/egl-understanding-eglchooseconfig-then-ignoring-it/
    eglBindAPI(EGL_OPENGL_ES_API);
    EGLDisplay disp = eglGetCurrentDisplay();
    if (disp != EGL_NO_DISPLAY)
    {
        struct
        {
            GLint id;
            char const* label;
        } const eglstrings[] = {
            { EGL_VENDOR,      "EGL vendor"      },
            { EGL_VERSION,     "EGL version"     },
            { EGL_CLIENT_APIS, "EGL client APIs" },
            { EGL_EXTENSIONS,  "EGL extensions"  },
        };
        for (auto& s : eglstrings)
        {
            auto val = eglQueryString(disp, s.id);
            mir::log_info(std::string(s.label) + ": " + (val ? val : ""));
        }
    }

    struct
    {
        GLenum id;
        char const* label;
    } const glstrings[] = {
        { GL_VENDOR,                   "GL vendor"     },
        { GL_RENDERER,                 "GL renderer"   },
        { GL_VERSION,                  "GL version"    },
        { GL_SHADING_LANGUAGE_VERSION, "GLSL version"  },
        { GL_EXTENSIONS,               "GL extensions" },
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

    auto const& prog = program_factory->border();
    glUseProgram(prog.data.id);
    border_model.upload_to_gpu();
}

Renderer::~Renderer()
{
}

void Renderer::tessellate(
    std::vector<mgl::Primitive>& primitives,
    mg::Renderable const& renderable,
    bool const is_flipped)
{
    primitives.resize(1);
    primitives[0] = mgl::tessellate_renderable_into_rectangle(renderable, geom::Displacement { 0, 0 }, is_flipped);
}

Renderer::DrawData Renderer::get_draw_data(
    mir::graphics::Renderable const& renderable,
    std::vector<RenderData> const& data) const
{
    DrawData result = {
        true, RenderData { .surface = nullptr, .transform = renderable.transformation(), .workspace_transform = glm::mat4(1.0), .alpha = renderable.alpha(), .output_area = viewport }
    };
    if (auto const surface = renderable.surface_if_any())
    {
        result.data.surface = surface.value();
        for (auto const& item : data)
        {
            if (item.surface == surface.value())
            {
                result.data = item;
                if (!item.output_area.overlaps(viewport))
                {
                    result.enabled = false;
                    return result;
                }
                break;
            }
        }
    }

    return result;
}

auto Renderer::render(mg::RenderableList const& renderables) const -> std::unique_ptr<mg::Framebuffer>
{
    output_surface->set_custom_output_filter(config->output_filter().shader_path);
    output_surface->make_current();
    output_surface->bind();

    glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT);

    ++frameno;

    auto const& render_data = compositor_state->render_data_manager()->get();

    mir::scene::Surface const* last_surface = nullptr;
    for (auto const& r : renderables)
    {
        // Renderables are guaranteed to be grouped on a per-surface basis. With this in mind, we will
        // check the first renderable in a group for its surface. We will use that surface to figure
        // out if the renderable needs to draw a border, and we will draw that first if that is the case.
        auto const data = get_draw_data(*r, render_data);
        if (!data.enabled)
            continue;

        if (data.data.needs_outline)
        {
            if (auto const surface = r->surface_if_any())
            {
                if (last_surface != surface.value())
                {
                    last_surface = surface.value();
                    draw_border(*surface.value(), data);
                }
            }
        }

        draw(*r, data);
    }

    auto output = output_surface->commit();

    // Report any GL errors after commit, to catch any *during* commit
    while (auto const gl_error = glGetError())
        mir::log_debug("GL error: %d", gl_error);

    return output;
}

void Renderer::draw(
    mg::Renderable const& renderable,
    DrawData const& data) const
{
    auto const texture = gl_interface->as_texture(renderable.buffer());
    auto const clip_area = renderable.clip_area();
    if (clip_area)
    {
        // The clip area is relative to the top-left of the output that it is on.
        // glScissor's coordinates are in terms of the viewport however, where (x,y)
        // is the bottom-left corner of the scissor box in viewport coordinates.
        //
        // Hence, we need to put the clip area in the coordinates of the viewport itself.

        // First, we compute the intersection of the clip area with the viewport.
        auto const intersection = intersect(*clip_area, viewport);
        if (!intersection)
        {
            glDisable(GL_SCISSOR_TEST);
            return;
        }

        // Then we invert and calculate the scissor x and y.
        const auto scissor_x = intersection->top_left.x.as_int() - viewport.top_left.x.as_int();
        int scissor_y = 0;
        switch (output_surface->layout())
        {
        case mir::graphics::gl::OutputSurface::Layout::GL:
            scissor_y = viewport.size.height.as_int()
                - (intersection->top_left.y.as_int() - viewport.top_left.y.as_int())
                - intersection->size.height.as_int();
            break;
        case mir::graphics::gl::OutputSurface::Layout::TopRowFirst:
            scissor_y = intersection->top_left.y.as_int() - viewport.top_left.y.as_int();
            break;
        }

        glm::vec4 const scissor = data.data.workspace_transform * glm::vec4(scissor_x, scissor_y, 0, 1);

        glEnable(GL_SCISSOR_TEST);
        glScissor(
            static_cast<GLint>(scissor.x * x_scale),
            static_cast<GLint>(scissor.y * y_scale),
            static_cast<GLint>(intersection->size.width.as_int() * x_scale),
            static_cast<GLint>(intersection->size.height.as_int() * y_scale));
    }
    auto const surface_pos = clip_area.value_or(renderable.screen_position()).top_left;
    auto const surface_size = clip_area.value_or(renderable.screen_position()).size;

    // All the programs are held by program_factory through its lifetime. Using pointers avoids
    // -Wdangling-reference.
    float const alpha = renderable.alpha() * data.data.alpha;
    auto const* const prog = &dynamic_cast<Program const&>(texture->shader(*program_factory)).data;

    glUseProgram(prog->id);
    if (prog->last_used_frameno != frameno)
    { // Avoid reloading the screen-global uniforms on every renderable
        // TODO: We actually only need to bind these *once*, right? Not once per frame?
        prog->last_used_frameno = frameno;
        for (auto i = 0u; i < prog->tex_uniforms.size(); ++i)
        {
            if (prog->tex_uniforms[i] != -1)
            {
                glUniform1i(prog->tex_uniforms[i], (int)i);
            }
        }

        glUniformMatrix4fv(prog->display_transform_uniform, 1, GL_FALSE,
            glm::value_ptr(display_transform));
        glUniformMatrix4fv(prog->screen_to_gl_coords_uniform, 1, GL_FALSE,
            glm::value_ptr(screen_to_gl_coords));
    }

    glActiveTexture(GL_TEXTURE0);

    auto const centerx = surface_pos.x.as_int() + static_cast<GLfloat>(surface_size.width.as_int()) / 2.0f;
    auto const centery = surface_pos.y.as_int() + static_cast<GLfloat>(surface_size.height.as_int()) / 2.0f;
    glUniform2f(prog->center_uniform, centerx, centery);

    glUniformMatrix4fv(prog->transform_uniform, 1, GL_FALSE,
        glm::value_ptr(data.data.transform));

    auto const border_config = config->get_border_config();
    auto const content_radius = data.data.needs_outline ? std::max(border_config.radius - static_cast<GLfloat>(border_config.size), 0.f) : 0.f;
    glUniform1f(prog->border_radius_uniform, content_radius);

    glUniform1f(prog->alpha_uniform, alpha);
    glUniform2f(prog->surface_size_uniform, static_cast<GLfloat>(surface_size.width.as_value()), static_cast<GLfloat>(surface_size.height.as_value()));

    glUniformMatrix4fv(prog->workspace_transform_uniform, 1, GL_FALSE,
        glm::value_ptr(data.data.workspace_transform));

    glEnableVertexAttribArray(static_cast<GLuint>(prog->position_attr));
    glEnableVertexAttribArray(static_cast<GLuint>(prog->texcoord_attr));

    primitives.clear();
    tessellate(primitives, renderable, texture->layout() == mg::gl::Texture::Layout::TopRowFirst);

    // if we fail to load the texture, we need to carry on (part of lp:1629275)
    try
    {
        typedef struct // Represents parameters of glBlendFuncSeparate()
        {
            GLenum src_rgb, dst_rgb, src_alpha, dst_alpha;
        } BlendSeparate;

        BlendSeparate client_blend;

        // These renderable method names could be better (see LP: #1236224)
        if (renderable.shaped()) // Client is RGBA:
        {
            client_blend = { GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
                GL_ONE, GL_ONE_MINUS_SRC_ALPHA };
        }
        else if (renderable.alpha() == 1.0f) // RGBX and no window translucency:
        {
            client_blend = { GL_ONE, GL_ZERO,
                GL_ZERO, GL_ONE }; // Avoid using src_alpha!
        }
        else
        { // Client is RGBX but we also have window translucency.
            // The texture alpha channel is possibly uninitialized so we must be
            // careful and avoid using SRC_ALPHA (LP: #1423462).
            client_blend = { GL_ONE, GL_ONE_MINUS_CONSTANT_ALPHA,
                GL_ZERO, GL_ONE };
            glBlendColor(0.0f, 0.0f, 0.0f, renderable.alpha());
        }

        for (auto const& p : primitives)
        {
            auto const blend = client_blend;
            texture->bind();

            glVertexAttribPointer(static_cast<GLuint>(prog->position_attr), 3, GL_FLOAT,
                GL_FALSE, sizeof(mgl::Vertex),
                &p.vertices[0].position);
            glVertexAttribPointer(static_cast<GLuint>(prog->texcoord_attr), 2, GL_FLOAT,
                GL_FALSE, sizeof(mgl::Vertex),
                &p.vertices[0].texcoord);

            if (blend.dst_rgb == GL_ZERO)
            {
                glDisable(GL_BLEND);
            }
            else
            {
                glEnable(GL_BLEND);
                glBlendFuncSeparate(blend.src_rgb, blend.dst_rgb,
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

    glDisableVertexAttribArray(static_cast<GLuint>(prog->texcoord_attr));
    glDisableVertexAttribArray(static_cast<GLuint>(prog->position_attr));
    if (clip_area)
    {
        glDisable(GL_SCISSOR_TEST);
    }
}

void Renderer::draw_border(ms::Surface const& surface, DrawData const& data) const
{
    auto const clip_area_opt = surface.clip_area();
    if (!clip_area_opt)
        return;

    // First, we select the border shader as our shader
    auto const* const prog = &program_factory->border().data;
    glUseProgram(prog->id);

    // Next, we use the clip area as our rendering size
    auto const border_config = config->get_border_config();
    auto const border_rect = geom::Rectangle(
        geom::Point(
            clip_area_opt.value().top_left.x.as_value() * x_scale - border_config.size,
            clip_area_opt.value().top_left.y.as_value() * y_scale - border_config.size),
        geom::Size(
            clip_area_opt.value().size.width.as_value() * x_scale + 2 * border_config.size,
            clip_area_opt.value().size.height.as_value() * y_scale + 2 * border_config.size));

    // Next, we update the uniforms for the context, including global transforms
    glUniformMatrix4fv(prog->display_transform_uniform, 1, GL_FALSE,
        glm::value_ptr(display_transform));
    glUniformMatrix4fv(prog->screen_to_gl_coords_uniform, 1, GL_FALSE,
        glm::value_ptr(screen_to_gl_coords));
    glUniformMatrix4fv(prog->workspace_transform_uniform, 1, GL_FALSE,
        glm::value_ptr(data.data.workspace_transform));

    auto const color = data.data.is_focused ? border_config.focus_color : border_config.color;
    glUniform4f(prog->border_color_uniform, color.r, color.g, color.b, color.a);
    glUniform1f(prog->border_radius_uniform, border_config.radius);
    glUniform1f(prog->border_width_uniform, static_cast<float>(border_config.size));

    // Next, we set model-specific transforms
    float const alpha = data.data.alpha;
    glm::mat4 border_transform = glm::scale(
        glm::translate(
            glm::mat4(1.0),
            glm::vec3(border_rect.top_left.x.as_value(), border_rect.top_left.y.as_value(), 0)),
        glm::vec3(border_rect.size.width.as_value(), border_rect.size.height.as_value(), 1));

    auto const centerx = static_cast<GLfloat>(border_rect.top_left.x.as_int() + border_rect.size.width.as_int()) / 2.0f;
    auto const centery = static_cast<GLfloat>(border_rect.top_left.y.as_int() + border_rect.size.height.as_int()) / 2.0f;
    glUniform2f(prog->center_uniform, centerx, centery);
    glUniformMatrix4fv(prog->transform_uniform, 1, GL_FALSE,
        glm::value_ptr(data.data.transform));
    glUniformMatrix4fv(prog->border_transform_uniform, 1, GL_FALSE,
        glm::value_ptr(border_transform));
    glUniform1f(prog->alpha_uniform, alpha);
    glUniform2f(prog->surface_size_uniform, static_cast<GLfloat>(border_rect.size.width.as_value()), static_cast<GLfloat>(border_rect.size.height.as_value()));

    // Now we can render our model. This should be as easy
    glBindBuffer(GL_ARRAY_BUFFER, border_model.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, border_model.ebo);

    glEnableVertexAttribArray(static_cast<GLuint>(prog->position_attr));
    glEnableVertexAttribArray(static_cast<GLuint>(prog->texcoord_attr));
    glVertexAttribPointer(static_cast<GLuint>(prog->position_attr), 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glVertexAttribPointer(static_cast<GLuint>(prog->texcoord_attr), 2, GL_FLOAT, GL_FALSE,
        sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, texcoord)));

    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(border_model.indices.size()), GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(static_cast<GLuint>(prog->position_attr));
    glDisableVertexAttribArray(static_cast<GLuint>(prog->texcoord_attr));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Renderer::set_viewport(mir::geometry::Rectangle const& rect)
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
    screen_to_gl_coords = glm::translate(glm::mat4(1.0f), glm::vec3 { -1.0f, 1.0f, 0.0f });

    /*
     * Perspective division is one thing that can't be done in a matrix
     * multiplication. It happens after the matrix multiplications. GL just
     * scales {x,y} by 1/w. So modify the final part of the projection matrix
     * to set w ([3]) to be the incoming z coordinate ([2]).
     */
    screen_to_gl_coords[2][3] = -1.0f;

    float const vertical_fov_degrees = 30.0f;
    float half_height = (float)rect.size.height.as_int() / 2.f;
    float const near = half_height / tanf((float)(vertical_fov_degrees * M_PI / 180.0f) / 2.f);
    float const far = -near;

    screen_to_gl_coords = glm::scale(screen_to_gl_coords,
        glm::vec3 { 2.0f / (float)rect.size.width.as_int(),
            -2.0f / (float)rect.size.height.as_int(),
            2.0f / (near - far) });
    screen_to_gl_coords = glm::translate(screen_to_gl_coords,
        glm::vec3 { -rect.top_left.x.as_int(),
            -rect.top_left.y.as_int(),
            0.0f });

    viewport = rect;
    update_gl_viewport();
}

void Renderer::update_gl_viewport()
{
    output_surface->make_current();
    output_surface->bind();

    /*
     * Letterboxing: Move the glViewport to add black bars in the case that
     * the logical viewport aspect ratio doesn't match the display aspect.
     * This keeps pixels square. Note "black"-bars are really glClearColor.
     */
    auto transformed_viewport = display_transform * glm::vec4(viewport.size.width.as_int(), viewport.size.height.as_int(), 0, 1);
    auto viewport_width = fabs(transformed_viewport[0]);
    auto viewport_height = fabs(transformed_viewport[1]);

    auto const output_size = output_surface->size();
    int const output_width = output_size.width.as_value();
    int const output_height = output_size.height.as_value();

    x_scale = static_cast<double>(output_width) / viewport_width;
    y_scale = static_cast<double>(output_height) / viewport_height;

    if (viewport_width > 0.0f && viewport_height > 0.0f && output_width > 0 && output_height > 0)
    {
        GLint reduced_width = output_width, reduced_height = output_height;
        // if viewport_aspect_ratio >= output_aspect_ratio
        if (viewport_width * (float)output_height >= (float)output_width * viewport_height)
            reduced_height = (int)((float)output_width * viewport_height / viewport_width);
        else
            reduced_width = (int)((float)output_height * viewport_width / viewport_height);

        GLint offset_x = (output_width - reduced_width) / 2;
        GLint offset_y = (output_height - reduced_height) / 2;

        glViewport(offset_x, offset_y, reduced_width, reduced_height);

        // outline_shader.setViewport(offset_x, offset_y, reduced_width, reduced_height);
    }
}

void Renderer::set_output_transform(glm::mat2 const& t)
{
    auto new_display_transform = glm::mat4(t);

    switch (output_surface->layout())
    {
    case mir::graphics::gl::OutputSurface::Layout::GL:
        break;
    case mir::graphics::gl::OutputSurface::Layout::TopRowFirst:
        // GL is going to render in its own coordinate system, but the OutputSurface
        // wants the output to be the other way up. Get GL to render upside-down instead.
        new_display_transform = glm::mat4 {
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

void Renderer::set_output_filter(MirOutputFilter)
{
}

void Renderer::suspend()
{
    output_surface->release_current();
}
