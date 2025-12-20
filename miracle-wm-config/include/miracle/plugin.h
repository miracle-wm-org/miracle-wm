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

#ifndef MIRACLE_WM_PLUGIN_H
#define MIRACLE_WM_PLUGIN_H

#include <mir_toolkit/common.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define MIRACLE_PLUGIN_VERSION 1

    /// A 2D point with integer coordinates.
    typedef struct
    {
        /// The x coordinate.
        int32_t x;

        /// The y coordinate.
        int32_t y;
    } miracle_point_t;

    /// A size with integer dimensions.
    typedef struct
    {
        /// The width.
        int32_t w;

        /// The height.
        int32_t h;
    } miracle_size_t;

    typedef struct
    {
        /// The runtime of the animation frame in seconds.
        float runtime_seconds;

        /// The total duration of the animation in seconds.
        float duration_seconds;

        /// The origin area, packed with x, y, w, and h.
        float origin[4];

        /// The destination area, packed with x, y, w, and h.
        float destination[4];

        /// The opacity start of the animation.
        float opacity_start;

        /// The opacity end of the animation.
        float opacity_end;
    } miracle_plugin_animation_frame_data_t;

    typedef struct
    {
        /// If set to `TRUE`, the animation is considered completed.
        ///
        /// At this point, the animated object will be moved to its destination
        /// with the appropriate opacity. The animation will be removed from the
        /// system.
        int32_t completed;

        /// If `TRUE`, #area is set.
        int32_t has_area;

        /// The area as a packed rectangle of x, y, width, and height.
        ///
        /// Be careful when using this value, as Mir will set this as the _actual_
        /// rectangle of the object. For example, if setting a window's rectangle,
        /// Mir will issue a position and resize request to the window. This is
        /// NOT something that you would want to do every frame. It is better to use
        /// the #transform if you want to animate the scale.
        float area[4];

        /// If `TRUE`, #transform is set.
        int32_t has_transform;

        /// The transform to apply to the animated object.
        ///
        /// This transform is backed by a glm::mat4, which is a column-major transform.
        float transform[16];

        /// If `TRUE`, #opacity is set.
        int32_t has_opacity;

        /// The opacity of the object.
        ///
        /// This must be [0, 1].
        float opacity;
    } miracle_plugin_animation_frame_result_t;

    /// Describes the properties of an application.
    ///
    /// Plugin authors may use this information in relation to
    typedef struct
    {
        /// The name of the application.
        const char* application_name;
    } miracle_application_info_t;

    /// Describes the properties of a window.
    ///
    /// Plugin authors may use this information to decide on the placement of a window.
    ///
    /// Use #miracle_plugin_get_application to get the application from which this window
    /// originated.
    ///
    /// Use #miracle_plugin_get_workspace to get the workspace of the window.
    typedef struct
    {
        /// The type of this window.
        MirWindowType window_type;

        /// The state of the window.
        MirWindowState state;

        /// The position of the window.
        miracle_point_t top_left;

        /// The size of the window.
        miracle_size_t size;

        /// The title of the window.
        const char* title;

        /// The depth layer of the window.
        MirDepthLayer depth_layer;
    } miracle_window_info_t;

    typedef struct
    {
        /// If set to `TRUE`, the placement is set and will override Miracle's internal
        /// placement strategy.
        int32_t is_set;

        /// The top left position of the window.
        ///
        /// This value is only used if #is_set is `TRUE`.
        miracle_point_t top_left;

        /// The size of the window.
        ///
        /// This value is only used if #is_set is `TRUE`.
        ///
        /// This value may not be honored by the window itself, meaning that
        /// it will be clipped by the compositor.
        miracle_size_t size;

        /// The depth layer of the window.
        ///
        /// Plugin authors are encouraged to use #miracle_window_info_t::depth_layer
        /// unless they would like to force the window into a different depth for
        /// whatever reason.
        MirDepthLayer depth_layer;
    } miracle_placement_t;

    typedef struct
    {

    } miracle_workspace_t;

    /// Describes an output.
    typedef struct
    {
        /// The position of the output.
        miracle_point_t position;

        /// The size of the output.
        miracle_size_t size;

        /// The name of the output.
        const char* name;
    } miracle_output_t;

    /// Given the \p window_info, retrieve the #miracle_application_info.
    ///
    /// \param window_info the window info
    /// \returns the application info for the window
    miracle_application_info_t miracle_plugin_get_application(miracle_window_info_t* window_info);

    miracle_workspace_t miracle_plugin_get_workspace(miracle_window_info_t* window_info);
#ifdef __cplusplus
}
#endif

#endif // MIRACLE_WM_PLUGIN_H
