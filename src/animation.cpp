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

#define GLM_ENABLE_EXPERIMENTAL
#define MIR_LOG_COMPONENT "animation"
#include "animation.h"
#include "geometry_helpers.h"
#include "plugin_manager.h"
#include <glm/gtx/transform.hpp>
#include <mir/log.h>

using namespace miracle;
namespace geom = mir::geometry;

namespace
{
float ease_out_bounce(BuiltInAnimationDefinition const& defintion, float x)
{
    if (x < 1 / defintion.d1)
    {
        return defintion.n1 * x * x;
    }
    else if (x < 2 / defintion.d1)
    {
        x = x - 1.5f;
        return defintion.n1 * (x / defintion.d1) * x + 0.75f;
    }
    else if (x < 2.5 / defintion.d1)
    {
        x = x - 2.25f;
        return defintion.n1 * (x / defintion.d1) * x + 0.9375f;
    }
    else
    {
        x = x - 2.625f;
        return defintion.n1 * (x / defintion.d1) * x + 0.984375f;
    }
}

float ease(BuiltInAnimationDefinition const& defintion, float t)
{
    // https://easings.net/
    switch (defintion.function)
    {
    case EaseFunction::linear:
        return t;
    case EaseFunction::ease_in_sine:
        return 1 - cosf((t * static_cast<float>(M_PI)) / 2.f);
    case EaseFunction::ease_in_out_sine:
        return -(cosf(static_cast<float>(M_PI) * t) - 1) / 2;
    case EaseFunction::ease_out_sine:
        return sinf((t * static_cast<float>(M_PI)) / 2.f);
    case EaseFunction::ease_in_quad:
        return t * t;
    case EaseFunction::ease_out_quad:
        return 1 - (1 - t) * (1 - t);
    case EaseFunction::ease_in_out_quad:
        return t < 0.5 ? 2 * t * t : 1 - powf(-2 * t + 2, 2) / 2;
    case EaseFunction::ease_in_cubic:
        return t * t * t;
    case EaseFunction::ease_out_cubic:
        return 1 - powf(1 - t, 3);
    case EaseFunction::ease_in_out_cubic:
        return t < 0.5 ? 4 * t * t * t : 1 - powf(-2 * t + 2, 3) / 2;
    case EaseFunction::ease_in_quart:
        return t * t * t * t;
    case EaseFunction::ease_out_quart:
        return 1 - powf(1 - t, 4);
    case EaseFunction::ease_in_out_quart:
        return t < 0.5 ? 8 * t * t * t * t : 1 - powf(-2 * t + 2, 4) / 2;
    case EaseFunction::ease_in_quint:
        return t * t * t * t * t;
    case EaseFunction::ease_out_quint:
        return 1 - powf(1 - t, 5);
    case EaseFunction::ease_in_out_quint:
        return t < 0.5 ? 16 * t * t * t * t * t : 1 - powf(-2 * t + 2, 5) / 2;
    case EaseFunction::ease_in_expo:
        return t == 0 ? 0 : powf(2, 10 * t - 10);
    case EaseFunction::ease_out_expo:
        return t == 1 ? 1 : 1 - powf(2, -10 * t);
    case EaseFunction::ease_in_out_expo:
        return t == 0
            ? 0
            : t == 1
            ? 1
            : t < 0.5 ? powf(2, 20 * t - 10) / 2
                      : (2 - powf(2, -20 * t + 10)) / 2;
    case EaseFunction::ease_in_circ:
        return 1 - sqrtf(1 - powf(t, 2));
    case EaseFunction::ease_out_circ:
        return sqrtf(1 - powf(t - 1, 2));
    case EaseFunction::ease_in_out_circ:
        return t < 0.5f
            ? (1 - sqrtf(1 - powf(2 * t, 2))) / 2
            : (sqrtf(1 - powf(-2 * t + 2, 2)) + 1) / 2;
    case EaseFunction::ease_in_back:
        return defintion.c3 * t * t * t - defintion.c1 * t * t;
    case EaseFunction::ease_out_back:
    {
        return 1 + defintion.c3 * powf(t - 1, 3) + defintion.c1 * powf(t - 1, 2);
    }
    case EaseFunction::ease_in_out_back:
        return t < 0.5
            ? (powf(2 * t, 2) * ((defintion.c2 + 1) * 2 * t - defintion.c2)) / 2
            : (powf(2 * t - 2, 2) * ((defintion.c2 + 1) * (t * 2 - 2) + defintion.c2) + 2) / 2;
    case EaseFunction::ease_in_elastic:
        return t == 0
            ? 0
            : t == 1
            ? 1
            : -powf(2, 10 * t - 10) * sinf((t * 10 - 10.75f) * defintion.c4);
    case EaseFunction::ease_out_elastic:
        return t == 0
            ? 0
            : t == 1
            ? 1
            : powf(2, -10 * t) * sinf((t * 10 - 0.75f) * defintion.c4) + 1;
    case EaseFunction::ease_in_out_elastic:
        return t == 0
            ? 0
            : t == 1
            ? 1
            : t < 0.5
            ? -(powf(2, 20 * t - 10) * sinf((20 * t - 11.125f) * defintion.c5)) / 2
            : (powf(2, -20 * t + 10) * sinf((20 * t - 11.125f) * defintion.c5)) / 2 + 1;
    case EaseFunction::ease_in_bounce:
        return 1 - ease_out_bounce(defintion, 1 - t);
    case EaseFunction::ease_out_bounce:
        return ease_out_bounce(defintion, t);
    case EaseFunction::ease_in_out_bounce:
        return t < 0.5
            ? (1 - ease_out_bounce(defintion, 1 - 2 * t)) / 2
            : (1 + ease_out_bounce(defintion, 2 * t - 1)) / 2;
    default:
        return 1.f;
    }
}

float interpolate_scale(float const p, float const start, float const end)
{
    float const diff = end - start;
    if (diff == 0)
        return 1.f;

    // We want to find the percentage that we should scale relative
    // to the [start] value. For example, if we are growing from 200
    // to 250, and p=0.5, then we should be at width 225, which would
    // be a scale up of 225 / 220;
    float const current = start + diff * p;
    return current / end;
}

struct SlideResult
{
    /// The current position that the surface should be in.
    /// This should also be used as the clip area position.
    glm::vec2 position;

