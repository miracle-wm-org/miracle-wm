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

#define GLM_FORCE_RADIANS
#define MIR_LOG_COMPONENT "GLRenderer"

#include "renderer.h"
#include "compositor_state.h"
#include "config.h"
#include "geometry_helpers.h"
#include "math_helpers.h"
#include "program_factory.h"
#include "tessellation_helpers.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <chrono>
#include <cmath>
#include <ctime>
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
/// Monotonic anchor captured at process load, shared by all per-output
/// OutputFilters so the screen-shader `time` uniform reads the same on every
/// output.
std::chrono::steady_clock::time_point const g_program_start = std::chrono::steady_clock::now();

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
        framebuffer { make_framebuffer(texture) }
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
        // If a plugin-set source filter was active, drop it and force the
        // config path to (re)load below.
        if (plugin_source_active)
        {
            plugin_source_active = false;
            program_path = std::nullopt;
            has_program = false;
            applied_screen_shader_generation = ~0ull;
        }
        else if (!needs_refresh(path))
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
        next_passes = std::vector<std::string> { buffer.str() };
        has_program = true;
    }

    /// Apply a plugin-supplied screen shader, bypassing the config file path.
    /// \p passes is a chained multi-pass shader (see #SamplerRegistry).
    /// \p generation is used to detect changes cheaply.
    void set_custom_output_filter_source(std::optional<std::vector<std::string>> const& passes, uint64_t generation)
    {
        plugin_source_active = passes.has_value();
        if (generation == applied_screen_shader_generation)
            return;

        applied_screen_shader_generation = generation;
        program_path = std::nullopt;
        has_program = passes.has_value() && !passes->empty();
        next_passes = passes;
    }

    void bind() override
    {
        // (Re)compile pending passes before rendering the screen content.
        if (next_passes)
        {
            try
            {
                std::vector<CompiledPass> compiled;
                compiled.reserve(next_passes->size());
                for (auto const& src : *next_passes)
                    compiled.push_back(compile_pass(src.c_str()));
                programs = std::move(compiled);
                has_program = !programs.empty();
                next_passes = std::nullopt;
            }
            catch (std::exception const& e)
            {
                mir::log_error("Failed to compile custom output filter: %s", e.what());
                programs.clear();
                has_program = false;
                next_passes = std::nullopt;
            }
        }

        // Bypass if no filter.
        if (!has_program)
        {
            output->bind();
            return;
        }

        // Render the screen content into our intermediate texture; commit() then
        // runs the shader pass(es) over it.
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
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

        // A single right-angle triangle that covers the whole output.
        static GLfloat const vertices[] = { -1, -1, 3, -1, -1, 3 };
        static GLfloat const tex_coords[] = { 0, 0, 2, 0, 0, 2 };

        auto const out_size = output->size();
        int const w = miracle::geometry_helpers::gl::width_value(out_size);
        int const h = miracle::geometry_helpers::gl::height_value(out_size);

        size_t const pass_count = programs.size();
        if (pass_count > 1)
        {
            pass_targets[0].ensure(out_size);
            pass_targets[1].ensure(out_size);
        }

        // Intermediate passes must overwrite, not blend into, their targets.
        GLboolean const blend_was_enabled = glIsEnabled(GL_BLEND);
        glDisable(GL_BLEND);

        // `texture` holds the original composited screen content; it is the
        // `tex_source` for every pass and the `tex` input of pass 0.
        GLuint read_tex = texture;
        for (size_t i = 0; i < pass_count; ++i)
        {
            bool const last = (i == pass_count - 1);
            auto const& cp = programs[i];

            if (last)
            {
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                output->bind(); // restores the output viewport
            }
            else
            {
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pass_targets[i % 2].framebuffer_id);
                glViewport(0, 0, w, h);
            }

            glUseProgram(cp.program);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, read_tex);
            if (cp.tex_uniform >= 0)
                glUniform1i(cp.tex_uniform, 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, texture);
            if (cp.tex_source_uniform >= 0)
                glUniform1i(cp.tex_source_uniform, 1);

            if (cp.surface_size_uniform >= 0)
                glUniform2f(cp.surface_size_uniform, static_cast<float>(w), static_cast<float>(h));

            if (cp.time_uniform >= 0)
                glUniform1f(cp.time_uniform,
                    std::chrono::duration<float>(std::chrono::steady_clock::now() - g_program_start).count());

            if (cp.time_of_day_uniform >= 0)
            {
                auto const now = std::chrono::system_clock::now();
                std::time_t const tt = std::chrono::system_clock::to_time_t(now);
                std::tm lt {};
                localtime_r(&tt, &lt);
                auto const frac = std::chrono::duration<float>(
                    now.time_since_epoch() - std::chrono::floor<std::chrono::seconds>(now.time_since_epoch()))
                                      .count();
                glUniform1f(cp.time_of_day_uniform,
                    lt.tm_hour * 3600.f + lt.tm_min * 60.f + lt.tm_sec + frac);
            }

            glEnableVertexAttribArray(cp.position_attrib);
            glVertexAttribPointer(cp.position_attrib, 2, GL_FLOAT, GL_FALSE, 0, vertices);
            glEnableVertexAttribArray(cp.texcoord_attrib);
            glVertexAttribPointer(cp.texcoord_attrib, 2, GL_FLOAT, GL_FALSE, 0, tex_coords);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            if (!last)
                read_tex = pass_targets[i % 2].texture_id;
        }

        if (blend_was_enabled)
            glEnable(GL_BLEND);
        glActiveTexture(GL_TEXTURE0);

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
    /// A single compiled screen-shader pass and its attribute/uniform locations.
    struct CompiledPass
    {
        ProgramHandle program;
        GLint position_attrib = -1;
        GLint texcoord_attrib = -1;
        GLint tex_uniform = -1;
        GLint tex_source_uniform = -1;
        GLint surface_size_uniform = -1;
        GLint time_uniform = -1;
        GLint time_of_day_uniform = -1;
    };

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

    static CompiledPass compile_pass(GLchar const* src)
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
            // The same shader contract as window shaders: `tex` is this pass's
            // input (pass 0 = original screen), `tex_source` is always the
            // original screen content, `surfaceSize` is the output size in px.
            // `time` is seconds since the compositor started; `timeOfDay` is
            // seconds since local midnight. All are optional — declare only what
            // you use.
            << "uniform sampler2D tex;\n"
               "uniform sampler2D tex_source;\n"
               "uniform vec2 surfaceSize;\n"
               "uniform float time;\n"
               "uniform float timeOfDay;\n"
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

        CompiledPass cp { std::move(program) };
        cp.position_attrib = glGetAttribLocation(cp.program, "position");
        cp.texcoord_attrib = glGetAttribLocation(cp.program, "texcoord");
        cp.tex_uniform = glGetUniformLocation(cp.program, "tex");
        cp.tex_source_uniform = glGetUniformLocation(cp.program, "tex_source");
        cp.surface_size_uniform = glGetUniformLocation(cp.program, "surfaceSize");
        cp.time_uniform = glGetUniformLocation(cp.program, "time");
        cp.time_of_day_uniform = glGetUniformLocation(cp.program, "timeOfDay");
        return cp;
    }

    static GLuint make_texture(mir::geometry::Size size)
    {
        GLuint tex;
        glGenTextures(1, &tex);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0,
            GL_RGBA,
            miracle::geometry_helpers::gl::width_value(size),
            miracle::geometry_helpers::gl::height_value(size),
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
    /// Pending shader source awaiting (re)compilation in bind().
    std::optional<std::vector<std::string>> next_passes;
    /// Compiled passes, run in order in commit().
    std::vector<CompiledPass> programs;
    /// Ping-pong targets for intermediate passes (only used when >1 pass).
    PassTarget pass_targets[2];
    bool plugin_source_active = false;
    uint64_t applied_screen_shader_generation = ~0ull;
};

