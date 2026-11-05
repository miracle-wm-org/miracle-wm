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

#ifndef MIRACLE_WM_SHADER_UNIFORM_H
#define MIRACLE_WM_SHADER_UNIFORM_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace miracle
{

/// A plugin-owned uniform that is uploaded to every program drawing a window.
struct ShaderUniform
{
    /// Matches #miracle_shader_uniform_type in plugin.h.
    enum class Type : uint32_t
    {
        float1 = 0,
        vec2 = 1,
        vec3 = 2,
        vec4 = 3,
        int1 = 4,
        mat4 = 5,
    };

    /// The most uniforms that a single window may have.
    static constexpr size_t max_per_window = 16;
    /// The longest name, in bytes, that a uniform may have.
    static constexpr size_t max_name_length = 64;

    std::string name;
    Type type = Type::float1;
    /// The value when [type] is a float type. Unused trailing values are zero.
    std::array<float, 16> floats = {};
    /// The value when [type] is [Type::int1].
    int32_t integer = 0;

    bool operator==(ShaderUniform const&) const = default;

    /// \returns the type for \p raw, or std::nullopt if it is not a known type.
    static std::optional<Type> type_from(uint32_t raw);

    /// \returns the size of a value of \p type, in bytes.
    static size_t value_size(Type type);

    /// \returns whether \p name may be set by a plugin: it must be a plain GLSL
    ///          identifier that the compositor does not set itself.
    static bool is_valid_name(std::string_view name);

    /// Build a uniform from \p value_size(type) bytes at \p value.
    static ShaderUniform from_bytes(std::string name, Type type, void const* value);
};

}

#endif // MIRACLE_WM_SHADER_UNIFORM_H