    /// The current size of the clip area. The surface should NOT
    /// be set to this size, as it has already been set on init().
    /// This size is strictly meant for the clip area.
    glm::vec2 clip_area_size;
};

inline SlideResult slide(float p, geom::Rectangle const& from, geom::Rectangle const& to)
{
    using namespace miracle::geometry_helpers;
    auto const distance = to.top_left - from.top_left;
    float const dx = static_cast<float>(distance.dx.as_int()) * p;
    float const dy = static_cast<float>(distance.dy.as_int()) * p;

    float const clip_scale_x = interpolate_scale(p, static_cast<float>(from.size.width.as_value()), static_cast<float>(to.size.width.as_value()));
    float const clip_scale_y = interpolate_scale(p, static_cast<float>(from.size.height.as_value()), static_cast<float>(to.size.height.as_value()));

    return {
        .position = to_glm(from.top_left) + glm::vec2(dx, dy),
        .clip_area_size = to_glm(to.size) * glm::vec2(clip_scale_x, clip_scale_y)
    };
}
}

Animation::Animation(
    AnimationHandle handle,
    AnimationDefinition const& definition,
    AnimationData&& data,
    std::function<void(AnimationFrameResult const&)>&& on_tick,
    std::shared_ptr<PluginManager> const& plugin_manager) :
    handle_ { handle },
    definition_ { definition },
    data_ { std::move(data) },
    on_tick { std::move(on_tick) },
    plugin_manager { plugin_manager }
{
}

AnimationHandle Animation::handle() const
{
    return handle_;
}

void Animation::mark_for_removal()
{
    is_being_removed_ = true;
}

bool Animation::is_being_removed() const
{
    return is_being_removed_;
}