Renderer::PassTarget::~PassTarget()
{
    if (framebuffer_id)
        glDeleteFramebuffers(1, &framebuffer_id);
    if (texture_id)
        glDeleteTextures(1, &texture_id);
}

void Renderer::PassTarget::ensure(mir::geometry::Size requested)
{
    if (requested.width.as_int() <= size.width.as_int() && requested.height.as_int() <= size.height.as_int())
        return;

    if (framebuffer_id)
        glDeleteFramebuffers(1, &framebuffer_id);
    if (texture_id)
        glDeleteTextures(1, &texture_id);

    size = requested;

    glGenTextures(1, &texture_id);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        size.width.as_int(), size.height.as_int(), 0,
        GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glGenFramebuffers(1, &framebuffer_id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_id);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, texture_id, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

int Renderer::run_offscreen_passes(
    mg::gl::Texture& texture,
    uint8_t shader_id,
    size_t pass_count,
    mir::geometry::Size buf_size,
    bool source_is_top_row_first) const
{
    // Save GL state that the off-screen passes will alter.
    GLint saved_fbo = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &saved_fbo);
    GLint saved_vp[4];
    glGetIntegerv(GL_VIEWPORT, saved_vp);
    GLboolean scissor_was_enabled = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean blend_was_enabled = glIsEnabled(GL_BLEND);

    int const w = buf_size.width.as_int();
    int const h = buf_size.height.as_int();

    pass_targets[0].ensure(buf_size);
    pass_targets[1].ensure(buf_size);

    glViewport(0, 0, w, h);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);

    // Fullscreen triangle positions (NDC) and two sets of texcoords.
    // Normal (GL convention, y=0 at bottom):
    static const GLfloat tri_pos[] = { -1.f, -1.f, 3.f, -1.f, -1.f, 3.f };
    static const GLfloat tri_tc_normal[] = { 0.f, 0.f, 2.f, 0.f, 0.f, 2.f };
    // Flipped Y (for TopRowFirst source textures):
    static const GLfloat tri_tc_flipped[] = { 0.f, 1.f, 2.f, 1.f, 0.f, -1.f };

    int ping = 0;
    int pong = 1;

    for (size_t i = 0; i < pass_count - 1; ++i)
    {
        auto variant = program_factory->resolve_custom_pass(shader_id, i, pass_count);
        PassProgram* pp = std::get<PassProgram*>(variant);
        // The shader may have been removed mid-frame (plugin unload). Bail out and let
        // the caller fall back to the default window shader.
        if (!pp)
            break;

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pass_targets[ping].framebuffer_id);
        glUseProgram(pp->id);

        // Unit 0: tex (the pass input)
        glActiveTexture(GL_TEXTURE0);
        if (i == 0)
        {
            // Pass 0 reads from the Mir window content texture.
            texture.bind();
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, pass_targets[pong].texture_id);
        }
        if (pp->tex_uniform >= 0)
            glUniform1i(pp->tex_uniform, 0);

        // Unit 1: tex_source (always the original window content)
        glActiveTexture(GL_TEXTURE1);
        texture.bind();
        if (pp->tex_source_uniform >= 0)
            glUniform1i(pp->tex_source_uniform, 1);

        if (pp->surface_size_uniform >= 0)
            glUniform2f(pp->surface_size_uniform, static_cast<GLfloat>(w), static_cast<GLfloat>(h));

        glEnableVertexAttribArray(static_cast<GLuint>(pp->position_attr));
        glEnableVertexAttribArray(static_cast<GLuint>(pp->texcoord_attr));

        glVertexAttribPointer(static_cast<GLuint>(pp->position_attr),
            2, GL_FLOAT, GL_FALSE, 0, tri_pos);

        // For pass 0: optionally flip Y for TopRowFirst source textures.
        // For pass 1+: scale UV so only the valid buf_size sub-region of the
        // (potentially oversized) pass_target texture is sampled.
        GLfloat tri_tc_scaled[6];
        const GLfloat* tc;
        if (i == 0)
        {
            tc = source_is_top_row_first ? tri_tc_flipped : tri_tc_normal;
        }
        else
        {
            float const u_max = static_cast<float>(w) / static_cast<float>(pass_targets[pong].size.width.as_int());
            float const v_max = static_cast<float>(h) / static_cast<float>(pass_targets[pong].size.height.as_int());
            tri_tc_scaled[0] = 0.f;
            tri_tc_scaled[1] = 0.f;
            tri_tc_scaled[2] = 2.f * u_max;
            tri_tc_scaled[3] = 0.f;
            tri_tc_scaled[4] = 0.f;
            tri_tc_scaled[5] = 2.f * v_max;
            tc = tri_tc_scaled;
        }
        glVertexAttribPointer(static_cast<GLuint>(pp->texcoord_attr),
            2, GL_FLOAT, GL_FALSE, 0, tc);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glDisableVertexAttribArray(static_cast<GLuint>(pp->texcoord_attr));
        glDisableVertexAttribArray(static_cast<GLuint>(pp->position_attr));

        std::swap(ping, pong);
    }

    // Restore GL state.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(saved_fbo));
    glViewport(saved_vp[0], saved_vp[1], saved_vp[2], saved_vp[3]);
    if (scissor_was_enabled)
        glEnable(GL_SCISSOR_TEST);
    if (blend_was_enabled)
        glEnable(GL_BLEND);

    // Return the index of the target holding the result (last ping before final swap).
    return pong;
}

