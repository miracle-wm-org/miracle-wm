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
