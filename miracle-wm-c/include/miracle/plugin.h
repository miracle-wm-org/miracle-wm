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

    typedef enum
    {
        miracle_animation_type_window_open,

        miracle_animation_type_window_close,

        miracle_animation_type_window_move,

        miracle_animation_type_workspace_switch,

        miracle_animation_type_window_none
    } miracle_animation_type;

    typedef struct
    {
        /// Animation type.
        ///
        /// This one of #miracle_animation_type.
        uint32_t type;

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
    /// Plugin authors may use #miracle_plugin_get_application to get the application
    /// given a window.
    typedef struct
    {
        /// The name of the application.
        const char* application_name;

        /// Internal data.
        uint64_t internal;
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
        ///
        /// If the window has not yet been placed, this will be arbitrary.
        miracle_point_t top_left;

        /// The size of the window.
        miracle_size_t size;

        /// The depth layer of the window.
        MirDepthLayer depth_layer;

        /// Pointer to internal data.
        ///
        /// Please do not use unless you plan to be very sneaky!
        uint64_t internal;

        /// The 4x4 transform matrix of the window (column-major).
        ///
        /// Defaults to the identity matrix.
        float transform[16];

        /// The alpha (opacity) of the window.
        ///
        /// Defaults to 1.0 (fully opaque).
        float alpha;
    } miracle_window_info_t;

    /// The type of the container.
    typedef enum miracle_container_type
    {
        /// The container has a single window in it.
        miracle_container_type_window,

        /// The container has multiple children in it.
        miracle_container_type_parent
    } miracle_container_type;

    /// Describes a layout for a container.
    typedef enum miracle_layout_scheme
    {
        /// None layout.
        miracle_layout_scheme_none,

        /// A horizontal layout.
        miracle_layout_scheme_horizontal,

        /// A vertical layout.
        miracle_layout_scheme_vertical,

        /// A tabbed layout.
        miracle_layout_scheme_tabbed,

        /// A stacked layout.
        miracle_layout_scheme_stacking
    } miracle_layout_scheme;

    /// Describes a container in a tree.
    ///
    /// A container may either a parent or a window. Parent containers
    /// have children, which can be retrieved via #miracle_plugin_get_child_from_container.
    ///
    /// The window of a window container can be retrieved via
    /// #miracle_plugin_get_window_info_from_container.
    typedef struct
    {
        /// The type of the container.
        ///
        /// This is a #miracle_container_type.
        uint32_t type;

        /// If `TRUE`, the container is floating within its workspace.
        ///
        /// This is only set if #type is #miracle_container_type_parent.
        int32_t is_floating;

        /// Describes how a container is laying out its content.
        ///
        /// This is #miracle_layut_scheme.
        ///
        /// This is only set if #type is #miracle_container_type_parent.
        uint32_t layout_scheme;

        /// The number of child containers inside of this container.
        ///
        /// Use #miracle_plugin_get_child_from_container to query the container by index.
        uint32_t num_child_containers;

        /// Pointer to internal data.
        ///
        /// Please do not use unless you plan to be very sneaky!
        uint64_t internal;
    } miracle_container_t;

    /// Describes a workspace.
    typedef struct
    {
        /// If `TRUE`, the workspace is valid.
        ///
        /// This may be `FALSE` for shell components that are not tethered to a particular
        /// workspace.
        int32_t is_set;

        /// If `TRUE`, #number is set.
        int32_t has_number;

        /// The number of the workspace.
        ///
        /// Only valid if #has_number is `TRUE`.
        uint32_t number;

        /// If `TRUE`, #name is set.
        int32_t has_name;

        /// The number of container trees in this workspace.
        ///
        /// Use #miracle_plugin_get_workspace_tree to get the tree at a particular index.
        /// Each tree is represented by a #miracle_container_t which is the root of the tree.
        uint32_t num_trees;

        /// Pointer to internal data.
        ///
        /// Please do not use unless you plan to be very sneaky.
        uint64_t internal;
    } miracle_workspace_t;

    /// Describes an output.
    typedef struct
    {
        /// The position of the output.
        miracle_point_t position;

        /// The size of the output.
        miracle_size_t size;

        /// If `TRUE`, the output is the primary output, otherwise `FALSE`.
        int32_t is_primary;

        /// The number of workspaces on this output.
        uint32_t num_workspaces;

        /// Pointer to internal data.
        ///
        /// Please do not use unless you plan to be very sneaky.
        uint64_t internal;
    } miracle_output_t;

    /// Describes the placement strategy for a window.
    ///
    /// This is used by #miracle_placement_t.
    typedef enum miracle_window_management_strategy_t
    {
        /// If selected, the plugin lets miracle place the window according
        /// to its own strategy.
        miracle_window_management_strategy_system,

        /// Describes a window that will be placed in the tiling grid.
        miracle_window_management_strategy_tiled,

        /// Describes a window whose behavior is entirely determined by
        /// the plugin.
        miracle_window_management_strategy_freestyle
    } miracle_window_management_strategy_t;

    /// Describes a tiled placement which is controlled by the plugin system.
    typedef struct
    {
        /// The ID of the parent container that this window should be placed inside.
        ///
        /// If the container has #miracle_container_t::type of #miracle_container_type_window, then
        /// the #layout_scheme will be applied to that window to form a new
        /// parent before placing the window at the #index.
        ///
        /// If the container has #miracle_container_t::type of #miracle_container_type_parent, then
        /// the #layout_scheme will be ignored and the window will be placed at
        /// the #index.
        ///
        /// If this is 0, then it is assumed to be null.
        uint64_t parent_internal;

        /// The index at which this container will be placed within the parent.
        uint32_t index;

        /// The requested layout scheme of the new parent.
        ///
        /// This will only be used if #miracle_container_type_parent has #miracle_container_t::type of #miracle_container_type_parent.
        miracle_layout_scheme layout_scheme;
    } miracle_tiled_placement_t;

    /// Describes a freestyle placement which is fully controlled by the plugin.
    typedef struct
    {
        /// The top left position of the window.
        miracle_point_t top_left;

        /// The depth layer of the window.
        ///
        /// Plugin authors are encouraged to use #miracle_window_info_t::depth_layer
        /// unless they would like to force the window into a different depth for
        /// whatever reason.
        uint32_t depth_layer;

        /// The workspace that this window should be placed on.
        ///
        /// If `0`, the window will always be shown.
        ///
        /// Defaults to the currently selected workspace.
        uint64_t workspace_internal;

        /// The size of the window.
        ///
        /// This value may not be honored by the window itself.
        miracle_size_t size;

        /// The 4x4 transform matrix applied to the window (column-major).
        ///
        /// Defaults to the identity matrix.
        float transform[16];

        /// The alpha (opacity) of the window.
        ///
        /// Defaults to 1.0 (fully opaque).
        float alpha;
    } miracle_freestyle_placement_t;

    typedef struct
    {
        /// The placement strategy for this window.
        ///
        /// This is miracle_window_management_strategy_t.
        ///
        /// Defaults to the #miracle_window_management_strategy_system.
        uint32_t strategy;

        /// The freestyle placement strategy.
        ///
        /// This is only honored if #strategy is #miracle_window_management_strategy_freestyle.
        miracle_freestyle_placement_t freestyle_placement;

        /// The titled placement strategy.
        ///
        /// This is only honored if #strategy is #miracle_window_management_strategy_tiled.
        miracle_tiled_placement_t tiled_placement;
    } miracle_placement_t;

    /// Describes a keyboard event.
    typedef struct
    {
        /// The keyboard action.
        uint32_t action;

        /// The keysym.
        uint32_t keysym;

        /// The raw scan code.
        int32_t scan_code;

        /// The modifiers held during the event.
        uint32_t modifiers;
    } miracle_keyboard_event_t;

    /// Set the state of a plugin-managed window.
    ///
    /// \param window_internal the internal pointer from #miracle_window_info_t::internal
    /// \param state the new window state (a #MirWindowState value)
    /// \returns 0 on success, -1 on error
    int32_t miracle_window_set_state(int64_t window_internal, int32_t state);

    /// Move a plugin-managed window to a different workspace.
    ///
    /// \param window_internal the internal pointer from #miracle_window_info_t::internal
    /// \param workspace_internal the internal pointer from #miracle_workspace_t::internal
    /// \returns 0 on success, -1 on error
    int32_t miracle_window_set_workspace(int64_t window_internal, int64_t workspace_internal);

    /// Resize a plugin-managed window.
    ///
    /// \param window_internal the internal pointer from #miracle_window_info_t::internal
    /// \param width the new width in pixels
    /// \param height the new height in pixels
    /// \returns 0 on success, -1 on error
    int32_t miracle_window_set_size(int64_t window_internal, int32_t width, int32_t height);

    /// Set the 4x4 column-major transform matrix of a plugin-managed window.
    ///
    /// \param window_internal the internal pointer from #miracle_window_info_t::internal
    /// \param transform pointer to 16 contiguous floats (column-major matrix)
    /// \returns 0 on success, -1 on error
    int32_t miracle_window_set_transform(int64_t window_internal, const float* transform);

    /// Set the alpha (opacity) of a plugin-managed window.
    ///
    /// \param window_internal the internal pointer from #miracle_window_info_t::internal
    /// \param alpha pointer to the new alpha value in [0, 1]
    /// \returns 0 on success, -1 on error
    int32_t miracle_window_set_alpha(int64_t window_internal, const float* alpha);

#ifdef __cplusplus
}
#endif

#endif // MIRACLE_WM_PLUGIN_H
