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

#define MIR_LOG_COMPONENT "immediate_renderer"
#include "immediate_renderer.h"
#include <mir/log.h>
#include <glm/gtc/type_ptr.hpp>

using namespace miracle;

namespace
{
const GLchar* const vertex_shader_src = R"(
attribute vec2 position;
attribute vec4 color;

uniform mat4 view;
uniform mat4 projection;
uniform mat4 model;

varying vec4 v_color;

void main() {
   gl_Position = view * projection * model * vec4(position.x, position.y, 0, 1);
   v_color = color;
}
)";

const GLchar* const fragment_shader_src = R"(
#ifdef GL_ES
precision mediump float;
#endif

varying vec4 v_color;

void main() {
   gl_FragColor = v_color;
}
)";

static GLuint load_shader(const char *src, GLenum type)
{
    GLuint shader = glCreateShader(type);
    if (shader)
    {
        GLint compiled;
        glShaderSource(shader, 1, &src, NULL);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled)
        {
            GLchar log[1024];
            glGetShaderInfoLog(shader, sizeof log - 1, NULL, log);
            log[sizeof log - 1] = '\0';
            mir::log_error("immediate_rendererer::load_shader compile failed: %s", log);
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}
}

Mesh::Mesh(std::vector<Vertex> const& vertices_, std::vector<GLuint> indices_)
    : vertices{vertices_},
      indices{indices_}
{
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
}

void Mesh::render() const
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)0);

    // Color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)offsetof(Vertex, color));

    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, nullptr);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}

Mesh Mesh::create_rectangle(glm::vec4 color)
{
    const std::vector<Vertex> vertex_buffer_data
    {
        { glm::vec2(-1.f, -1.f), color },
        { glm::vec2(1.f, -1.f), color },
        { glm::vec2(1.f, 1.f), color },
        { glm::vec2(-1.f, 1.f), color }
    };
    const std::vector<GLuint> index_buffer_data
    {
        0, 1, 2,
        0, 2, 3
    };
    return { vertex_buffer_data, index_buffer_data };
}

Mesh::~Mesh()
{
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);
}

Model::Model(const std::vector<Mesh> &meshes)
    : meshes{meshes}
{
    transform = glm::scale(transform, glm::vec3(0.5f));
}

void Model::render() const
{
    for (auto const& mesh : meshes)
        mesh.render();
}

ImmediateRenderer::ImmediateRenderer()
{
    GLuint vertex_shader = 0;
    vertex_shader = load_shader(vertex_shader_src, GL_VERTEX_SHADER);
    assert(vertex_shader);

    GLuint fragment_shader = 0;
    fragment_shader = load_shader(fragment_shader_src, GL_FRAGMENT_SHADER);
    assert(fragment_shader);

    program_id = glCreateProgram();
    assert(program_id);
    glAttachShader(program_id, vertex_shader);
    glAttachShader(program_id, fragment_shader);
    glLinkProgram(program_id);

    int success;
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (!success)
    {
        mir::log_error("immediate_renderer: failed to link");
        return;
    }

    glBindAttribLocation(program_id, 0, "position");
    glBindAttribLocation(program_id, 1, "color");
    attributes.position = glGetAttribLocation(program_id, "position");
    attributes.color = glGetAttribLocation(program_id, "color");
    uniforms.model = glGetUniformLocation(program_id, "model");
    uniforms.view = glGetUniformLocation(program_id, "view");
    uniforms.projection = glGetUniformLocation(program_id, "projection");
    glEnableVertexAttribArray(attributes.position);
    glEnableVertexAttribArray(attributes.color);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

ImmediateRenderer::~ImmediateRenderer()
{
    glDeleteProgram(program_id);
}

void ImmediateRenderer::use()
{
    glUseProgram(program_id);
}

void ImmediateRenderer::render(miracle::Model const& model)
{
    use();
    glUniformMatrix4fv(uniforms.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(uniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(uniforms.model, 1, GL_FALSE, glm::value_ptr(model.transform));
    model.render();
    glUseProgram(0);
}