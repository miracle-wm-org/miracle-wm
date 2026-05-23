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

#include <cstdint>
#define MIR_LOG_COMPONENT "program_factory"

#include "program_factory.h"
#include <format>
#include <mir/graphics/egl_error.h>
#include <mir/log.h>

namespace
{
const GLchar* const vertex_shader_src = R"(
attribute vec3 position;
attribute vec2 texcoord;

uniform mat4 screen_to_gl_coords;
uniform mat4 display_transform;
uniform mat4 workspace_transform;
uniform mat4 transform;
uniform vec2 center;

varying vec2 v_texcoord;

void main() {
   vec4 p = vec4(center, 0.0, 0.0);
   vec4 transformed = (transform * (vec4(position, 1.0) - p)) + p;
   gl_Position = display_transform * screen_to_gl_coords * workspace_transform * transformed;
   v_texcoord = texcoord;
}
)";

const GLchar* const border_vertex_shader_src = R"(
attribute vec3 position;
attribute vec2 texcoord;

uniform mat4 screen_to_gl_coords;
uniform mat4 display_transform;
uniform mat4 workspace_transform;
uniform mat4 border_transform;
uniform mat4 transform;
uniform vec2 center;

varying vec2 v_texcoord;

void main() {
   // First, we transform the border to be resized and scaled to match the
   // surface that it is surrounding. We subtract p to shift the model origin
   // from its center (0,0) to its bottom-left (-0.5,-0.5), so the transform
   // maps the unit quad correctly to [border_rect.top_left, border_rect.top_left + border_rect.size].
   vec4 p = vec4(-0.5, -0.5, 0.0, 0.0);
   vec4 transformed = border_transform * (vec4(position, 1.0) - p);

   // Afterwards. we apply the regular transform from the surface.
   p = vec4(center, 0.0, 0.0);
   transformed = (transform * (transformed - p)) + p;

   gl_Position = display_transform * screen_to_gl_coords * workspace_transform * transformed;
   v_texcoord = texcoord;
}
)";

const GLchar* const fragment_border_src = R"(
#ifdef GL_ES
precision highp float;
#endif

uniform float alpha;
uniform vec2 surfaceSize;
uniform vec4 borderColor;
uniform float borderRadius;
uniform float borderWidth;

varying vec2 v_texcoord;

float roundedRectSDF(vec2 p, vec2 size, float r) {
    vec2 halfSize = size * 0.5;
    vec2 d = abs(p - halfSize) - (halfSize - vec2(r));
    return length(max(d, 0.0)) - r;
}

void main() {
    vec2 pixelPos = v_texcoord * surfaceSize;

    vec2 center = surfaceSize * 0.5;

    float outerSDF = roundedRectSDF(pixelPos, surfaceSize, borderRadius);
    vec2 innerSize = surfaceSize - vec2(borderWidth * 2.0);
    float innerRadius = max(borderRadius - borderWidth, 0.0);
    vec2 innerPos = pixelPos - center + (innerSize * 0.5);  // recenter coordinates
    float innerSDF = roundedRectSDF(innerPos, innerSize, innerRadius);

    float borderAlpha =
        smoothstep(0.5, -0.5, outerSDF) *
        (1.0 - smoothstep(0.5, -0.5, innerSDF));

    vec4 color = borderColor * alpha * borderAlpha;

    if (color.a < 0.01)
        discard;

    gl_FragColor = color;
}

)";

}

