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

#ifndef MIR_GL_PRIMITIVE_H_
#define MIR_GL_PRIMITIVE_H_

#include <GLES2/gl2.h>

namespace mir
{
namespace gl
{
    struct Vertex
    {
        GLfloat position[3];
        GLfloat texcoord[2];
    };

    struct Primitive
    {
        enum
        {
            max_vertices = 4
        };

        Primitive() :
            type(GL_TRIANGLE_FAN),
            nvertices(4)
        {
            // Default is a quad. Just need to assign vertices[] and tex_id.
        }

        GLenum type; // GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_TRIANGLES etc
        int nvertices;
        Vertex vertices[max_vertices];
    };
}
}
#endif /* MIR_GL_PRIMITIVE_H_ */
