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

#ifndef MIRACLE_PRIMITIVE_RENDERER_H
#define MIRACLE_PRIMITIVE_RENDERER_H

#include <GLES2/gl2.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace miracle
{
struct Vertex2d
{
    glm::vec2 position;
    glm::vec4 color;
};

struct Mesh2d
{
    std::vector<Vertex2d> vertices;
    std::vector<uint32_t> indices;

    GLuint VBO = 0;
    GLuint EBO = 0;

    void uploadToGPU();
    void draw() const;
    void destroy();
};

struct Model2d
{
    std::vector<Mesh2d> meshes;

    void uploadToGPU();
    void draw() const;
    void destroy();
};

class Shader2d
{
public:
    Shader2d();
    ~Shader2d();
    void use() const;
    GLuint getProgram() const;
    Mesh2d createRectangle(glm::vec2 position, glm::vec2 size, glm::vec4 color);
    Model2d createBorders(glm::vec2 position, glm::vec2 size, float borderWidth, glm::vec4 color);
    void setViewport(float x, float y, float width, float height);

private:
    GLuint programID;

    GLuint compileShader(GLenum type, const std::string& source);
    void checkCompileErrors(GLuint shader, const std::string& type);
};

} // miracle

#endif // MIRACLE_PRIMITIVE_RENDERER_H