miracle::ProgramData::ProgramData(GLuint program_id)
{
    id = program_id;
    position_attr = glGetAttribLocation(id, "position");
    if (position_attr < 0)
        mir::log_warning("Program is missing position_attr");
    texcoord_attr = glGetAttribLocation(id, "texcoord");
    if (texcoord_attr < 0)
        mir::log_warning("Program is missing texcoord_attr");
    for (auto i = 0u; i < tex_uniforms.size(); ++i)
    {
        /* You can reference uniform arrays as tex[0], tex[1], tex[2], … until you
         * hit the end of the array, which will return -1 as the location.
         */
        auto const uniform_name = std::string { "tex[" } + std::to_string(i) + "]";
        tex_uniforms[i] = glGetUniformLocation(id, uniform_name.c_str());
    }
    center_uniform = glGetUniformLocation(id, "center");
    if (center_uniform < 0)
        mir::log_warning("Program is missing centre_uniform");

    display_transform_uniform = glGetUniformLocation(id, "display_transform");
    if (display_transform_uniform < 0)
        mir::log_warning("Program is missing display_transform_uniform");

    workspace_transform_uniform = glGetUniformLocation(id, "workspace_transform");
    if (workspace_transform_uniform < 0)
        mir::log_warning("Program is missing workspace_transform_uniform");

    transform_uniform = glGetUniformLocation(id, "transform");
    if (transform_uniform < 0)
        mir::log_warning("Program is missing transform_uniform");

    screen_to_gl_coords_uniform = glGetUniformLocation(id, "screen_to_gl_coords");
    if (screen_to_gl_coords_uniform < 0)
        mir::log_warning("Program is missing screen_to_gl_coords_uniform");

    alpha_uniform = glGetUniformLocation(id, "alpha");
    if (alpha_uniform < 0)
        mir::log_warning("Program is missing alpha_uniform");

    surface_size_uniform = glGetUniformLocation(id, "surfaceSize");
    if (surface_size_uniform < 0)
        mir::log_warning("Program is missing surfaceSize");

    border_transform_uniform = glGetUniformLocation(id, "border_transform");
    if (border_transform_uniform < 0)
        mir::log_warning("Program is missing border_transform_uniform");

    border_color_uniform = glGetUniformLocation(id, "borderColor");
    if (border_color_uniform < 0)
        mir::log_warning("Program is missing borderColor");

    border_radius_uniform = glGetUniformLocation(id, "borderRadius");
    if (border_radius_uniform < 0)
        mir::log_warning("Program is missing borderRadius");

    border_width_uniform = glGetUniformLocation(id, "borderWidth");
    if (border_width_uniform < 0)
        mir::log_warning("Program is missing borderWidth");
}

miracle::Program::Program(ProgramHandle&& program) :
    program_handle(std::move(program)),
    data { program_handle }
{
}

miracle::PassProgram::PassProgram(ProgramHandle&& prog) :
    program_handle(std::move(prog)),
    id { program_handle },
    position_attr { glGetAttribLocation(id, "position") },
    texcoord_attr { glGetAttribLocation(id, "texcoord") },
    tex_uniform { glGetUniformLocation(id, "tex") },
    tex_source_uniform { glGetUniformLocation(id, "tex_source") },
    surface_size_uniform { glGetUniformLocation(id, "surfaceSize") }
{
}

const GLchar* const pass_vertex_shader_src = R"(
attribute vec2 position;
attribute vec2 texcoord;
varying vec2 v_texcoord;
void main() {
    gl_Position = vec4(position, 0, 1);
    v_texcoord = texcoord;
}
)";

miracle::ProgramFactory::ProgramFactory(std::shared_ptr<SamplerRegistry> const& sampler_registry) :
    sampler_registry_ { sampler_registry },
    vertex_shader { compile_shader(GL_VERTEX_SHADER, vertex_shader_src) },
    pass_vertex_shader { compile_shader(GL_VERTEX_SHADER, pass_vertex_shader_src) },
    border_vertex_shader { compile_shader(GL_VERTEX_SHADER, border_vertex_shader_src) },
    border_fragment_shader { ShaderHandle(compile_shader(GL_FRAGMENT_SHADER, fragment_border_src)) },
    border_program { Program(link_shader(border_vertex_shader, border_fragment_shader)) }
{
}

mir::graphics::gl::Program& miracle::ProgramFactory::compile_fragment_shader(
    void const* id,
    char const* extension_fragment,
    char const* fragment_fragment)
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

    std::string const fragment_src = std::string(extension_fragment) +
        R"(

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 surfaceSize;

)" + std::string(fragment_fragment)
        +
        R"(

uniform float alpha;
uniform float borderRadius;

varying vec2 v_texcoord;  // This is going to be [0, 1]

float roundedRectSDF(vec2 p, vec2 size, float r) {
    vec2 halfSize = size * 0.5;
    vec2 d = abs(p - halfSize) - (halfSize - vec2(r));
    return length(max(d, 0.0)) - r;
}

void main() {
    vec2 pixelPos = v_texcoord * surfaceSize;
    float sdf = roundedRectSDF(pixelPos, surfaceSize, borderRadius);
    float shapeMask = 1.0 - smoothstep(0.0, 1.0, sdf);

    vec4 contentColor = alpha * sample_to_rgba(v_texcoord);
    contentColor *= shapeMask;
    if (contentColor.a < 0.01)
        discard;

   gl_FragColor = contentColor;
}
)";

    // GL shader compilation is *not* threadsafe, and requires external synchronisation
    std::lock_guard lock { compilation_mutex };

    ShaderHandle const alpha_shader {
        compile_shader(GL_FRAGMENT_SHADER, fragment_src.c_str())
    };

    programs.emplace_back(id, std::make_unique<Program>(link_shader(vertex_shader, alpha_shader)));

    return *programs.back().second;

    // We delete the shaders here. This is fine; it only marks them
    // for deletion. GL will only delete them once the GL Program they're linked in is destroyed.
}

