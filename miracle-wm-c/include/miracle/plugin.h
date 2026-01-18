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

    /// Provides context for a call from the plugin system into
    /// Miracle's internals. Plugin authors must supply this as a
    /// parameter when they want to query the system.
    typedef struct
    {
        /// Opaque pointer to internal data.
        void* internal;
    } miracle_context_t;

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

        /// Pointer to internal data.
        ///
        /// Please do not use unless you plan to be very sneaky!
        void* internal;
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

    /// Describes a workspace.
    typedef struct
    {
        /// If `TRUE`, the workspace is set.
        ///
        /// This will be `FALSE` for windows that have not yet been placed.
        int32_t is_set;

        /// If `TRUE`, #number is set.
        int32_t has_number;

        /// The number of the workspace.
        ///
        /// Only valid if #has_number is `TRUE`.
        uint32_t number;

        /// If `TRUE`, #name is set.
        int32_t has_name;

        /// The name of the workspace.
        ///
        /// Only valid if #has_name is `TRUE`.
        const char* name;

        /// Pointer to internal data.
        ///
        /// Please do not use unless you plan to be very sneaky.
        void* internal;
    } miracle_workspace_t;

    /// Describes an output.
    typedef struct
    {
        /// If `TRUE`, the output is set, otherwise `FALSE`.
        int32_t is_set;

        /// The position of the output.
        miracle_point_t position;

        /// The size of the output.
        miracle_size_t size;

        /// The name of the output.
        const char* name;

        /// If `TRUE`, the output is the primary output, otherwise `FALSE`.
        int32_t is_primary;

        /// Pointer to internal data.
        ///
        /// Please do not use unless you plan to be very sneaky.
        void* internal;
    } miracle_output_t;

    /// Retrieve the #miracle_application_info for a given window.
    ///
    /// \param context the context
    /// \param window_info the window info
    /// \returns the application info for the window
    miracle_application_info_t miracle_plugin_get_application(miracle_context_t* context, miracle_window_info_t* window_info);

    /// Retrieve the #miracle_workspace_t of that the window is on.
    ///
    /// If the window has yet to be placed, this will represent the tenative workspace.
    /// \param context the context
    /// \param window_info the window info
    /// \returns the workspace that the window is on
    miracle_workspace_t miracle_plugin_get_workspace_from_window(miracle_context_t* context, miracle_window_info_t* window_info);

    /// Retrieve the #miracle_output_t that a window is on.
    ///
    /// \param context the context
    /// \param workspace a workspace
    /// \returns the output to which the workspace belongs
    miracle_output_t miracle_plugin_get_output_from_workspace(miracle_context_t* context, miracle_workspace_t* workspace);

    /// Retrive the number of outputs.
    ///
    /// \param context the context
    /// \returns the number of outputs
    uint32_t miracle_plugin_num_outputs(miracle_context_t* context);

    /// Retrive an output by the \p index.
    ///
    /// Outputs appear in no specific order. Querying an index beyond #miracle_plugin_num_outputs
    /// is undefined.
    ///
    /// \param context the context
    /// \param index the index
    /// \returns the output at the index
    miracle_output_t miracle_plugin_get_output(miracle_context_t* context, uint32_t index);
#ifdef __cplusplus
}
#endif

#endif // MIRACLE_WM_PLUGIN_H