bool Animation::tick(float dt)
{
    runtime_seconds += dt;
    float const t = (runtime_seconds / definition_.duration_seconds);
    if (runtime_seconds >= definition_.duration_seconds)
    {
        on_tick(finish());
        return true;
    }

    switch (definition_.type)
    {
    case AnimationType::built_in:
    {
        AnimationFrameResult result;
        for (auto const& builtin_def : std::get<BuiltInAnimationList>(definition_.data))
            result = tick_built_in(builtin_def, t).merge(result);
        on_tick(result);
        if (result.is_complete)
            on_tick(finish());
        return result.is_complete;
    }
    case AnimationType::plugin:
    {
        // TODO: Do not resolve this every time
        auto const& plugin_def = std::get<PluginAnimationDefinition>(definition_.data);
        auto const handle = plugin_manager->get_wasm_module(plugin_def.plugin_name);
        if (handle == 0)
        {
            mir::log_error("Animation plugin failed to load: %s", plugin_def.plugin_name.c_str());
            on_tick({ true, data_.area_end, glm::mat4(1.f), data_.opacity_end });
            return true;
        }

        miracle_plugin_animation_frame_data_t frame_data;
        frame_data.runtime_seconds = runtime_seconds;
        frame_data.duration_seconds = definition_.duration_seconds;
        frame_data.origin[0] = static_cast<float>(data_.area_start.top_left.x.as_int());
        frame_data.origin[1] = static_cast<float>(data_.area_start.top_left.y.as_int());
        frame_data.origin[2] = static_cast<float>(data_.area_start.size.width.as_value());
        frame_data.origin[3] = static_cast<float>(data_.area_start.size.height.as_value());
        frame_data.destination[0] = static_cast<float>(data_.area_end.top_left.x.as_int());
        frame_data.destination[1] = static_cast<float>(data_.area_end.top_left.y.as_int());
        frame_data.destination[2] = static_cast<float>(data_.area_end.size.width.as_value());
        frame_data.destination[3] = static_cast<float>(data_.area_end.size.height.as_value());
        frame_data.opacity_start = data_.opacity_start;
        frame_data.opacity_end = data_.opacity_end;
        auto const frame_result = plugin_manager->animate_frame(handle, plugin_def.function_name, frame_data);
        AnimationFrameResult animation_result;
        animation_result.is_complete = frame_result.completed != 0;
        if (frame_result.has_area != 0)
        {
            animation_result.rectangle = geom::Rectangle {
                geom::Point { static_cast<int>(frame_result.area[0]), static_cast<int>(frame_result.area[1]) },
                geom::Size { frame_result.area[2],                   frame_result.area[3]                   }
            };
        }
        if (frame_result.has_transform != 0)
        {
            glm::mat4 transform;
            std::memcpy(&transform, frame_result.transform, sizeof(glm::mat4));
            animation_result.transform = transform;
        }
        else
            animation_result.transform = glm::mat4(1.f);
        if (frame_result.has_opacity != 0)
        {
            animation_result.opacity = frame_result.opacity;
        }
        on_tick(animation_result);
        if (animation_result.is_complete)
            on_tick(finish());
        return animation_result.is_complete;
    }
    default:
        on_tick(finish());
        return true;
    }
}

AnimationFrameResult Animation::finish() const
{
    return { true, data_.area_end, glm::mat4(1.f), data_.opacity_end };
}

AnimationFrameResult Animation::tick_built_in(BuiltInAnimationDefinition const& builtin_def, float t)
{
    switch (builtin_def.type)
    {
    case BultInAnimationType::slide:
    {
        auto const p = ease(builtin_def, t);
        const auto [position, clip_area_size] = slide(p, data_.area_start, data_.area_end);
        auto const next = geom::Rectangle {
            geom::Point { position.x,       position.y       },
            geom::Size { clip_area_size.x, clip_area_size.y }
        };
        return {
            false,
            next,
            std::nullopt,
            1.f
        };
    }
    case BultInAnimationType::grow:
    {
        auto const p = ease(builtin_def, t);
        glm::mat4 const transform = glm::scale(
            glm::mat4(1.f),
            glm::vec3(p, p, 1.f));
        return { false, std::nullopt, transform, std::nullopt };
    }
    case BultInAnimationType::shrink:
    {
        auto const p = 1.f - ease(builtin_def, t);
        glm::mat4 const transform = glm::scale(
            glm::mat4(1.f),
            glm::vec3(p, p, 1.f));
        return { false, std::nullopt, transform, std::nullopt };
    }
    case BultInAnimationType::fade:
    {
        auto const p = ease(builtin_def, t);
        float const opacity_diff = data_.opacity_end - data_.opacity_start;
        return { false, std::nullopt, std::nullopt, data_.opacity_start + opacity_diff * p };
    }
    case BultInAnimationType::disabled:
    default:
        return { true, data_.area_end, std::nullopt, std::nullopt };
    }
}