mir::graphics::gl::Program& miracle::ProgramFactory::resolve_custom(uint8_t id)
{
    std::lock_guard lock { sampler_registry_->mutex };
    for (auto const& entry : sampler_registry_->entries)
    {
        if (entry.id == id)
        {
            return compile_fragment_shader(
                reinterpret_cast<void*>(id),
                "uniform sampler2D tex;\nuniform sampler2D tex_source;\n",
                entry.passes[0].c_str());
        }
    }

    throw std::runtime_error("Unknown custom sampler id");
}

size_t miracle::ProgramFactory::pass_count(uint8_t id) const
{
    std::lock_guard lock { sampler_registry_->mutex };
    for (auto const& entry : sampler_registry_->entries)
    {
        if (entry.id == id)
            return entry.passes.size();
    }
    return 1;
}

miracle::PassProgram& miracle::ProgramFactory::compile_intermediate_pass(void const* key, char const* sampler_glsl)
{
    for (auto const& pair : pass_programs)
    {
        if (pair.first == key)
            return *pair.second;
    }

    std::string const fragment_src = "#ifdef GL_ES\n"
                                     "precision mediump float;\n"
                                     "#endif\n"
                                     "\n"
                                     "uniform sampler2D tex;\n"
                                     "uniform sampler2D tex_source;\n"
                                     "uniform vec2 surfaceSize;\n"
                                     "\n"
        + std::string(sampler_glsl) + "\n"
                                      "varying vec2 v_texcoord;\n"
                                      "void main() {\n"
                                      "    gl_FragColor = sample_to_rgba(v_texcoord);\n"
                                      "}\n";

    std::lock_guard lock { compilation_mutex };

    ShaderHandle frag { compile_shader(GL_FRAGMENT_SHADER, fragment_src.c_str()) };
    ProgramHandle prog { glCreateProgram() };
    glAttachShader(prog, frag);
    glAttachShader(prog, pass_vertex_shader);
    glLinkProgram(prog);
    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        GLchar log[1024];
        glGetProgramInfoLog(prog, sizeof log - 1, NULL, log);
        log[sizeof log - 1] = '\0';
        throw std::runtime_error(std::string("Linking intermediate pass shader failed: ") + log);
    }

    pass_programs.emplace_back(key, std::make_unique<PassProgram>(std::move(prog)));
    return *pass_programs.back().second;
}

std::variant<miracle::Program*, miracle::PassProgram*> miracle::ProgramFactory::resolve_custom_pass(
    uint8_t id, size_t pass_index, size_t pass_count_val)
{
    std::lock_guard lock { sampler_registry_->mutex };
    for (auto const& entry : sampler_registry_->entries)
    {
        if (entry.id != id)
            continue;

        auto const& glsl = entry.passes[pass_index];
        if (pass_index == pass_count_val - 1)
        {
            // Final pass: use the full alpha+SDF wrapper via compile_fragment_shader.
            // Cast away the lock — compile_fragment_shader has its own compilation_mutex.
            auto* prog = &dynamic_cast<Program&>(compile_fragment_shader(
                reinterpret_cast<void const*>((uintptr_t(id) << 8) | pass_index),
                "uniform sampler2D tex;\nuniform sampler2D tex_source;\n",
                glsl.c_str()));
            return prog;
        }
        else
        {
            void const* key = reinterpret_cast<void const*>((uintptr_t(id) << 8) | pass_index);
            auto* pp = &compile_intermediate_pass(key, glsl.c_str());
            return pp;
        }
    }
    throw std::runtime_error("Unknown custom sampler id");
}

GLuint miracle::ProgramFactory::compile_shader(GLenum type, GLchar const* src)
{
    GLuint id = glCreateShader(type);
    if (!id)
    {
        throw std::runtime_error("Failed to create shader");
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
        throw std::runtime_error(std::string("Compile failed: ") + log + " for:\n" + src);
    }
    return id;
}

miracle::ProgramHandle miracle::ProgramFactory::link_shader(
    ShaderHandle const& vertex_shader,
    ShaderHandle const& fragment_shader)
{
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
        throw std::runtime_error(std::string("Linking GL shader failed: ") + log);
    }

    return program;
}