Renderer::Renderer(
    std::shared_ptr<mir::graphics::GLRenderingProvider> gl_interface,
    std::unique_ptr<mir::graphics::gl::OutputSurface> output,
    std::shared_ptr<Config> const& config,
    std::shared_ptr<CompositorState> const& compositor_state,
    std::shared_ptr<SamplerRegistry> const& sampler_registry) :
    output_surface { std::make_unique<OutputFilter>(make_output_current(std::move(output))) },
    clear_color { 0.0f, 0.0f, 0.0f, 1.0f },
    program_factory { std::make_unique<ProgramFactory>(sampler_registry) },
    screen_to_gl_coords(1),
    display_transform(1),
    gl_interface { std::move(gl_interface) },
    border_model(Mesh::rectangle(glm::vec3(-0.5, -0.5, 0), glm::vec2(1, 1))),
    config { config },
    compositor_state { compositor_state },
    sampler_registry { sampler_registry }
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

RenderData const* Renderer::find_render_data(mir::scene::Surface const* surface) const
{
    if (!surface)
        return nullptr;

    for (auto const& item : render_data_cache)
    {
        if (item.window.operator std::shared_ptr<mir::scene::Surface>().get() == surface)
            return &item;
    }

    return nullptr;
}

Renderer::DrawData Renderer::get_draw_data(
    mir::graphics::Renderable const& renderable,
    mir::scene::Surface const* surface,
    RenderData const* tracked,
    std::optional<SceneOverridePlacement> const& placement,
    mir::geometry::Rectangle const& placement_real) const
{
    (void)surface;
    DrawData result = {
        true, renderable.alpha(), RenderData { .transform = renderable.transformation(), .workspace_transform = glm::mat4(1.0), .output_area = viewport }
    };
    if (tracked)
    {
        result.data = *tracked;
        if (placement)
        {
            result.placement = placement;
            result.override_real = placement_real;

            // The override may draw the surface far from its tracked output
            // area, so cull against where the placement actually lands: the
            // real rectangle moved to the placement position, then transformed
            // about its center.
            glm::vec2 const size {
                static_cast<float>(std::max(placement_real.size.width.as_value(), 1)),
                static_cast<float>(std::max(placement_real.size.height.as_value(), 1))
            };
            glm::vec2 const center = placement->position + size / 2.f;
            glm::vec2 min = center, max = center;
            for (auto const& corner : {
                     placement->position,
                     glm::vec2 { placement->position.x + size.x, placement->position.y          },
                     glm::vec2 { placement->position.x,          placement->position.y + size.y },
                     placement->position + size
            })
            {
                glm::vec2 const transformed = glm::vec2(placement->transformation * glm::vec4(corner - center, 0.f, 1.f)) + center;
                min = glm::min(min, transformed);
                max = glm::max(max, transformed);
            }

            geom::Rectangle const placement_rect {
                geom::Point { static_cast<int>(min.x),         static_cast<int>(min.y)         },
                geom::Size { static_cast<int>(max.x - min.x), static_cast<int>(max.y - min.y) }
            };
            if (!placement_rect.overlaps(viewport))
                result.enabled = false;
        }
        else if (tracked->output_area && !tracked->output_area->overlaps(viewport))
            result.enabled = false;
    }

    return result;
}

