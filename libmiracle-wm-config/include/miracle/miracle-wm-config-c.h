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
extern "C" {
#endif

typedef enum {
    MIRACLE_CONFIG_ERROR_LEVEL_WARNING,
    MIRACLE_CONFIG_ERROR_LEVEL_ERROR
} miracle_config_error_level_t;

typedef struct {
    int line;
    int column;
    miracle_config_error_level_t level;
    const char* filename;
    const char* message;
} miracle_config_error_t;

typedef struct {
    void* _internal; // Opaque pointer to ConfigData
} miracle_config_data_t;

typedef struct {
    miracle_config_data_t config;
    void* _errors; // Opaque pointer to vector<Error>
} miracle_config_load_result_t;

// ConfigData accessors
uint miracle_config_get_primary_modifier(const miracle_config_data_t* config);
void miracle_config_set_primary_modifier(miracle_config_data_t* config, uint modifier);

uint miracle_config_get_primary_button(const miracle_config_data_t* config);
void miracle_config_set_primary_button(miracle_config_data_t* config, uint button);

int miracle_config_get_inner_gaps_x(const miracle_config_data_t* config);
void miracle_config_set_inner_gaps_x(miracle_config_data_t* config, int value);

int miracle_config_get_inner_gaps_y(const miracle_config_data_t* config);
void miracle_config_set_inner_gaps_y(miracle_config_data_t* config, int value);

int miracle_config_get_outer_gaps_x(const miracle_config_data_t* config);
void miracle_config_set_outer_gaps_x(miracle_config_data_t* config, int value);

int miracle_config_get_outer_gaps_y(const miracle_config_data_t* config);
void miracle_config_set_outer_gaps_y(miracle_config_data_t* config, int value);

int miracle_config_get_resize_jump(const miracle_config_data_t* config);
void miracle_config_set_resize_jump(miracle_config_data_t* config, int value);

bool miracle_config_get_animations_enabled(const miracle_config_data_t* config);
void miracle_config_set_animations_enabled(miracle_config_data_t* config, bool enabled);

// Returns pointer to terminal command string or NULL if not set
const char* miracle_config_get_terminal(const miracle_config_data_t* config);
// Set terminal command (makes a copy of the string)
void miracle_config_set_terminal(miracle_config_data_t* config, const char* terminal);

typedef struct {
    int action; // MirKeyboardAction as int
    uint modifiers;
    int key;
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
    int action,
    uint modifiers,
    int key,
    const char* command);
void miracle_config_clear_custom_key_commands(miracle_config_data_t* config);
// Remove command at index (returns false if index is invalid)
bool miracle_config_remove_custom_key_command(miracle_config_data_t* config, size_t index);

// Startup app accessors
typedef struct {
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
void miracle_config_clear_startup_apps(miracle_config_data_t* config);
bool miracle_config_remove_startup_app(miracle_config_data_t* config, size_t index);

// Environment variable accessors
typedef struct {
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
void miracle_config_clear_environment_variables(miracle_config_data_t* config);
bool miracle_config_remove_environment_variable(miracle_config_data_t* config, size_t index);

// Key command accessors (fixed size array)
typedef struct {
    int action; // MirKeyboardAction as int
    uint modifiers;
    int key;
} miracle_key_command_t;

size_t miracle_config_get_key_command_count();
size_t miracle_config_get_key_command_list_count(const miracle_config_data_t* config, int command_type);
miracle_key_command_t miracle_config_get_key_command(
    const miracle_config_data_t* config,
    int command_type,
    size_t index);
void miracle_config_add_key_command(
    miracle_config_data_t* config,
    int command_type,
    int action,
    uint modifiers,
    int key);
bool miracle_config_remove_key_command(
    miracle_config_data_t* config,
    int command_type,
    size_t index);
void miracle_config_clear_key_commands(
    miracle_config_data_t* config,
    int command_type);

// Border config accessors
typedef struct {
    int size;
    float focus_color[4]; // RGBA
    float color[4]; // RGBA
} miracle_border_config_t;

miracle_border_config_t miracle_config_get_border_config(const miracle_config_data_t* config);
void miracle_config_set_border_config(
    miracle_config_data_t* config,
    int size,
    const float focus_color[4],
    const float color[4]);

// Animation definition accessors
typedef enum {
    MIRACLE_ANIMATION_TYPE_DISABLED,
    MIRACLE_ANIMATION_TYPE_SLIDE,
    MIRACLE_ANIMATION_TYPE_GROW,
    MIRACLE_ANIMATION_TYPE_SHRINK,
    MIRACLE_ANIMATION_TYPE_FADE_IN,
    MIRACLE_ANIMATION_TYPE_FADE_OUT,
    MIRACLE_ANIMATION_TYPE_MAX
} miracle_animation_type_t;

typedef enum {
    MIRACLE_EASE_FUNCTION_LINEAR,
    MIRACLE_EASE_FUNCTION_EASE_IN_SINE,
    MIRACLE_EASE_FUNCTION_EASE_OUT_SINE,
    MIRACLE_EASE_FUNCTION_EASE_IN_OUT_SINE,
    MIRACLE_EASE_FUNCTION_EASE_IN_QUAD,
    MIRACLE_EASE_FUNCTION_EASE_OUT_QUAD,
    MIRACLE_EASE_FUNCTION_EASE_IN_OUT_QUAD,
    MIRACLE_EASE_FUNCTION_EASE_IN_CUBIC,
    MIRACLE_EASE_FUNCTION_EASE_OUT_CUBIC,
    MIRACLE_EASE_FUNCTION_EASE_IN_OUT_CUBIC,
    MIRACLE_EASE_FUNCTION_EASE_IN_QUART,
    MIRACLE_EASE_FUNCTION_EASE_OUT_QUART,
    MIRACLE_EASE_FUNCTION_EASE_IN_OUT_QUART,
    MIRACLE_EASE_FUNCTION_EASE_IN_QUINT,
    MIRACLE_EASE_FUNCTION_EASE_OUT_QUINT,
    MIRACLE_EASE_FUNCTION_EASE_IN_OUT_QUINT,
    MIRACLE_EASE_FUNCTION_EASE_IN_EXPO,
    MIRACLE_EASE_FUNCTION_EASE_OUT_EXPO,
    MIRACLE_EASE_FUNCTION_EASE_IN_OUT_EXPO,
    MIRACLE_EASE_FUNCTION_EASE_IN_CIRC,
    MIRACLE_EASE_FUNCTION_EASE_OUT_CIRC,
    MIRACLE_EASE_FUNCTION_EASE_IN_OUT_CIRC,
    MIRACLE_EASE_FUNCTION_EASE_IN_BACK,
    MIRACLE_EASE_FUNCTION_EASE_OUT_BACK,
    MIRACLE_EASE_FUNCTION_EASE_IN_OUT_BACK,
    MIRACLE_EASE_FUNCTION_EASE_IN_ELASTIC,
    MIRACLE_EASE_FUNCTION_EASE_OUT_ELASTIC,
    MIRACLE_EASE_FUNCTION_EASE_IN_OUT_ELASTIC,
    MIRACLE_EASE_FUNCTION_EASE_IN_BOUNCE,
    MIRACLE_EASE_FUNCTION_EASE_OUT_BOUNCE,
    MIRACLE_EASE_FUNCTION_EASE_IN_OUT_BOUNCE,
    MIRACLE_EASE_FUNCTION_MAX
} miracle_ease_function_t;

typedef enum {
    MIRACLE_ANIMATABLE_EVENT_WINDOW_OPEN,
    MIRACLE_ANIMATABLE_EVENT_WINDOW_MOVE,
    MIRACLE_ANIMATABLE_EVENT_WINDOW_CLOSE,
    MIRACLE_ANIMATABLE_EVENT_WORKSPACE_SWITCH,
    MIRACLE_ANIMATABLE_EVENT_MAX
} miracle_animatable_event_t;

typedef struct {
    miracle_animation_type_t type;
    miracle_ease_function_t function;
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
    miracle_animatable_event_t event);
void miracle_config_set_animation_definition(
    miracle_config_data_t* config,
    miracle_animatable_event_t event,
    const miracle_animation_definition_t* definition);

// Workspace config accessors
typedef struct {
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
void miracle_config_clear_workspace_configs(miracle_config_data_t* config);
bool miracle_config_remove_workspace_config(
    miracle_config_data_t* config,
    size_t index);

// Move modifier accessors
uint miracle_config_get_move_modifier(const miracle_config_data_t* config);
void miracle_config_set_move_modifier(miracle_config_data_t* config, uint modifier);

// Drag and drop config accessors
typedef struct {
    bool enabled;
    uint modifiers;
} miracle_drag_and_drop_config_t;

miracle_drag_and_drop_config_t miracle_config_get_drag_and_drop(const miracle_config_data_t* config);
void miracle_config_set_drag_and_drop(
    miracle_config_data_t* config,
    bool enabled,
    uint modifiers);

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

#ifdef __cplusplus
}
#endif

#endif // MIRACLE_WM_CONFIG_C_H
