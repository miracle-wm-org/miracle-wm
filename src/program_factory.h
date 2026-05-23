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

#ifndef MIRACLE_WM_PROGRAM_FACTORY_H
#define MIRACLE_WM_PROGRAM_FACTORY_H

#include "sampler_registry.h"

#include <GLES2/gl2.h>
#include <array>
#include <cstdint>
#include <memory>
#include <mir/graphics/program.h>
#include <mir/graphics/program_factory.h>
#include <mutex>
#include <variant>
#include <vector>

namespace miracle
{

template <void (*deleter)(GLuint)>
class GLHandle
{
public:
    explicit GLHandle(GLuint id) :
        id { id }
    {
    }

    ~GLHandle()
    {
        if (id)
            (*deleter)(id);
    }

    GLHandle(GLHandle const&) = delete;

    GLHandle& operator=(GLHandle const&) = delete;

    GLHandle(GLHandle&& from) :
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

using ProgramHandle = GLHandle<&glDeleteProgram>;
using ShaderHandle = GLHandle<&glDeleteShader>;

struct ProgramData
{
    GLuint id = 0;
    /* 8 is the minimum number of texture units a GL implementation can provide
     * and should comfortably provide enough textures for any conceivable buffer
     * format
     */
    std::array<GLint, 8> tex_uniforms;
    GLint position_attr = -1;
    GLint texcoord_attr = -1;
    GLint center_uniform = -1;
    GLint display_transform_uniform = -1;
    GLint workspace_transform_uniform = -1;
    GLint transform_uniform = -1;
    GLint screen_to_gl_coords_uniform = -1;
    GLint alpha_uniform = -1;
    GLint surface_size_uniform = -1;
    GLint border_transform_uniform = -1;
    GLint border_color_uniform = -1;
    GLint border_width_uniform = -1;
    GLint border_radius_uniform = -1;
    mutable long long last_used_frameno = 0;

    ProgramData(GLuint program_id);
};

class Program : public mir::graphics::gl::Program
{
public:
    explicit Program(ProgramHandle&& program);
    ProgramHandle program_handle;
    ProgramData data;
};

/// Lightweight program for intermediate (non-final) passes that don't need
/// the full ProgramData uniform set (alpha, transforms, SDF, etc.).
struct PassProgram
{
    explicit PassProgram(ProgramHandle&& prog);
    ProgramHandle program_handle;
    GLuint id = 0;
    GLint position_attr = -1;
    GLint texcoord_attr = -1;
    GLint tex_uniform = -1;
    GLint tex_source_uniform = -1;
    GLint surface_size_uniform = -1;
};

class ProgramFactory : public mir::graphics::gl::ProgramFactory
{
public:
    explicit ProgramFactory(std::shared_ptr<SamplerRegistry> const& sampler_registry);

    /// Creates a fragment shader for the given [id] that appends
    /// the [extension_fragment] to the top, and expects [fragment_fragment]
    /// to implement [sample_to_rgba] for that texture.
    mir::graphics::gl::Program& compile_fragment_shader(
        void const* id,
        char const* extension_fragment,
        char const* fragment_fragment) override;

    /// Resolve the program from its unique identifier.
    ///
    /// \param id the unique identifier
    /// \returns the program
    /// \throws if the identifier cannot be found
    mir::graphics::gl::Program& resolve_custom(uint8_t id);

    /// For multi-pass shaders: resolve the program for a specific pass.
    /// - final pass (pass_index == pass_count-1): returns a full Program& (alpha + SDF wrapper).
    /// - intermediate pass: returns a PassProgram& (plain gl_FragColor = sample_to_rgba wrapper).
    /// \throws if id cannot be found
    std::variant<Program*, PassProgram*> resolve_custom_pass(uint8_t id, size_t pass_index, size_t pass_count);

    /// Returns the number of passes registered for the given shader id.
    size_t pass_count(uint8_t id) const;

    /// Retrieves the border shader
    Program const& border() const { return border_program; }

    /// Register the sampler method and return its unique identifier.
    /// This returned id may be used later in "resolve" to get the shader
private:
    static GLuint compile_shader(GLenum type, GLchar const* src);
    static ProgramHandle link_shader(
        ShaderHandle const& vertex_shader,
        ShaderHandle const& fragment_shader);

    PassProgram& compile_intermediate_pass(void const* key, char const* sampler_glsl);

    std::shared_ptr<SamplerRegistry> sampler_registry_;
    ShaderHandle const vertex_shader;
    ShaderHandle const pass_vertex_shader;
    ShaderHandle const border_vertex_shader;

    ShaderHandle const border_fragment_shader;
    Program const border_program;
    std::vector<std::pair<void const*, std::unique_ptr<Program>>> programs;
    std::vector<std::pair<void const*, std::unique_ptr<PassProgram>>> pass_programs;
    // GL requires us to synchronise multi-threaded access to the shader APIs.
    std::mutex compilation_mutex;
};

} // miracle

#endif // MIRACLE_WM_PROGRAM_FACTORY_H