auto Renderer::render(mg::RenderableList const& renderables) const -> std::unique_ptr<mg::Framebuffer>
{
    // A plugin-set screen shader takes precedence over the config path.
    auto const screen = sampler_registry->screen_shader_state();
    if (screen.passes)
        output_surface->set_custom_output_filter_source(screen.passes, screen.generation);
    else
        output_surface->set_custom_output_filter(config->output_filter().shader_path);
    output_surface->make_current();
    output_surface->bind();

    glClearColor(clear_color[0], clear_color[1], clear_color[2], clear_color[3]);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClear(GL_COLOR_BUFFER_BIT);

    ++frameno;

    compositor_state->render_data_manager()->copy_if_changed(render_data_generation, render_data_cache);

    // Resolving the override once pins it for the whole frame, so a concurrent
    // release on another thread cannot destroy it mid-render.
    auto const scene_override = compositor_state->scene_override_manager()->try_resolve();

    // Renderables are guaranteed to be grouped on a per-surface basis, so the tracked
    // render data is looked up once per surface group and reused for the renderables
    // that follow. The first renderable of a group also draws the border, if needed.
    mir::scene::Surface const* group_surface = nullptr;
    RenderData const* group_data = nullptr;
    std::optional<SceneOverridePlacement> group_placement;
    geom::Rectangle group_real;
    bool first_renderable = true;
    for (auto const& r : renderables)
    {
        mir::scene::Surface const* surface = nullptr;
        if (auto const s = r->surface_if_any())
            surface = s.value();

        bool const new_group = first_renderable || surface != group_surface;
        first_renderable = false;
        if (new_group)
        {
            group_surface = surface;
            group_data = find_render_data(surface);
            group_placement = std::nullopt;
            if (scene_override && group_data && group_data->window)
            {
                group_placement = scene_override->place(group_data->window);
                group_real = r->screen_position();
            }
        }

        auto const data = get_draw_data(*r, surface, group_data, group_placement, group_real);
        if (!data.enabled)
            continue;

        draw(*r, data);

        if (new_group && data.data.needs_outline && surface)
            draw_border(*surface, data);
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
    auto clip_area = renderable.clip_area();

    // When the scene override supplies a placement, the surface is drawn at its
    // real size but relocated to the placement position, with the placement's
    // transformation applied about the center of the relocated rectangle via
    // the existing 'center' + 'transform' uniforms.
    glm::vec2 placement_center, placement_offset;
    if (data.placement)
    {
        glm::vec2 const real_size {
            static_cast<float>(std::max(data.override_real.size.width.as_value(), 1)),
            static_cast<float>(std::max(data.override_real.size.height.as_value(), 1))
        };
        glm::vec2 const real_top_left {
            static_cast<float>(data.override_real.top_left.x.as_value()),
            static_cast<float>(data.override_real.top_left.y.as_value())
        };
        placement_offset = data.placement->position - real_top_left;
        placement_center = data.placement->position + real_size / 2.f;

        // An overridden surface is never clipped. The clip rectangle describes
        // where the window management policy wanted the window to sit, which is
        // not where the override is drawing it, and the override already
        // controls the size the surface appears at.
        clip_area = std::nullopt;
    }

    if (clip_area)
    {
        // First, we compute the intersection of the clip area with the viewport.
        auto const intersection = intersect(*clip_area, viewport);
        if (!intersection)
        {
            glDisable(GL_SCISSOR_TEST);
            return;
        }

        // Next, we map the viewport-relative clip rectangle to framebuffer pixel coordinates,
        // accounting for output rotation and surface layout.
        using namespace miracle::geometry_helpers::gl;
        int const rel_x = x_int(intersection->top_left) - x_int(viewport.top_left);
        int const rel_y = y_int(intersection->top_left) - y_int(viewport.top_left);
        int const rel_w = width_int(intersection->size);
        int const rel_h = height_int(intersection->size);
        int const vp_w = width_int(viewport.size);
        int const vp_h = height_int(viewport.size);

        float fb_x, fb_y, fb_w, fb_h;
        bool const top_row_first = (output_surface->layout() == mir::graphics::gl::OutputSurface::Layout::TopRowFirst);

        switch (output_rotation)
        {
        case OutputRotation::normal:
            fb_w = rel_w;
            fb_h = rel_h;
            fb_x = rel_x;
            fb_y = top_row_first ? rel_y : (vp_h - rel_y - rel_h);
            break;
        case OutputRotation::left_90:
            fb_w = rel_h;
            fb_h = rel_w;
            fb_x = rel_y;
            fb_y = top_row_first ? (vp_w - rel_x - rel_w) : rel_x;
            break;
        case OutputRotation::inverted_180:
            fb_w = rel_w;
            fb_h = rel_h;
            fb_x = vp_w - rel_x - rel_w;
            fb_y = top_row_first ? (vp_h - rel_y - rel_h) : rel_y;
            break;
        case OutputRotation::right_270:
            fb_w = rel_h;
            fb_h = rel_w;
            fb_x = vp_h - rel_y - rel_h;
            fb_y = top_row_first ? rel_x : (vp_w - rel_x - rel_w);
            break;
        }

        // Apply the workspace transform to the scissor position and size.
        glm::vec4 scissor_pos = data.data.workspace_transform * glm::vec4(fb_x, fb_y, 0, 1);
        glm::vec4 scissor_size = data.data.workspace_transform * glm::vec4(fb_w, fb_h, 0, 0);
        fb_x = scissor_pos.x;
        fb_y = scissor_pos.y;
        fb_w = scissor_size.x;
        fb_h = scissor_size.y;

        glEnable(GL_SCISSOR_TEST);
        glScissor(
            static_cast<GLint>(fb_x * x_scale),
            static_cast<GLint>(fb_y * y_scale),
            static_cast<GLint>(fb_w * x_scale),
            static_cast<GLint>(fb_h * y_scale));
    }
    auto surface_pos = clip_area.value_or(renderable.screen_position()).top_left;
    auto surface_size = clip_area.value_or(renderable.screen_position()).size;
    if (data.placement)
    {
        surface_pos = geom::Point { static_cast<int>(data.placement->position.x), static_cast<int>(data.placement->position.y) };
        surface_size = data.override_real.size;
    }

    // All the programs are held by program_factory through its lifetime. Using pointers avoids
    // -Wdangling-reference.
    float const alpha = data.alpha;

    // Determine pass count for multi-pass custom shaders.
    size_t const num_passes = data.data.shader_id
        ? program_factory->pass_count(*data.data.shader_id)
        : 1;
    bool const is_multipass = num_passes > 1;

    // For multi-pass shaders, run intermediate off-screen passes first.
    int offscreen_result_target = -1;
    if (is_multipass)
    {
        bool const source_is_top_row_first = texture->layout() == mg::gl::Texture::Layout::TopRowFirst;
        offscreen_result_target = run_offscreen_passes(
            *texture,
            *data.data.shader_id,
            num_passes,
            renderable.buffer()->size(),
            source_is_top_row_first);
    }

    // Resolve the final-pass program.
    // For multi-pass: final pass (pass_count-1) via resolve_custom_pass.
    // For single-pass custom or default: existing path.
    // If a custom shader cannot be resolved (e.g. its owning plugin unloaded and the
    // shader was removed), fall back to the default window shader instead of throwing.
    auto const* const prog = [&]() -> ProgramData const*
    {
        if (is_multipass)
        {
            auto variant = program_factory->resolve_custom_pass(
                *data.data.shader_id, num_passes - 1, num_passes);
            if (auto* const p = std::get<Program*>(variant))
                return &p->data;
        }
        else if (data.data.shader_id)
        {
            if (auto* const p = program_factory->resolve_custom(*data.data.shader_id))
                return &dynamic_cast<Program const&>(*p).data;
        }
        return &dynamic_cast<Program const&>(texture->shader(*program_factory)).data;
    }();

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

    using namespace miracle::geometry_helpers::gl;
    if (data.placement)
    {
        // The exact (unrounded) center of the relocated window is the pivot the
        // override mapping below is derived about.
        glUniform2f(prog->center_uniform, placement_center.x, placement_center.y);

        // Moves the real rect to the placement position, then applies the
        // override's transformation about that rect's center. The policy's own
        // transform is intentionally not composed in: the override owns the
        // window's presentation while active.
        glm::mat4 const override_transform = data.placement->transformation
            * glm::translate(glm::mat4(1.f), glm::vec3(placement_offset, 0.f));
        glUniformMatrix4fv(prog->transform_uniform, 1, GL_FALSE,
            glm::value_ptr(override_transform));
    }
    else
    {
        auto const centerx = x(surface_pos) + width(surface_size) / 2.0f;
        auto const centery = y(surface_pos) + height(surface_size) / 2.0f;
        glUniform2f(prog->center_uniform, centerx, centery);

        glUniformMatrix4fv(prog->transform_uniform, 1, GL_FALSE,
            glm::value_ptr(data.data.transform));
    }

    auto const border_config = config->get_border_config();
    auto const content_radius = data.data.needs_outline ? std::max(border_config.radius - static_cast<GLfloat>(border_config.size), 0.f) : 0.f;
    glUniform1f(prog->border_radius_uniform, content_radius);

    glUniform1f(prog->alpha_uniform, alpha);
    glUniform2f(prog->surface_size_uniform, static_cast<GLfloat>(miracle::geometry_helpers::gl::width_value(surface_size)), static_cast<GLfloat>(miracle::geometry_helpers::gl::height_value(surface_size)));

    glUniformMatrix4fv(prog->workspace_transform_uniform, 1, GL_FALSE,
        glm::value_ptr(data.data.workspace_transform));

    glEnableVertexAttribArray(static_cast<GLuint>(prog->position_attr));
    glEnableVertexAttribArray(static_cast<GLuint>(prog->texcoord_attr));

    primitives.clear();
    // For multi-pass shaders, the intermediate texture is in GL convention (y=0 at bottom),
    // so tessellate with is_flipped=false regardless of the original Mir texture layout.
    tessellate(primitives, renderable, is_multipass ? false : (texture->layout() == mg::gl::Texture::Layout::TopRowFirst));

    // The pass_targets use a grow-only allocation: a previously large window may have left
    // the textures bigger than the current buf_size. Scale the UV coords so the final pass
    // only samples the valid buf_size sub-region of the (potentially oversized) texture.
    if (is_multipass)
    {
        auto const& target = pass_targets[offscreen_result_target];
        float const u_scale = static_cast<float>(renderable.buffer()->size().width.as_int())
            / static_cast<float>(target.size.width.as_int());
        float const v_scale = static_cast<float>(renderable.buffer()->size().height.as_int())
            / static_cast<float>(target.size.height.as_int());
        if (u_scale < 1.0f || v_scale < 1.0f)
        {
            for (auto& p : primitives)
            {
                for (int vi = 0; vi < p.nvertices; ++vi)
                {
                    p.vertices[vi].texcoord[0] *= u_scale;
                    p.vertices[vi].texcoord[1] *= v_scale;
                }
            }
        }
    }

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
        else if (alpha == 1.0f) // RGBX and no window translucency:
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
            glBlendColor(0.0f, 0.0f, 0.0f, alpha);
        }

        for (auto const& p : primitives)
        {
            auto const blend = client_blend;

            if (is_multipass)
            {
                // Bind the result of the off-screen pass chain.
                // Unit 1 (tex_source) = original window content.
                glActiveTexture(GL_TEXTURE1);
                texture->bind();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, pass_targets[offscreen_result_target].texture_id);
            }
            else
            {
                texture->bind();
            }

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
    std::optional<geom::Rectangle> clip_area_opt;
    if (data.placement)
    {
        // An overridden surface is never clipped: the border wraps the window
        // at its real size, relocated to the placement, and is scaled along
        // with it by the placement's own transformation below.
        clip_area_opt = geom::Rectangle {
            geom::Point { static_cast<int>(data.placement->position.x), static_cast<int>(data.placement->position.y) },
            data.override_real.size
        };
    }
    else
    {
        clip_area_opt = surface.clip_area();
        if (!clip_area_opt)
            clip_area_opt = geom::Rectangle { surface.top_left(), surface.window_size() };
    }

    // First, we select the border shader as our shader
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA,
        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    auto const* const prog = &program_factory->border().data;
    glUseProgram(prog->id);

    // Next, we use the clip area as our rendering size
    auto const border_config = config->get_border_config();
    auto const border_rect = geom::Rectangle(
        geom::Point(
            clip_area_opt.value().top_left.x.as_value() * x_scale,
            clip_area_opt.value().top_left.y.as_value() * y_scale),
        geom::Size(
            clip_area_opt.value().size.width.as_value() * x_scale,
            clip_area_opt.value().size.height.as_value() * y_scale));

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
    using namespace miracle::geometry_helpers;
    float const alpha = data.alpha;
    glm::mat4 border_transform = glm::scale(
        glm::translate(
            glm::mat4(1.0),
            glm::vec3(gl::x(border_rect.top_left), gl::y(border_rect.top_left), 0)),
        glm::vec3(gl::width(border_rect.size), gl::height(border_rect.size), 1));

    auto const centerx = gl::x(border_rect.top_left) + gl::width(border_rect.size) / 2.0f;
    auto const centery = gl::y(border_rect.top_left) + gl::height(border_rect.size) / 2.0f;
    glUniform2f(prog->center_uniform, centerx, centery);
    glUniformMatrix4fv(prog->transform_uniform, 1, GL_FALSE,
        glm::value_ptr(data.placement ? data.placement->transformation : data.data.transform));
    glUniformMatrix4fv(prog->border_transform_uniform, 1, GL_FALSE,
        glm::value_ptr(border_transform));
    glUniform1f(prog->alpha_uniform, alpha);
    glUniform2f(prog->surface_size_uniform, static_cast<GLfloat>(gl::width_value(border_rect.size)), static_cast<GLfloat>(gl::height_value(border_rect.size)));

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

    // Detect rotation from the 2x2 transform matrix (column-major in glm)
    if (t[0][0] != 0.0f)
        output_rotation = (t[0][0] > 0) ? OutputRotation::normal : OutputRotation::inverted_180;
    else
        output_rotation = (t[1][0] < 0) ? OutputRotation::left_90 : OutputRotation::right_270;

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
