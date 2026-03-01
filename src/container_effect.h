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

#ifndef MIRACLE_CONTAINER_EFFECT_H
#define MIRACLE_CONTAINER_EFFECT_H
#include <glm/fwd.hpp>

namespace miracle
{
struct ContainerEffect
{
public:
    ContainerEffect blend(ContainerEffect const& other);
    float alpha = 1.f;
    glm::mat4 transform = glm::mat4(1.f);
};
}

#endif