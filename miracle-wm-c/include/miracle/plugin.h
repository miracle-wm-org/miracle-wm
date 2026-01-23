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
    /// Plugin authors may use #miracle_plugin_get_application to get the application
    /// given a window.
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
        ///
        /// If the window has not yet been placed, this will be arbitrary.
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
        enum miracle_container_type type;

        /// If `TRUE`, the container is floating within its workspace.
        ///
        /// This is only set if #type is #miracle_container_type_parent.
        int32_t is_floating;

        /// Describes how a container is laying out its content.
        ///
        /// This is only set if #type is #miracle_container_type_parent.
        miracle_layout_scheme layout_scheme;

        /// The number of child containers inside of this container.
        ///
        /// Use #miracle_plugin_get_child_from_container to query the container by index.
        uint32_t num_child_containers;

        /// Pointer to internal data.
        ///
        /// Please do not use unless you plan to be very sneaky!
        void* internal;
    } miracle_container_t;

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

        /// The number of container trees in this workspace.
        ///
        /// Use #miracle_plugin_get_workspace_tree to get the tree at a particular index.
        /// Each tree is represented by a #miracle_container_t which is the root of the tree.
        uint32_t num_trees;

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

    /// Describes the placement strategy for a window.
    ///
    /// This is used by #miracle_placement_t.
    typedef enum miracle_window_management_strategy_t
    {
        /// Describes a window that will be placed in the tiling grid.
        tiled,

        /// Describes a window whose behavior is entirely determined by
        /// the plugin.
        freestyle
    } miracle_window_management_strategy_t;

    /// Describes a tiled placement which is controlled by the plugin system.
    typedef struct
    {
        /// The parent container that this window should be placed inside.
        ///
        /// If the container has #miracle_container_t::type of #miracle_container_type_window, then
        /// the #layout_scheme will be applied to that window to form a new
        /// parent before placing the window at the #index.
        ///
        /// If the container has #miracle_container_t::type of #miracle_container_type_parent, then
        /// the #layout_scheme will be ignored and the window will be placed at
        /// the #index.
        miracle_container_t parent;

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
        MirDepthLayer depth_layer;

        /// The workspace that this window should be placed on.
        ///
        /// If `NULL`, the window will always be shown.
        ///
        /// Defaults to the currently selected workspace.
        miracle_workspace_t* workspace;

        /// The size of the window.
        ///
        /// This value may not be honored by the window itself.
        miracle_size_t size;
    } miracle_freestyle_placement_t;

    typedef struct
    {
        /// The placement strategy for this window.
        ///
        /// Defaults to the inherited value from the active workspace, which is
        /// most likely #tiled.
        miracle_window_management_strategy_t strategy;

        /// The freestyle placement strategy.
        ///
        /// This is only honored if #strategy is #freestyle.
        miracle_freestyle_placement_t freestyle_placement;

        /// The titled placement strategy.
        ///
        /// This is only honored if #strategy is #tiled.
        miracle_tiled_placement_t titled_placement;
    } miracle_placement_t;

    /// Retrieve the #miracle_application_info for a given window.
    ///
    /// \param context the context
    /// \param window_info the window info
    /// \returns the application info for the window
    miracle_application_info_t miracle_window_info_get_application(miracle_context_t* context, miracle_window_info_t* window_info);

    /// Retrieve the #miracle_workspace_t of that the window is on.
    ///
    /// If the window has yet to be placed, this will represent the tentative workspace.
    ///
    /// \param context the context
    /// \param window_info the window info
    /// \returns the workspace that the window is on
    miracle_workspace_t miracle_window_info_get_workspace(miracle_context_t* context, miracle_window_info_t* window_info);

    /// Retrieve the #miracle_output_t that a window is on.
    ///
    /// \param context the context
    /// \param workspace a workspace
    /// \returns the output to which the workspace belongs
    miracle_output_t miracle_workspace_get_output(miracle_context_t* context, miracle_workspace_t* workspace);

    /// Retrieve the number of outputs.
    ///
    /// \param context the context
    /// \returns the number of outputs
    uint32_t miracle_get_outputs(miracle_context_t* context);

    /// Retrieve an output by the \p index.
    ///
    /// Outputs appear in no specific order. Querying an index beyond #miracle_plugin_num_outputs
    /// is undefined.
    ///
    /// \param context the context
    /// \param index the index
    /// \returns the output at the index
    miracle_output_t miracle_get_output_at(miracle_context_t* context, uint32_t index);

    /// Retrieve a tree by the \p index.
    ///
    /// Trees appear in no specific order. Querying an index beyond #miracle_workspace_t::num_trees
    /// is undefined.
    ///
    /// Each workspace is guaranteed to return at least one tree, but they may return more if there are
    /// floating trees associated with the workspace.
    ///
    /// \param context the context
    /// \param workspace the workspace
    /// \param index the index
    /// \returns the container at the index
    miracle_container_t miracle_workspace_get_tree(miracle_context_t* context, miracle_workspace_t* workspace, uint32_t index);

    /// Retrieve a child container from a parent \p container.
    ///
    /// Children will be returned in the order that they appear in the \p container. Querying an index beyond
    /// #miracle_container_t::num_trees is undefined.
    ///
    /// This is only defined when the \p container has #miracle_container_t::type of #miracle_container_type::parent.
    ///
    /// \param context the context
    /// \param container the container
    /// \param index the index
    /// \returns the child container at the index
    miracle_container_t miracle_container_get_child_at(miracle_context_t* context, miracle_container_t* container, uint32_t index);

    /// Retrieve window information from a window \p container.
    ///
    /// This is only defined when the \p container has #miracle_container_t::type of #miracle_container_type::window.
    ///
    /// \param context the context
    /// \param container the container
    /// \returns the window info of the container
    miracle_window_info_t miracle_container_get_window(miracle_context_t* context, miracle_container_t* container);
#ifdef __cplusplus
}
#endif

#endif // MIRACLE_WM_PLUGIN_H
