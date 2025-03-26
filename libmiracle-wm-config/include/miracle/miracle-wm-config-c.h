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
    void* _internal; // Opaque pointer to ConfigLoadResult
} miracle_config_load_result_t;

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
