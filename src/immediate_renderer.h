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

#ifndef MIRACLE_WM_IMMEDIATE_RENDERER_H
#define MIRACLE_WM_IMMEDIATE_RENDERER_H

#include <glm/glm.hpp>
#include <vector>
#include <GLES2/gl2.h>

namespace miracle
{

struct Vertex
{
    glm::vec2 position;
    glm::vec4 color;
};

struct Mesh
{
public:
    Mesh(std::vector<Vertex> const& vertices, std::vector<GLuint> indices);
    ~Mesh();

    static Mesh create_rectangle(glm::vec4 color);

    void render() const;
private:
    GLuint vbo = 0;
    GLuint ebo = 0;
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
};

struct Model
{
public:
    explicit Model(std::vector<Mesh> const& meshes);
    void render() const;
    glm::mat4 transform = glm::mat4(1.f);

private:
    std::vector<Mesh> meshes;
};

/// A renderer for non-surface scene effects (e.g. particles, borders, etc.)
class ImmediateRenderer
{
public:
    ImmediateRenderer();
    ~ImmediateRenderer();

    void use();
    void render(Model const&);
    void set_view(glm::mat4 const&);
    void set_projection(glm::mat4 const&);

private:
    GLuint program_id = 0;
    struct {
        GLint position = 0;
        GLint color = 0;
    } attributes;
    struct {
        GLint view = 0;
        GLint projection = 0;
        GLint model = 0;
    } uniforms;
    glm::mat4 view = glm::mat4(1.f);
    glm::mat4 projection = glm::mat4(1.f);
};

}

#endif //MIRACLE_WM_IMMEDIATE_RENDERER_H
