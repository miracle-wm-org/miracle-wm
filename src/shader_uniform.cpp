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

#include "shader_uniform.h"

#include <algorithm>
#include <cstring>

using namespace miracle;

namespace
{
/// Uniforms that the compositor sets on the programs it draws windows with.
constexpr std::string_view reserved_names[] = {
    "tex",
    "tex_source",
    "center",
    "display_transform",
    "workspace_transform",
    "transform",
    "screen_to_gl_coords",
    "alpha",
    "surfaceSize",
    "border_transform",
    "borderColor",
    "borderRadius",
    "borderWidth",
    "u_world_to_clip",
    "u_window_rect",
    "u_window_size",
};

bool is_identifier_start(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool is_identifier_char(char c)
{
    return is_identifier_start(c) || (c >= '0' && c <= '9');
}
}

std::optional<ShaderUniform::Type> ShaderUniform::type_from(uint32_t raw)
{
    if (raw > static_cast<uint32_t>(Type::mat4))
        return std::nullopt;
    return static_cast<Type>(raw);
}

size_t ShaderUniform::value_size(Type type)
{
    switch (type)
    {
    case Type::float1:
        return sizeof(float);
    case Type::vec2:
        return 2 * sizeof(float);
    case Type::vec3:
        return 3 * sizeof(float);
    case Type::vec4:
        return 4 * sizeof(float);
    case Type::int1:
        return sizeof(int32_t);
    case Type::mat4:
        return 16 * sizeof(float);
    }
    return 0;
}

bool ShaderUniform::is_valid_name(std::string_view name)
{
    if (name.empty() || name.size() > max_name_length)
        return false;
    if (!is_identifier_start(name.front()) || !std::all_of(name.begin(), name.end(), is_identifier_char))
        return false;
    if (name.starts_with("gl_"))
        return false;
    return std::find(std::begin(reserved_names), std::end(reserved_names), name) == std::end(reserved_names);
}

ShaderUniform ShaderUniform::from_bytes(std::string name, Type type, void const* value)
{
    ShaderUniform uniform { .name = std::move(name), .type = type };
    if (type == Type::int1)
        std::memcpy(&uniform.integer, value, sizeof(int32_t));
    else
        std::memcpy(uniform.floats.data(), value, value_size(type));
    return uniform;
}
