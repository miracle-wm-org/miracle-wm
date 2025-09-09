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

#include "shader_2d.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <iostream>

using namespace miracle;

// Vertex shader as a string literal
const char* vertexShaderSource = R"(
attribute vec2 aPos;
attribute vec4 aColor;

uniform mat4 uProjection;
uniform mat4 uModel;
uniform mat4 uMesh;

varying vec4 vertexColor;

void main() {
    gl_Position = uProjection * uModel * uMesh * vec4(aPos, 0.0, 1.0);
    vertexColor = aColor;
}
)";

// Fragment shader as a string literal
const char* fragmentShaderSource = R"(
#ifdef GL_ES
precision mediump float;
#endif
varying vec4 vertexColor;

void main() {
    gl_FragColor = vertexColor;
}
)";

Shader2d::Shader2d()
{
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    // Check linking errors
    GLint success;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success)
    {
        GLchar infoLog[512];
        glGetProgramInfoLog(programID, 512, nullptr, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                  << infoLog << std::endl;
    }

    // Delete the shaders as they're linked into our program now and no longer needed
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader2d::~Shader2d()
{
    glDeleteProgram(programID);
}

void Shader2d::use() const
{
    glUseProgram(programID);
}

GLuint Shader2d::getProgram() const
{
    return programID;
}

GLuint Shader2d::compileShader(GLenum type, const std::string& source)
{
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    checkCompileErrors(shader, (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT");

    return shader;
}

void Shader2d::checkCompileErrors(GLuint shader, const std::string& type)
{
    GLint success;
    GLchar infoLog[512];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                      << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
        }
    }
}

Mesh2d Shader2d::createRectangle(glm::vec2 position, glm::vec2 size, glm::vec4 color)
{
    Mesh2d mesh;

    glm::vec2 bottomLeft = position;
    glm::vec2 bottomRight = position + glm::vec2(size.x, 0.0f);
    glm::vec2 topLeft = position + glm::vec2(0.0f, size.y);
    glm::vec2 topRight = position + size;

    mesh.vertices = {
        { bottomLeft,  color },
        { bottomRight, color },
        { topRight,    color },
        { topLeft,     color }
    };

    mesh.indices = {
        0, 1, 2, // First triangle
        2, 3, 0 // Second triangle
    };

    return mesh;
}

Model2d Shader2d::createBorders(glm::vec2 position, glm::vec2 size, float borderWidth, glm::vec4 color)
{
    Model2d model;

    glm::vec2 pos = position;
    glm::vec2 totalSize = size;

    // Bottom border
    model.meshes.push_back(createRectangle(
        { pos.x, pos.y },
        { totalSize.x, borderWidth },
        color));

    // Top border
    model.meshes.push_back(createRectangle(
        { pos.x, pos.y + totalSize.y - borderWidth },
        { totalSize.x, borderWidth },
        color));

    // Left border
    model.meshes.push_back(createRectangle(
        { pos.x, pos.y + borderWidth },
        { borderWidth, totalSize.y - 2 * borderWidth },
        color));

    // Right border
    model.meshes.push_back(createRectangle(
        { pos.x + totalSize.x - borderWidth, pos.y + borderWidth },
        { borderWidth, totalSize.y - 2 * borderWidth },
        color));

    return model;
}

void Shader2d::setViewport(float x, float y, float width, float height)
{
    glm::mat4 const projection = glm::ortho(
        x, x + width, // Left, Right
        y, y + height, // Bottom, Top
        -1.0f, 1.0f // Near, Far
    );
    auto const projectionLoc = glGetUniformLocation(programID, "uProjection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);
}

void Mesh2d::uploadToGPU()
{
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex2d)), vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)), indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh2d::draw(Shader2d const& shader) const
{
    auto const mesh_loc = glGetUniformLocation(shader.getProgram(), "uMesh");
    glUniformMatrix4fv(mesh_loc, 1, GL_FALSE, &transform[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // Set up vertex attributes again (no VAO to save them!)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2d), (const void*)offsetof(Vertex2d, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2d), (const void*)offsetof(Vertex2d, color));

    // Draw
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);

    // Optionally disable attributes again (good practice)
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Mesh2d::destroy()
{
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Model2d::uploadToGPU()
{
    for (auto& mesh : meshes)
    {
        mesh.uploadToGPU();
    }
}

void Model2d::draw(Shader2d const& shader) const
{
    auto const model_loc = glGetUniformLocation(shader.getProgram(), "uModel");
    glUniformMatrix4fv(model_loc, 1, GL_FALSE, &transform[0][0]);
    for (const auto& mesh : meshes)
    {
        mesh.draw(shader);
    }
}

void Model2d::destroy()
{
    for (auto& mesh : meshes)
    {
        mesh.destroy();
    }
}

void Model2d::set_color(glm::vec4 const& color)
{
    for (auto& mesh : meshes)
    {
        for (auto& vertex : mesh.vertices)
        {
            vertex.color = color;
        }

        glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(Vertex2d)), mesh.vertices.data());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}
