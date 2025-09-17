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

#include <mir_toolkit/mir_input_device_types.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        MIRACLE_CONFIG_ERROR_LEVEL_WARNING,
        MIRACLE_CONFIG_ERROR_LEVEL_ERROR
    } miracle_config_error_level_t;

    typedef struct
    {
        int line;
        int column;
        miracle_config_error_level_t level;
        const char* filename;
        const char* message;
    } miracle_config_error_t;

    typedef struct
    {
        void* _internal; // Opaque pointer to ConfigData
    } miracle_config_data_t;

    typedef struct
    {
        miracle_config_data_t config;
        void* ptr; // Opaque pointer to miracle::ConfigLoadResult
    } miracle_config_load_result_t;

    typedef struct
    {
        bool success;
        void* ptr; // Opaque pointer to miracle::ConfigSaveResult
    } miracle_config_save_result_t;

    typedef struct
    {
        const char* name;
        uint value;
    } miracle_config_option_t;

    // Options getters
    uint miracle_config_get_modifier_options_count();
    miracle_config_option_t miracle_config_get_modifier_option(uint i);
    uint miracle_config_get_mouse_button_options_count();
    miracle_config_option_t miracle_config_get_mouse_button_option(uint i);
    uint miracle_config_get_mouse_actions_options_count();
    miracle_config_option_t miracle_config_get_mouse_actions_option(uint i);
    uint miracle_config_get_keyboard_actions_options_count();
    miracle_config_option_t miracle_config_get_keyboard_actions_option(uint i);
    uint miracle_config_get_built_in_key_command_options_count();
    miracle_config_option_t miracle_config_get_built_in_key_command_option(uint i);
    uint miracle_config_get_animateable_event_options_count();
    miracle_config_option_t miracle_config_get_animateable_event_option(uint i);
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

    const char* miracle_config_path();

    // Creates a new ConfigLoadResult by loading from the given path
    miracle_config_load_result_t* miracle_config_load(const char* path);

    // Gets the number of errors in the result
    size_t miracle_config_get_error_count(const miracle_config_load_result_t* result);

    // Gets an error by index (0-based)
    const miracle_config_error_t* miracle_config_get_error(
        const miracle_config_load_result_t* result,
        size_t index);

    // Frees the memory allocated for the config load result
    void miracle_config_free(miracle_config_load_result_t* result);

    // Saves the config to the given path
    miracle_config_save_result_t* miracle_config_save(const char* path, const miracle_config_data_t* config);

    // Gets the number of errors in a [miracle_config_save_result_t]
    size_t miracle_save_result_get_error_count(const miracle_config_save_result_t* result);

    // Gets an error by index (0-based)
    const miracle_config_error_t* miracle_save_result_get_error(
        const miracle_config_save_result_t* result,
        size_t index);

    // Frees the memory allocated by [miracle_config_save].
    void miracle_save_result_free(miracle_config_save_result_t* result);

    uint miracle_config_get_primary_modifier(const miracle_config_data_t* config);
    void miracle_config_set_primary_modifier(miracle_config_data_t* config, uint modifier);

    uint miracle_config_get_primary_button(const miracle_config_data_t* config);
    void miracle_config_set_primary_button(miracle_config_data_t* config, uint button);

    uint miracle_config_get_inner_gaps_x(const miracle_config_data_t* config);
    void miracle_config_set_inner_gaps_x(miracle_config_data_t* config, uint value);

    uint miracle_config_get_inner_gaps_y(const miracle_config_data_t* config);
    void miracle_config_set_inner_gaps_y(miracle_config_data_t* config, uint value);

    uint miracle_config_get_outer_gaps_x(const miracle_config_data_t* config);
    void miracle_config_set_outer_gaps_x(miracle_config_data_t* config, uint value);

    uint miracle_config_get_outer_gaps_y(const miracle_config_data_t* config);
    void miracle_config_set_outer_gaps_y(miracle_config_data_t* config, uint value);

    int miracle_config_get_resize_jump(const miracle_config_data_t* config);
    void miracle_config_set_resize_jump(miracle_config_data_t* config, int value);

    bool miracle_config_get_animations_enabled(const miracle_config_data_t* config);
    void miracle_config_set_animations_enabled(miracle_config_data_t* config, bool enabled);

    const char* miracle_config_get_terminal(const miracle_config_data_t* config);
    void miracle_config_set_terminal(miracle_config_data_t* config, const char* terminal);

    typedef struct
    {
        uint action; // MirKeyboardAction as uint
        uint modifiers; // List of modifiers as uint
        uint key; // Input event code of the key, must be oen of https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
        const char* command; // NULL-terminated string
    } miracle_custom_key_command_t;

    // Returns pointer to config data from load result
    const miracle_config_data_t* miracle_config_get_data(const miracle_config_load_result_t* result);

    // Custom key command accessors
    size_t miracle_config_get_custom_key_command_count(const miracle_config_data_t* config);
    miracle_custom_key_command_t miracle_config_get_custom_key_command(
        const miracle_config_data_t* config,
        size_t index);
    void miracle_config_add_custom_key_command(
        miracle_config_data_t* config,
        uint action,
        uint modifiers,
        uint key,
        const char* command);
    void miracle_config_edit_custom_key_command(
        miracle_config_data_t* config,
        size_t index,
        uint action,
        uint modifiers,
        uint key,
        const char* command);
    void miracle_config_clear_custom_key_commands(miracle_config_data_t* config);
    // Remove command at index (returns false if index is invalid)
    bool miracle_config_remove_custom_key_command(miracle_config_data_t* config, size_t index);

    typedef struct
    {
        uint action; // MirKeyboardAction as uint
        uint modifiers; // List of modifiers as uint
        uint key; // Input event code of the key, must be one of https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
        uint command; // DefaultKeyCommand as uint
    } miracle_built_in_key_command_t;

    size_t miracle_config_get_built_in_key_command_override_count(const miracle_config_data_t* config);
    miracle_built_in_key_command_t miracle_config_get_built_in_key_command_override(
        const miracle_config_data_t* config,
        size_t index);
    void miracle_config_add_built_in_key_command_override(
        miracle_config_data_t* config,
        uint action,
        uint modifiers,
        uint key,
        uint command);
    void miracle_config_set_built_in_key_command_override(
        miracle_config_data_t* config,
        size_t index,
        uint action,
        uint modifiers,
        uint key,
        uint command);
    bool miracle_config_remove_built_in_key_command_override(
        const miracle_config_data_t* config,
        size_t index);

    // Startup app accessors
    typedef struct
    {
        const char* command;
        bool restart_on_death;
        bool no_startup_id;
        bool should_halt_compositor_on_death;
        bool in_systemd_scope;
    } miracle_startup_app_t;

    size_t miracle_config_get_startup_app_count(const miracle_config_data_t* config);
    miracle_startup_app_t miracle_config_get_startup_app(const miracle_config_data_t* config, size_t index);
    void miracle_config_add_startup_app(
        miracle_config_data_t* config,
        const char* command,
        bool restart_on_death,
        bool no_startup_id,
        bool should_halt_compositor_on_death,
        bool in_systemd_scope);
    void miracle_config_set_startup_app(
        miracle_config_data_t* config,
        size_t index,
        const char* command,
        bool restart_on_death,
        bool no_startup_id,
        bool should_halt_compositor_on_death,
        bool in_systemd_scope);
    bool miracle_config_remove_startup_app(miracle_config_data_t* config, size_t index);

    // Environment variable accessors
    typedef struct
    {
        const char* key;
        const char* value;
    } miracle_environment_variable_t;

    size_t miracle_config_get_environment_variable_count(const miracle_config_data_t* config);
    miracle_environment_variable_t miracle_config_get_environment_variable(
        const miracle_config_data_t* config,
        size_t index);
    void miracle_config_add_environment_variable(
        miracle_config_data_t* config,
        const char* key,
        const char* value);
    void miracle_config_set_environment_variable(
        miracle_config_data_t* config,
        size_t index,
        const char* key,
        const char* value);
    bool miracle_config_remove_environment_variable(miracle_config_data_t* config, size_t index);

    // Border config accessors
    typedef struct
    {
        int size;
        float radius;
        float focus_color[4]; // RGBA
        float color[4]; // RGBA
    } miracle_border_config_t;

    miracle_border_config_t miracle_config_get_border_config(const miracle_config_data_t* config);
    void miracle_config_set_border_config(
        miracle_config_data_t* config,
        int size,
        float radius,
        const float focus_color[4],
        const float color[4]);

    // Animation definition accessors
    typedef struct
    {
        bool is_default;
        uint type;
        uint function;
        float duration_seconds;
        float c1;
        float c2;
        float c3;
        float c4;
        float c5;
        float n1;
        float d1;
    } miracle_animation_definition_t;

    size_t miracle_config_get_animation_definition_count();
    miracle_animation_definition_t miracle_config_get_animation_definition(
        const miracle_config_data_t* config,
        size_t index);
    void miracle_config_set_animation_definition(
        miracle_config_data_t* config,
        size_t index,
        const miracle_animation_definition_t* definition);
    void miracle_config_reset_animation_definition(
        miracle_config_data_t* config,
        size_t index);

    // Workspace config accessors
    typedef struct
    {
        int num; // -1 if not set
        int container_type; // miracle::ContainerType as int
        const char* name; // NULL if not set
    } miracle_workspace_config_t;

    size_t miracle_config_get_workspace_config_count(const miracle_config_data_t* config);
    miracle_workspace_config_t miracle_config_get_workspace_config(
        const miracle_config_data_t* config,
        size_t index);
    void miracle_config_add_workspace_config(
        miracle_config_data_t* config,
        int num,
        int container_type,
        const char* name);
    void miracle_config_set_workspace_config(
        miracle_config_data_t* config,
        size_t index,
        int num,
        int container_type,
        const char* name);
    void miracle_config_clear_workspace_configs(miracle_config_data_t* config);
    bool miracle_config_remove_workspace_config(
        miracle_config_data_t* config,
        size_t index);

    // Move modifier accessors
    uint miracle_config_get_move_modifier(const miracle_config_data_t* config);
    void miracle_config_set_move_modifier(miracle_config_data_t* config, uint modifier);

    // Drag and drop config accessors
    typedef struct
    {
        bool enabled;
        uint modifiers;
    } miracle_drag_and_drop_config_t;

    miracle_drag_and_drop_config_t miracle_config_get_drag_and_drop(const miracle_config_data_t* config);
    void miracle_config_set_drag_and_drop(
        miracle_config_data_t* config,
        bool enabled,
        uint modifiers);

    // Mouse configuration
    typedef struct
    {
        MirPointerHandedness handedness;
        double acceleration_bias;
        double vscroll_speed;
        double hscroll_speed;
        MirPointerAcceleration acceleration;
    } miracle_mouse_config_t;

    miracle_mouse_config_t miracle_config_get_mouse_config(const miracle_config_data_t* config);
    void miracle_config_set_mouse_config(
        miracle_config_data_t* config,
        MirPointerHandedness handedness,
        double acceleration_bias,
        double vscroll_speed,
        double hscroll_speed,
        MirPointerAcceleration acceleration);

    /// Defines the keymap for the keyboard.
    typedef struct
    {
        /// If true, this means that the keymap is used.
        bool is_set;

        /// The language for the keymap.
        const char* language = "";

        /// Whether the [variant] is set.
        bool has_variant = false;

        /// The variant.
        const char* variant = "";

        // The number of options specified on this keymap.
        size_t options_count = false;
    } miracle_keymap_t;

    /// Retrieve the keymap from the configuration data.
    miracle_keymap_t miracle_config_get_keymap(const miracle_config_data_t* config);
    void miracle_config_set_keymap(
        miracle_config_data_t* config,
        bool is_set,
        const char* language,
        bool has_variant,
        const char* variant);
    const char* miracle_config_get_keymap_option(const miracle_config_data_t* config, size_t index);
    void miracle_config_set_keymap_option(
        miracle_config_data_t* config,
        size_t index,
        const char* option);
    void miracle_config_add_keymap_option(miracle_config_data_t* config, const char* option);
    void miracle_config_remove_keymap_option(miracle_config_data_t* config, size_t index);

    /// Retrieve the repeat delay for the keyboard.
    ///
    /// This is the delay in milliseconds since the previous key down.
    int miracle_config_get_key_repeat_delay(const miracle_config_data_t* config);

    /// Set the repeat delay for the keyboard.
    ///
    /// The delay is in milliseconds since the previous key down.
    void miracle_config_set_key_repeat_delay(miracle_config_data_t* config, int delay);

    /// Retrieve the repeat rate for the keyboard.
    ///
    /// This is expressed in "characters per second".
    int miracle_config_get_key_repeat_rate(const miracle_config_data_t* config);

    /// Set the repeat rate for the keyboard.
    ///
    /// This is express in "characters per second".
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
    miracle_hover_click_t miracle_config_get_hover_click(const miracle_config_data_t* config);

    /// Set the hover click config.
    void miracle_config_set_hover_click(
        miracle_config_data_t* config,
        bool enabled,
        uint hover_duration_milliseconds,
        int cancel_displacement_threshold,
        int reclick_displacement_threshold);

#ifdef __cplusplus
}
#endif

#endif // MIRACLE_WM_CONFIG_C_H
