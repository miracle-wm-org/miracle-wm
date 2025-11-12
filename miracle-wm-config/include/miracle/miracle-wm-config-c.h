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

#ifndef MIRACLE_WM_CONFIG_C_H
#define MIRACLE_WM_CONFIG_C_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /// The level of an error.
    typedef enum
    {
        /// A warning level.
        MIRACLE_CONFIG_ERROR_LEVEL_WARNING,

        /// An error level.
        MIRACLE_CONFIG_ERROR_LEVEL_ERROR
    } miracle_config_error_level_t;

    /// Describes an error encountered while loading the configuration.
    typedef struct
    {
        /// The line at which the error occurred.
        int line;

        /// The column at which the error occurred.
        int column;

        /// The level of the error.
        miracle_config_error_level_t level;

        /// The file that the error originated in.
        const char* filename;

        /// A message describing what wrent wrong.
        const char* message;
    } miracle_config_error_t;

    /// An opaque pointer to the configuration.
    ///
    /// Callers should use the provided C methods to access information
    /// from the configuration.
    typedef struct
    {
        void* _internal; // Opaque pointer to ConfigData
    } miracle_config_data_t;

    /// Created by calling #miracle_config_load.
    ///
    /// Callers may use this to access the loaded config in addition to information
    /// about what went wrong during loading, if anything.
    typedef struct
    {
        /// The miracle configuration.
        miracle_config_data_t config;

        /// An opaque pointer to the underlying data.
        void* ptr;
    } miracle_config_load_result_t;

    /// Created by calling #miracle_config_save.
    typedef struct
    {
        /// `true` if the save was a success, otherwise `false`.
        bool success;

        /// An opaque pointer to information related to save success.
        void* ptr;
    } miracle_config_save_result_t;

    /// A generic construct used to describe an option for a given type in miracle.
    ///
    /// Callers will often use the `miracle_config_get_*_options_count` and
    /// `miracle_config_get_*_option` functions to retrieve instance of this object.
    typedef struct
    {
        /// The display name of the option.
        ///
        /// This is intended to be displayed by UIs.
        const char* name;

        /// The value of the option.
        ///
        /// This is intended to be used in setters on the miracle config API.
        uint value;
    } miracle_config_option_t;

    /// Returns a pointer to the loaded #miracle_config_data_t from a #miracle_config_load_result_t.
    ///
    /// \param result a load result
    /// \returnes the config data
    const miracle_config_data_t* miracle_config_get_data(const miracle_config_load_result_t* result);

    /// Retrieve the number of possible modifier options.
    ///
    /// Modifiers may be retrieved using #miracle_config_get_modifier_option.
    ///
    /// \returns number of possible modifier options
    uint miracle_config_get_modifier_options_count();

    /// Retrieve a modifier option at a particular index.
    ///
    /// Providing an index greater than #miracle_config_get_modifier_options_count results in
    /// undefined behavior.
    ///
    /// \param i the provided index
    /// \returns the option at the provided index
    miracle_config_option_t miracle_config_get_modifier_option(uint i);

    /// Retrieve the number of options for mouse buttons.
    ///
    /// Each mouse button option may be retrieved with #miracle_config_get_mouse_button_option.
    ///
    /// \returns the number of mouse button options
    uint miracle_config_get_mouse_button_options_count();

    /// Retrieve a mouse button option at a particular index.
    ///
    /// Providing an index greater than #miracle_config_get_mouse_button_options_count results in
    /// undefined behavior.
    ///
    /// \param i the provided index
    /// \returns the option at the provided index
    miracle_config_option_t miracle_config_get_mouse_button_option(uint i);

    /// Retrieve the number of options for mouse actions.
    ///
    /// Each mouse action option may be retrieved with #miracle_config_get_mouse_actions_option.
    ///
    /// \returns the number of mouse action options
    uint miracle_config_get_mouse_actions_options_count();

    /// Retrieve a mouse action option at a particular index.
    ///
    /// Providing an index greater than #miracle_config_get_mouse_actions_options_count results in
    /// undefined behavior.
    ///
    /// \param i the provided index
    /// \returns the option at the provided index
    miracle_config_option_t miracle_config_get_mouse_actions_option(uint i);

    /// Retrieve the number of options for keyboard actions.
    ///
    /// Each mouse action option may be retrieved with #miracle_config_get_keyboard_actions_option.
    ///
    /// \returns the number of mouse action options
    uint miracle_config_get_keyboard_actions_options_count();

    /// Retrieve a keyboard action option at a particular index.
    ///
    /// Providing an index greater than #miracle_config_get_keyboard_actions_options_count results in
    /// undefined behavior.
    ///
    /// \param i the provided index
    /// \returns the option at the provided index
    miracle_config_option_t miracle_config_get_keyboard_actions_option(uint i);
    uint miracle_config_get_built_in_key_command_options_count();
    miracle_config_option_t miracle_config_get_built_in_key_command_option(uint i);
    uint miracle_config_get_animation_type_options_count();
    miracle_config_option_t miracle_config_get_animation_type_option(uint i);
    uint miracle_config_get_ease_function_options_count();
    miracle_config_option_t miracle_config_get_ease_function_option(uint i);
    uint miracle_config_get_layout_options_count();
    miracle_config_option_t miracle_config_get_layout_option(uint i);
    uint miracle_config_get_handedness_options_count();
    miracle_config_option_t miracle_config_get_handedness_option(uint i);
    uint miracle_config_get_acceleration_options_count();
    miracle_config_option_t miracle_config_get_acceleration_option(uint i);

    /// Retrieve the standard miracle config path.
    ///
    /// This is typically located at `~/.config/miracle-wm/config.yaml`.
    ///
    /// \returns the standard miracle config path
    const char* miracle_config_path();

    /// Load the miracle configuration from the provided path.
    ///
    /// \param path path to the file
    /// \returns a load result
    miracle_config_load_result_t* miracle_config_load(const char* path);

    /// Returns the number of errors found in the #miracle_config_load_result_t.
    ///
    /// Individual errors may be retrieved via #miracle_config_get_error.
    ///
    /// \param result a load result
    /// \returns the number of errors
    size_t miracle_config_get_error_count(const miracle_config_load_result_t* result);

    /// Retrieve an error from the #miracle_config_load_result_t.
    ///
    /// Use #miracle_config_get_error_count to see how many errors are available.
    ///
    /// \param result a load result
    /// \param index the error
    /// \returns a pointer to an error, or `NULL` if the index does not exist.
    const miracle_config_error_t* miracle_config_get_error(
        const miracle_config_load_result_t* result,
        size_t index);

    /// Frees a #miracle_config_load_result_t.
    ///
    /// \param result result to free
    void miracle_config_free(miracle_config_load_result_t* result);

    /// Saves the provided \p config to the given \p path.
    ///
    /// \param path the path to save to
    /// \param config the configuration to save
    /// \returns a save result
    miracle_config_save_result_t* miracle_config_save(const char* path, const miracle_config_data_t* config);

    /// Retrieve the number of errors in a #miracle_config_save_result_t.
    ///
    /// Use #miracle_save_result_get_error to access each error.
    ///
    /// \param result the save result
    /// \returns the number of errors
    size_t miracle_save_result_get_error_count(const miracle_config_save_result_t* result);

    /// Retrieve an error from the #miracle_config_save_result_t.
    ///
    /// Use #miracle_save_result_get_error_count to see how many errors are available.
    ///
    /// \param result a save result
    /// \param index the error
    /// \returns a pointer to an error, or `NULL` if the index does not exist.
    const miracle_config_error_t* miracle_save_result_get_error(
        const miracle_config_save_result_t* result,
        size_t index);

    /// Frees the memory allocated by #miracle_config_save.
    ///
    /// \param result to free
    void miracle_save_result_free(miracle_config_save_result_t* result);

    /// Retrieve the number of includes that this configuration refers to.
    ///
    /// Use #miracle_config_get_include to access each include.
    ///
    /// \param config the config
    /// \returns the number of includes
    size_t miracle_config_get_num_includes(const miracle_config_data_t* config);

    /// Retrieve an "include" by index.
    ///
    /// An "include" is another path in the system that contains another configuration
    /// file for miracle. Users may use includes to compose their configuration out of
    /// multiple files
    ///
    /// Use #miracle_config_get_num_includes to get the number of includes available.
    ///
    /// \param config the config
    /// \param index the index
    /// \returns a path to another include
    const char* miracle_config_get_include(const miracle_config_data_t* config, size_t index);

    /// Add a new include to the configuration.
    ///
    /// \param config the config
    /// \param value the new include
    /// \param index the index to add it at
    void miracle_config_add_include(const miracle_config_data_t* config, const char* value, size_t index);

    /// Remove an include from the configuration.
    ///
    /// \param config the config
    /// \param index the index to remove
    void miracle_config_remove_include(const miracle_config_data_t* config, size_t index);

    /// Set an include at a particular index.
    ///
    /// \param config the config
    /// \param value the value to set
    /// \param index the index to modify
    void miracle_config_set_include(const miracle_config_data_t* config, const char* value, size_t index);

    /// Retrieve the primary modifier.
    ///
    /// The modifier will be one of those found by calling #miracle_config_get_modifier_option.
    ///
    /// \param config the config
    /// \returns the modifier value
    uint miracle_config_get_primary_modifier(const miracle_config_data_t* config);

    /// Set the primary modifier.
    ///
    /// The modifier should be one of those found by calling #miracle_config_get_modifier_option.
    ///
    /// \param config the config
    /// \param modifier the new modifier
    void miracle_config_set_primary_modifier(miracle_config_data_t* config, uint modifier);

    /// Retrieve the primary mouse button.
    ///
    /// The mouse button will be one of those found by calling #miracle_config_get_mouse_button_option.
    ///
    /// \param config the config
    /// \returns the primary mouse button value
    uint miracle_config_get_primary_button(const miracle_config_data_t* config);

    /// Sets the primary mouse button.
    ///
    /// The mouse button should be one of those found by calling #miracle_config_get_mouse_button_option.
    ///
    /// \param config the config
    /// \param button the primary mouse button value
    void miracle_config_set_primary_button(miracle_config_data_t* config, uint button);

    /// Retrieve the inner gaps of the \p config in the horizontal direction.
    ///
    /// \returns the horizontal inner gaps
    uint32_t miracle_config_get_inner_gaps_x(const miracle_config_data_t* config);

    /// Set the horizontal inner gaps.
    ///
    /// \param config the config
    /// \param value the horizontal inner gaps
    void miracle_config_set_inner_gaps_x(miracle_config_data_t* config, uint32_t value);

    /// Retrieve the inner gaps of the \p config in the vertical direction.
    ///
    /// \returns the vertical inner gaps
    uint32_t miracle_config_get_inner_gaps_y(const miracle_config_data_t* config);

    /// Set the vertical inner gaps.
    ///
    /// \param config the config
    /// \param value the vertical inner gaps
    void miracle_config_set_inner_gaps_y(miracle_config_data_t* config, uint32_t value);

    /// Retrieve the outer gaps of the \p config in the horizontal direction.
    ///
    /// \returns the horizontal outer gaps
    uint32_t miracle_config_get_outer_gaps_x(const miracle_config_data_t* config);

    /// Set the horizontal outer gaps.
    ///
    /// \param config the config
    /// \param value the horizontal outer gaps
    void miracle_config_set_outer_gaps_x(miracle_config_data_t* config, uint32_t value);

    /// Retrieve the outer gaps of the \p config in the vertical direction.
    ///
    /// \returns the vertical outer gaps
    uint32_t miracle_config_get_outer_gaps_y(const miracle_config_data_t* config);

    /// Set the vertical outer gaps.
    ///
    /// \param config the config
    /// \param value the vertical outer gaps
    void miracle_config_set_outer_gaps_y(miracle_config_data_t* config, uint32_t value);

    /// Retrieve the resize jump.
    ///
    /// \param config the config
    /// \returns the resize jump
    int miracle_config_get_resize_jump(const miracle_config_data_t* config);

    /// Set the resize jump.
    ///
    /// \param config the config
    /// \param value the jump in pixels
    void miracle_config_set_resize_jump(miracle_config_data_t* config, int value);

    /// Retrieve whether animations are enabled.
    ///
    /// \param config the config
    /// \returns `true` if enabled, otherwise `false`
    bool miracle_config_get_animations_enabled(const miracle_config_data_t* config);

    /// Set whether or not animations are enabled.
    ///
    /// \param config the config
    /// \param enabled the new anbled status
    void miracle_config_set_animations_enabled(miracle_config_data_t* config, bool enabled);

    /// Get the terminal command as a string.
    ///
    /// \param config the config
    /// \returns the terminal command
    const char* miracle_config_get_terminal(const miracle_config_data_t* config);

    /// Set the terminal command.
    ///
    /// \param config the config
    /// \param terminal the terminal command as a string.
    void miracle_config_set_terminal(miracle_config_data_t* config, const char* terminal);

    /// Describes a custom keybind action.
    typedef struct
    {
        /// The keyboard action that triggers this command.
        ///
        /// Use #miracle_config_get_keyboard_actions_option to list the options.
        uint action;

        /// A bit field describing the modifiers that must be held for this keybind
        /// to be triggered.
        ///
        /// Use #miracle_config_get_modifier_option to list the options.
        uint modifiers;

        /// The input event code that must be acted on to trigger the action.
        ///
        /// See https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h for the options.
        uint key;

        /// The command that will be triggered by this key combination.
        const char* command;
    } miracle_custom_key_command_t;

    /// Retrieve the number of custom keybinds in the \p config.
    ///
    /// Use #miracle_config_get_custom_key_command to retrieve the key commands.
    ///
    /// \param config the config
    /// \returns the number of custom keybinds
    size_t miracle_config_get_custom_key_command_count(const miracle_config_data_t* config);

    /// Retrieve a custom keybind by index.
    ///
    /// Use #miracle_config_get_custom_key_command_count to retrieve the number of key commands
    /// that are available.
    ///
    /// \param config the config
    /// \paramm index the index of the key command
    /// \returns the custom key command
    miracle_custom_key_command_t miracle_config_get_custom_key_command(
        const miracle_config_data_t* config,
        size_t index);

    /// Add a custom keybind.
    ///
    /// \param config the config
    /// \param key_command the keybind
    void miracle_config_add_custom_key_command(
        miracle_config_data_t* config,
        miracle_custom_key_command_t* key_command);

    /// Modify a custom keybind.
    ///
    /// \param config the config
    /// \param index the index to change
    /// \param key_command the keybind
    void miracle_config_edit_custom_key_command(
        miracle_config_data_t* config,
        size_t index,
        miracle_custom_key_command_t* key_command);

    /// Remove the custom key command by \p index.
    ///
    /// \param config the config
    /// \param index the index
    /// \returns `true` if it was removed, otherwise `false`
    bool miracle_config_remove_custom_key_command(miracle_config_data_t* config, size_t index);

    /// Describes an internal keybind that is overridden by this keybind.
    typedef struct
    {
        /// The keyboard action that triggers this command.
        ///
        /// Use #miracle_config_get_keyboard_actions_option to list the options.
        uint action;

        /// A bit field describing the modifiers that must be held for this keybind
        /// to be triggered.
        ///
        /// Use #miracle_config_get_modifier_option to list the options.
        uint modifiers;

        /// The input event code that must be acted on to trigger the action.
        ///
        /// See https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h for the options.
        uint key;

        /// The command that is being overridden.
        ///
        /// Use #miracle_config_get_built_in_key_command_option to get the built-in key command options.
        uint command;
    } miracle_built_in_key_command_override_t;

    /// Retrieve the number of built-in keybind overrides.
    ///
    /// \param config the config
    /// \returns the number of built-in keybind overrides
    size_t miracle_config_get_built_in_key_command_override_count(const miracle_config_data_t* config);

    /// Retrieve the built-in key command override at a particular \p index.
    ///
    /// \param config the config
    /// \param index the index
    /// \returns the build in key command override
    miracle_built_in_key_command_override_t miracle_config_get_built_in_key_command_override(
        const miracle_config_data_t* config,
        size_t index);

    /// Add a built-in keybind override.
    ///
    /// \param config the config
    /// \param key_command_override the override
    void miracle_config_add_built_in_key_command_override(
        miracle_config_data_t* config,
        miracle_built_in_key_command_override_t* key_command_override);

    /// Modify a built-in keybind override at a particular \p index.
    ///
    /// \param config the config
    /// \param index the index
    /// \param key_command_override the override
    void miracle_config_set_built_in_key_command_override(
        miracle_config_data_t* config,
        size_t index,
        miracle_built_in_key_command_override_t* key_command_override);

    /// Remove a built-in key command override at a particular \p index.
    ///
    /// \param config the config
    /// \param index the index
    /// \returns `true` if successfully removed, otherwise `false`.
    bool miracle_config_remove_built_in_key_command_override(
        const miracle_config_data_t* config,
        size_t index);

    /// Represents an application that will start when the compositor is ready for client
    /// connections.
    typedef struct
    {
        /// The command to execute.
        const char* command;

        /// If `true`, the command will be rerun if it returns a non-zero exit code.
        bool restart_on_death;

        /// Specifies that the app should receive no startup ID.
        bool no_startup_id;

        /// If `true`, the compositor will exit when this program exits.
        bool should_halt_compositor_on_death;

        /// If `true`, the #command will be run in systemd's scope.
        bool in_systemd_scope;
    } miracle_startup_app_t;

    /// Retrieve the number of startup applications.
    ///
    /// \param config the config
    /// \returns the number of startup applications
    size_t miracle_config_get_startup_app_count(const miracle_config_data_t* config);

    /// Retrieve a startup application at a particular \p index.
    ///
    /// \param config the config
    /// \param index the index
    /// \returns a startup app
    miracle_startup_app_t miracle_config_get_startup_app(const miracle_config_data_t* config, size_t index);

    /// Add a new startup app.
    ///
    /// \param config the config
    /// \param startup_app the new startup app
    void miracle_config_add_startup_app(
        miracle_config_data_t* config,
        miracle_startup_app_t* startup_app);

    /// Modify the startup app at a particular \p index.
    ///
    /// \param config the config
    /// \param index the index to modify
    /// \param startup_app the startup app
    void miracle_config_set_startup_app(
        miracle_config_data_t* config,
        size_t index,
        miracle_startup_app_t* startup_app);

    /// Remove a startup app at a particular index.
    ///
    /// \param config the config
    /// \param index the index to remove
    bool miracle_config_remove_startup_app(miracle_config_data_t* config, size_t index);

    /// Describes an environment variable that will be set when the compositor starts.
    typedef struct
    {
        /// The key for the environment variable.
        const char* key;

        /// The value for the environment variable.
        const char* value;
    } miracle_environment_variable_t;

    /// Retrieve the number of environment variables.
    ///
    /// \param config the config
    /// \returns the number of environment variables set in the config
    size_t miracle_config_get_environment_variable_count(const miracle_config_data_t* config);

    /// Retrieve an environment variable at a particular \p index.
    ///
    /// \param config the config
    /// \param index the index
    /// \returns the environment variable
    miracle_environment_variable_t miracle_config_get_environment_variable(
        const miracle_config_data_t* config,
        size_t index);

    /// Add an environment variable.
    ///
    /// \param config the config
    /// \param variable the environment variable to add
    void miracle_config_add_environment_variable(
        miracle_config_data_t* config,
        miracle_environment_variable_t* variable);

    /// Modify an environment variable.
    ///
    /// \param config the config
    /// \param index to modify
    /// \param variable new variable
    void miracle_config_set_environment_variable(
        miracle_config_data_t* config,
        size_t index,
        miracle_environment_variable_t* variable);

    /// Remove an environment variable.
    ///
    /// \param config the config
    /// \param index to remove
    /// \returns `true` if removed, otherwise `false`
    bool miracle_config_remove_environment_variable(miracle_config_data_t* config, size_t index);

    // The configuration for window borders.
    typedef struct
    {
        /// The size of the border in pixels.
        int size;

        /// The radius of the border in pixels.
        float radius;

        /// The RGBA color of the border when focused.
        float focus_color[4];

        /// The RGBA color of the border when not focused.
        float color[4];
    } miracle_border_config_t;

    /// Retrieve the border config.
    ///
    /// \param config the config
    /// \returns the border config
    miracle_border_config_t miracle_config_get_border_config(const miracle_config_data_t* config);

    /// Modify the border config.
    ///
    /// \param config the config
    /// \param border_config the new border config
    void miracle_config_set_border_config(
        miracle_config_data_t* config,
        miracle_border_config_t* border_config);

    /// Describes an animateable event in miracle.
    ///
    /// Miracle provides animations at key points in the lifecycles of
    /// windows, workspaces, and more.
    typedef struct
    {
        /// The name of the animateable event.
        ///
        /// This CANNOT be changed by the user.
        const char* name;

        /// If `true`, this event has not been altered from its default yet.
        bool is_default;

        /// The duration of the animation in seconds.
        float duration_seconds;

        /// The number of animations that are run concurrently when this event happens.
        ///
        /// Each animateable event is built from a number of animations
        /// that run concurrent to each other. This enables the user to
        /// combine animations in fun and unique ways.
        ///
        /// Use #miracle_animateable_event_get_animation to get an animation at a particular
        /// index. There exists other methods to update each animation and add more as well.
        size_t num_animations;

        /// Opaque pointer.
        void* _internal;
    } miracle_animateable_event_t;

    /// Describes an animation.
    typedef struct
    {
        /// The animation type.
        ///
        /// Use #miracle_config_get_animation_type_option to get the available
        /// animation types.
        uint type;

        /// The animation easing function.
        ///
        /// Use #miracle_config_get_ease_function_option to get the available
        /// easing functiont ypes.
        uint function;

        /// The c1 variable of easing functions.
        ///
        /// See https://easings.net/.
        float c1;

        /// The c2 variable of easing functions.
        ///
        /// See https://easings.net/.
        float c2;

        /// The c3 variable of easing functions.
        ///
        /// See https://easings.net/.
        float c3;

        /// The c4 variable of easing functions.
        ///
        /// See https://easings.net/.
        float c4;

        /// The c5 variable of easing functions.
        ///
        /// See https://easings.net/.
        float c5;

        /// The n1 variable of easing functions.
        ///
        /// See https://easings.net/.
        float n1;

        /// The d1 variable of easing functions.
        ///
        /// See https://easings.net/.
        float d1;
    } miracle_built_in_animation_t;

    /// Retrieve the number of animateable events.
    ///
    /// \returns the number of animateable events
    size_t miracle_config_get_animateable_event_count();

    /// Retrieve an animateable event at a particular \p index.
    ///
    /// \param config the config
    /// \param index the index
    /// \returns the animateable event definition
    miracle_animateable_event_t miracle_config_get_animateable_event(
        miracle_config_data_t* config,
        size_t index);

    /// Modify an animateable event at a particular \p index.
    ///
    /// \param config the config
    /// \param index the index
    /// \param definition the animateable event definition
    void miracle_config_set_animateable_event(
        miracle_config_data_t* config,
        size_t index,
        const miracle_animateable_event_t* definition);

    /// Reset the animateable event at \p index to its default.
    ///
    /// \param config the config
    /// \param index the index
    void miracle_config_reset_animation_definition(
        miracle_config_data_t* config,
        size_t index);

    /// Retrieve an animation from an animateable event by \p index.
    ///
    /// \param animateable_event the animateable event
    /// \param index the index
    /// \returns the animation at the index
    miracle_built_in_animation_t miracle_animateable_event_get_animation(
        miracle_animateable_event_t* animateable_event,
        size_t index);

    /// Add a new animation to the animateable event.
    ///
    /// \param animateable_event the animateable event
    /// \param animation the new animation
    void miracle_animateable_event_add_animation(
        miracle_animateable_event_t* animateable_event,
        miracle_built_in_animation_t animation);

    /// Modify an animation at a particular \p index.
    ///
    /// \param animateable_event the animateable event
    /// \param index the index
    /// \param animation the modification
    void miracle_animateable_event_set_animation(
        miracle_animateable_event_t* animateable_event,
        size_t index,
        miracle_built_in_animation_t animation);

    /// Remove an animation from an animateable event.
    ///
    /// \param animateable_event the event
    /// \param index to remove
    void miracle_animateable_event_remove_animation(
        miracle_animateable_event_t* animateable_event,
        size_t index);

    /// Describes a configuration for a workspace.
    typedef struct
    {
        /// `true` if #num is set, otherwise `false`.
        bool has_num;

        /// The workspace number associated with this configuration.
        ///
        /// Either #num or #name must be set for the configuration to be
        /// valid.
        int num;

        /// `true` if #name is set, otherwise `false`.
        bool has_name;

        /// The workspace name associated with this configuration.
        ///
        /// Either #num or #name must be set for the configuration to be
        /// valid.
        const char* name;

        /// `true` if #layout_strategy is set, otherwise `false`.
        bool has_layout_strategy;

        /// The layout strategy for windows on this workspace.
        ///
        /// This strategy decides how new windows are placed for this workspace.
        ///
        /// Callers may use #miracle_config_get_layout_option and #miracle_config_get_layout_options_count
        /// to list the available layout strategies.
        ///
        /// Defaults to "tiling".
        int layout_strategy;
    } miracle_workspace_config_t;

    /// Retrieve the number of workspace configurations.
    ///
    /// \returns the number of workspace configs
    size_t miracle_config_get_workspace_config_count(const miracle_config_data_t* config);

    /// Retrieve a workspace configuration at a particular \p index.
    ///
    /// \param config the config
    /// \param index the index
    /// \returns the workspace config
    miracle_workspace_config_t miracle_config_get_workspace_config(
        const miracle_config_data_t* config,
        size_t index);

    /// Add a new workspace config.
    ///
    /// \param config the config
    /// \param workspace_config the new workspace config
    void miracle_config_add_workspace_config(
        miracle_config_data_t* config,
        miracle_workspace_config_t* workspace_config);

    /// Modify a workspace config.
    ///
    /// \param config the config
    /// \param index the index
    /// \param workspace_config the modification
    void miracle_config_set_workspace_config(
        miracle_config_data_t* config,
        size_t index,
        miracle_workspace_config_t* workspace_config);

    /// Remove a workspace config.
    ///
    /// \param config the config
    /// \param index to remove
    bool miracle_config_remove_workspace_config(
        miracle_config_data_t* config,
        size_t index);

    /// Get the move modifier key.
    ///
    /// \param config the config
    /// \returns the move modifier
    uint miracle_config_get_move_modifier(const miracle_config_data_t* config);

    /// Set the move modifier key
    ///
    /// \param config the config
    /// \param modifier the new modifier
    void miracle_config_set_move_modifier(miracle_config_data_t* config, uint modifier);

    // The drag and drop configuration.
    typedef struct
    {
        /// Whether or not drag and drop is enabled.
        bool enabled;

        /// The modifiers that must be held for drag and drop to work.
        uint modifiers;
    } miracle_drag_and_drop_config_t;

    /// Retrieve the drag and drop config.
    ///
    /// \param config the config
    /// \returns the drag and drop config
    miracle_drag_and_drop_config_t miracle_config_get_drag_and_drop(const miracle_config_data_t* config);

    /// Modify the drag and drop config.
    ///
    /// \param config the config
    /// \param drag_and_drop_config the modified drag and drop config
    void miracle_config_set_drag_and_drop(
        miracle_config_data_t* config,
        miracle_drag_and_drop_config_t* drag_and_drop_config);

    // The mouse configuration.
    typedef struct
    {
        /// The handedness of the mouse.
        ///
        /// Use #miracle_config_get_handedness_option to list the options.
        uint handedness;

        /// The acceleration bias of the mouse.
        ///
        /// This is a number between -1 and 1.
        ///
        /// Defaults to 0.
        double acceleration_bias;

        /// The vertical scroll speed of the mouse.
        ///
        /// This is a number between 0 and 1.
        ///
        /// Defaults to 1.
        double vscroll_speed;

        /// The horizontal scroll speed of the mouse.
        ///
        /// This is a number between 0 and 1.
        ///
        /// Defaults to 1.
        double hscroll_speed;

        /// The acceleration profile of the mouse.
        ///
        /// Use #miracle_config_get_acceleration_option to list the options.
        uint acceleration;
    } miracle_mouse_config_t;

    /// Retrieve the mouse config.
    ///
    /// \param config the config
    /// \returns the mouse config
    miracle_mouse_config_t miracle_config_get_mouse_config(const miracle_config_data_t* config);

    /// Modify the mouse config.
    ///
    /// \param config the config
    /// \param mouse_config the new mouse config
    void miracle_config_set_mouse_config(
        miracle_config_data_t* config,
        miracle_mouse_config_t* mouse_config);

    /// Defines the touchpad configuration.
    typedef struct
    {
        /// Whether to disable the touchpad while typing.
        ///
        /// Defaults to false.
        bool disable_while_typing;

        /// Whether to disable the touchpad when an external mouse is connected.
        ///
        /// Defaults to false.
        bool disable_with_external_mouse;

        /// The acceleration bias of the touchpad.
        ///
        /// This is a number between -1 and 1.
        ///
        /// Defaults to 0.
        float acceleration_bias;

        /// The vertical scroll speed of the touchpad.
        ///
        /// Defaults to 1.
        float vscroll_speed;

        /// The horizontal scroll speed of the touchpad.
        ///
        /// Defaults to 1.
        float hscroll_speed;

        /// Whether tap-to-click is enabled.
        ///
        /// Defaults to true.
        bool tap_to_click;

        /// Whether middle mouse button emulation is enabled.
        ///
        /// Defaults to false.
        bool middle_mouse_button_emulation;
    } miracle_touchpad_config_t;

    /// Retrieve the touchpad config.
    ///
    /// \param config the config
    /// \returns the touchpad config
    miracle_touchpad_config_t miracle_config_get_touchpad_config(const miracle_config_data_t* config);

    /// Modify the touchpad config.
    ///
    /// \param config the config
    /// \param touchpad_config the new touchpad config
    void miracle_config_set_touchpad_config(
        miracle_config_data_t* config,
        miracle_touchpad_config_t* touchpad_config);

    /// Defines the keymap for the keyboard.
    typedef struct
    {
        /// If `true`, this means that the keymap is used.
        bool is_set;

        /// The language for the keymap.
        const char* language;

        /// Whether the [variant] is set.
        bool has_variant;

        /// The variant.
        const char* variant;

        /// The number of options specified on this keymap.
        ///
        /// Use #miracle_config_get_keymap_option to retrieve the option at a particular index.
        size_t options_count;
    } miracle_keymap_t;

    /// Retrieve the keymap.
    ///
    /// \param config the config
    /// \returns the keymap
    miracle_keymap_t miracle_config_get_keymap(const miracle_config_data_t* config);

    /// Modify the keymap.
    ///
    /// \param config the config
    /// \param keymap the keymap modification
    void miracle_config_set_keymap(
        miracle_config_data_t* config,
        miracle_keymap_t* keymap);

    /// Retrieve an option from the keymap.
    ///
    /// \param config the config
    /// \param index the index
    /// \returns the option
    const char* miracle_config_get_keymap_option(const miracle_config_data_t* config, size_t index);

    /// Modify an option from the keymap.
    ///
    /// \param config the config
    /// \param index to modify
    /// \param option new value
    void miracle_config_set_keymap_option(
        miracle_config_data_t* config,
        size_t index,
        const char* option);

    /// Add an option to the keymap.
    ///
    /// \param config the config
    /// \param option the new option
    void miracle_config_add_keymap_option(miracle_config_data_t* config, const char* option);

    /// Remove an option from the keymap.
    ///
    /// \param config the config
    /// \param index the index
    void miracle_config_remove_keymap_option(miracle_config_data_t* config, size_t index);

    /// Retrieve the repeat delay for the keyboard.
    ///
    /// This is the delay in milliseconds since the previous key down.
    ///
    /// \param config the config
    /// \returns the key repeate delay
    int miracle_config_get_key_repeat_delay(const miracle_config_data_t* config);

    /// Set the repeat delay for the keyboard.
    ///
    /// The delay is in milliseconds since the previous key down.
    ///
    /// \param config the config
    /// \param delay the new delay
    void miracle_config_set_key_repeat_delay(miracle_config_data_t* config, int delay);

    /// Retrieve the repeat rate for the keyboard.
    ///
    /// This is expressed in "characters per second".
    ///
    /// \param config the config
    /// \returns the repeat rate
    int miracle_config_get_key_repeat_rate(const miracle_config_data_t* config);

    /// Set the repeat rate for the keyboard.
    ///
    /// This is expressed in "characters per second".
    ///
    /// \param config the config
    /// \param rate the new rate
    void miracle_config_set_key_repeat_rate(miracle_config_data_t* config, int rate);

    /// Defines the hover click configuration.
    typedef struct
    {
        /// Whether hover click is enabled.
        bool enabled;

        /// The length of time that the pointer must stay still in order to dispatch
        /// a left click.
        ///
        /// Defaults to 1000ms.
        uint hover_duration_milliseconds;

        /// The distance in pixels that the pointer mut move from the initial hover click
        /// position to cancel it.
        ///
        /// Defaults to 10px.
        int cancel_displacement_threshold;

        /// The distance in pixels that the pointer must move from the last hover click
        /// or hover click cancel position to initiate a new hover click.
        ///
        /// Defaults to 5px.
        int reclick_displacement_threshold;
    } miracle_hover_click_t;

    /// Retrieve the hover click config.
    ///
    /// \param config the config
    /// \returns the hover click config
    miracle_hover_click_t miracle_config_get_hover_click(const miracle_config_data_t* config);

    /// Modify the hover click config.
    ///
    /// \param config the config
    /// \param hover_click the new hover click config
    void miracle_config_set_hover_click(
        miracle_config_data_t* config,
        miracle_hover_click_t* hover_click);

    /// Defines the simulated secondary click configuration
    typedef struct
    {
        /// Whether simulated secondary click is enabled.
        bool enabled;

        /// The length of time that the user must hold down the left pointer button
        /// to dispatch a secondary click.
        ///
        /// Defaults to 1000ms.
        uint hold_duration_milliseconds;

        /// The distance in pixels that the pointer can move before the secondary
        /// click is cancelled.
        ///
        /// Defaults to 20px.
        int displacement_threshold;
    } miracle_simulated_secondary_click_t;

    /// Retrieve the simulated secondary click config.
    ///
    /// \param config the config
    /// \returns the simulated secondary click config
    miracle_simulated_secondary_click_t miracle_config_get_simulated_secondary_click(miracle_config_data_t const* config);

    /// Modify the simulated secondary click config.
    ///
    /// \param config the config
    /// \param simulated_secondary_click the modification
    void miracle_config_set_simulated_secondary_click(
        miracle_config_data_t* config,
        miracle_simulated_secondary_click_t* simulated_secondary_click);

    /// Defines the output filter used by miracle.
    typedef struct
    {
        /// Whether #shader_path is enabled or not.
        bool shader_path_enabled;

        /// A shader path. This can start with a tilde that will resolve to the home directory.
        const char* shader_path;
    } miracle_output_filter_t;

    /// Retrieve the output filter.
    ///
    /// \param config the config
    /// \returns the output filter config
    miracle_output_filter_t miracle_config_get_output_filter(const miracle_config_data_t* config);

    /// Modify the output filter.
    ///
    /// \param config the config
    /// \param output_filter the output filter
    void miracle_config_set_output_filter(
        miracle_config_data_t* config,
        miracle_output_filter_t* output_filter);

    /// Defines the cursor properties.
    typedef struct
    {
        /// The scale of the cursor.
        ///
        /// Defaults to 1.
        float scale;

        /// How to focus windows with the cursor.
        ///
        /// Defaults to 0 (focus on hover)
        uint focus_mode; // miracle::CursorFocusMode as uint
    } miracle_cursor_t;

    /// Retrieve the cursor config.
    ///
    /// \param config the config
    /// \returns the cursor
    miracle_cursor_t miracle_config_get_cursor(const miracle_config_data_t* config);

    /// Modify the cursor config.
    ///
    /// \param config the config
    /// \param cursor the cursor
    void miracle_config_set_cursor(miracle_config_data_t* config, miracle_cursor_t* cursor);

    /// Defines the slow keys accessibility feature.
    typedef struct
    {
        /// Whether slow keys is enabled or not.
        ///
        /// Defaults to `false`.
        bool enabled;

        /// Time before a key press is registered, in milliseconds.
        ///
        /// Defaults to 200ms.
        uint hold_duration_milliseconds;
    } miracle_slow_keys_t;

    /// Retrieve the slow keys configuration.
    ///
    /// \param config the config
    /// \returns the slow keys config
    miracle_slow_keys_t miracle_config_get_slow_keys(const miracle_config_data_t* config);

    /// Modify the slow keys configuration.
    ///
    /// \param config the config
    /// \param slow_keys the slow keys conifg
    void miracle_config_set_slow_keys(miracle_config_data_t* config, miracle_slow_keys_t* slow_keys);

    typedef struct
    {
        /// Whether sticky keys is enabled or not.
        ///
        /// Defaults to `false`.
        bool enabled;

        /// When set to true, depressing two modifier keys simultaneously will result
        /// in sticky keys being temporarily disabled until all keys are released.
        ///
        /// Defaults to `true`.
        bool should_disable_if_two_keys_are_pressed_together;
    } miracle_sticky_keys_t;

    /// Retrieve the sticky key config.
    ///
    /// \param config the config
    /// \returns the sticky keys config
    miracle_sticky_keys_t miracle_config_get_sticky_keys(const miracle_config_data_t* config);

    /// Modify the sticky keys config.
    ///
    /// \param config the config
    /// \param sticky_keys the sticky keys config
    void miracle_config_set_sticky_keys(miracle_config_data_t* config, miracle_sticky_keys_t* sticky_keys);

    /// Defines the parameters for the magnifier accessibility feature.
    typedef struct
    {
        /// Whether the magnifier is enabled by default.
        ///
        /// Defaults to `false`.
        bool enabled;

        /// The default scale of the magnifier.
        ///
        /// Defaults to 1.5.
        float scale;

        /// The scale increment of the magnifier.
        ///
        /// Defaults to 0.5.
        float scale_increment;

        /// The default width of the magnifier.
        ///
        /// Defaults to 400px.
        int width;

        /// The default height of the magnifier.
        ///
        /// Defaults to 400px.
        int height;

        /// The size increment of the magnifier.
        ///
        /// Defaults to 50px.
        int size_increment;
    } miracle_magnifier_t;

    /// Retrieve the magnifier config.
    ///
    /// \param config the config
    /// \returns the magnifier config
    miracle_magnifier_t miracle_config_get_magnifier(const miracle_config_data_t* config);

    /// Modify the magnifier.
    ///
    /// \param config the config
    /// \param magnifier the magnifier config
    void miracle_config_set_magnifier(miracle_config_data_t* config, miracle_magnifier_t magnifier);

#ifdef __cplusplus
}
#endif

#endif // MIRACLE_WM_CONFIG_C_H
