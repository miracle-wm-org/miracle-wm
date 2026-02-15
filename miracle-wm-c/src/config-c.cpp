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

#include <miracle/config.h>
#include <miracle/cpp/config-cpp.h>
#include <miracle/cpp/cursor_focus_mode.h>
#include <miracle/cpp/default_key_command.h>
#include <miracle/cpp/keyboard.h>
#include <miracle/cpp/mouse_button.h>
#include <miracle/cpp/touchpad.h>
#include <vector>

extern "C"
{
    const char* miracle_config_path()
    {
        static thread_local std::string path;
        path = miracle::get_config_path();
        return path.c_str();
    }

    miracle_config_load_result_t* miracle_config_load(const char* path)
    {
        auto const cpp_result = new miracle::ConfigLoadResult(miracle::load_config(path));
        auto const result = new miracle_config_load_result_t();
        result->config._internal = &cpp_result->config;
        result->ptr = cpp_result;
        return result;
    }

    const miracle_config_data_t* miracle_config_get_data(const miracle_config_load_result_t* result)
    {
        return &result->config;
    }

    size_t miracle_config_get_error_count(const miracle_config_load_result_t* result)
    {
        auto const data = static_cast<miracle::ConfigLoadResult*>(result->ptr);
        return data->errors.size();
    }

    const miracle_config_error_t* miracle_config_get_error(
        const miracle_config_load_result_t* result,
        size_t index)
    {
        auto const data = static_cast<miracle::ConfigLoadResult*>(result->ptr);
        auto const errors = &data->errors;
        if (index >= errors->size())
            return nullptr;

        static thread_local miracle_config_error_t error;
        auto const& cpp_error = (*errors)[index];

        error.line = cpp_error.line;
        error.column = cpp_error.column;
        error.level = cpp_error.level == miracle::ErrorLevel::warning
            ? MIRACLE_CONFIG_ERROR_LEVEL_WARNING
            : MIRACLE_CONFIG_ERROR_LEVEL_ERROR;
        error.filename = cpp_error.filename.c_str();
        error.message = cpp_error.message.c_str();

        return &error;
    }

    void miracle_config_free(miracle_config_load_result_t* result)
    {
        if (result)
        {
            delete static_cast<miracle::ConfigLoadResult*>(result->ptr);
            delete result;
        }
    }

    // Save result
    miracle_config_save_result_t* miracle_config_save(const char* path, const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        auto const result = new miracle::ConfigSaveResult(miracle::save_config(path, *data));
        auto const c_result = new miracle_config_save_result_t();
        c_result->success = result->success;
        c_result->ptr = result;
        return c_result;
    }

    size_t miracle_save_result_get_error_count(const miracle_config_save_result_t* result)
    {
        auto const ptr = static_cast<miracle::ConfigSaveResult*>(result->ptr);
        return ptr->errors.size();
    }

    const miracle_config_error_t* miracle_save_result_get_error(
        const miracle_config_save_result_t* result,
        size_t index)
    {
        auto const ptr = static_cast<miracle::ConfigSaveResult*>(result->ptr);
        auto const errors = &ptr->errors;
        if (index >= errors->size())
            return nullptr;

        static thread_local miracle_config_error_t error;
        const auto& cpp_error = (*errors)[index];

        error.line = cpp_error.line;
        error.column = cpp_error.column;
        error.level = cpp_error.level == miracle::ErrorLevel::warning
            ? MIRACLE_CONFIG_ERROR_LEVEL_WARNING
            : MIRACLE_CONFIG_ERROR_LEVEL_ERROR;
        error.filename = cpp_error.filename.c_str();
        error.message = cpp_error.message.c_str();

        return &error;
    }

    void miracle_save_result_free(miracle_config_save_result_t* result)
    {
        if (result)
        {
            delete static_cast<miracle::ConfigSaveResult*>(result->ptr);
            delete result;
        }
    }

    // ConfigData accessors implementation
    size_t miracle_config_get_num_includes(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->includes->size();
    }

    const char* miracle_config_get_include(const miracle_config_data_t* config, size_t index)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->includes.value[index].c_str();
    }

    void miracle_config_add_include(const miracle_config_data_t* config, const char* value, size_t index)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        data->includes->insert(data->includes->begin() + static_cast<std::vector<std::string>::difference_type>(index), value);
    }

    void miracle_config_remove_include(const miracle_config_data_t* config, size_t index)
    {
        const auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->includes->erase(data->includes->begin() + static_cast<std::vector<std::string>::difference_type>(index));
    }

    void miracle_config_set_include(const miracle_config_data_t* config, const char* value, size_t index)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->includes.value[index] = value;
    }

    uint miracle_config_get_primary_modifier(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->primary_modifier;
    }

    void miracle_config_set_primary_modifier(miracle_config_data_t* config, uint modifier)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);

        for (auto const& [fst, scd] : miracle::mir_input_event_modifier_opts)
        {
            if (scd == modifier)
            {
                data->primary_modifier = modifier;
                return;
            }
        }

        // TODO: Bubble the error back to the user
    }

    uint miracle_config_get_modifier_options_count()
    {
        return miracle::mir_input_event_modifier_opts.size();
    }

    miracle_config_option_t miracle_config_get_modifier_option(uint i)
    {
        return {
            miracle::mir_input_event_modifier_opts[i].first,
            miracle::mir_input_event_modifier_opts[i].second
        };
    }

    uint miracle_config_get_mouse_button_options_count()
    {
        return miracle::mir_mouse_buttons_opts.size();
    }

    miracle_config_option_t miracle_config_get_mouse_button_option(uint i)
    {
        return {
            miracle::mir_mouse_buttons_opts[i].first,
            miracle::mir_mouse_buttons_opts[i].second
        };
    }

    uint miracle_config_get_mouse_actions_options_count()
    {
        return miracle::mir_mouse_actions_opts.size();
    }

    miracle_config_option_t miracle_config_get_mouse_actions_option(uint i)
    {
        return {
            miracle::mir_mouse_actions_opts[i].first,
            miracle::mir_mouse_actions_opts[i].second
        };
    }

    uint miracle_config_get_keyboard_actions_options_count()
    {
        return miracle::mir_keyboard_actions_strings.size();
    }

    miracle_config_option_t miracle_config_get_keyboard_actions_option(uint i)
    {
        return {
            miracle::mir_keyboard_actions_strings[i].first,
            miracle::mir_keyboard_actions_strings[i].second
        };
    }

    uint miracle_config_get_built_in_key_command_options_count()
    {
        return miracle::default_key_command_strings.size();
    }

    miracle_config_option_t miracle_config_get_built_in_key_command_option(uint i)
    {
        return {
            miracle::default_key_command_strings[i],
            i
        };
    }

    uint miracle_config_get_animation_type_options_count()
    {
        return static_cast<uint>(miracle::AnimationType::max);
    }

    miracle_config_option_t miracle_config_get_animation_type_option(uint i)
    {
        return {
            miracle::animation_type_strings[i],
            i
        };
    }

    uint miracle_config_get_built_in_animation_type_options_count()
    {
        return static_cast<uint>(miracle::BultInAnimationType::max);
    }

    miracle_config_option_t miracle_config_get_built_in_animation_type_option(uint i)
    {
        return {
            miracle::built_in_animation_type_strings[i],
            i
        };
    }

    uint miracle_config_get_ease_function_options_count()
    {
        return static_cast<uint>(miracle::EaseFunction::max);
    }

    miracle_config_option_t miracle_config_get_ease_function_option(uint i)
    {
        return {
            miracle::ease_function_strings[i],
            i
        };
    }

    uint miracle_config_get_handedness_options_count()
    {
        return static_cast<uint>(mir_pointer_handedness_left + 1);
    }

    miracle_config_option_t miracle_config_get_handedness_option(uint i)
    {
        switch (i)
        {
        case mir_pointer_handedness_left:
            return { "left", i };
        case mir_pointer_handedness_right:
        default:
            return { "right", i };
        }
    }

    uint miracle_config_get_acceleration_options_count()
    {
        return static_cast<uint>(mir_pointer_acceleration_adaptive + 1);
    }

    miracle_config_option_t miracle_config_get_acceleration_option(uint i)
    {
        switch (i)
        {
        case mir_pointer_acceleration_adaptive:
            return { "adapative", i };
        case mir_pointer_acceleration_none:
        default:
            return { "none", i };
        }
    }

    uint miracle_config_get_touchpad_click_mode_options_count()
    {
        return miracle::mir_touchpad_click_mode_opts.size();
    }

    miracle_config_option_t miracle_config_get_touchpad_click_mode_option(uint i)
    {
        return {
            miracle::mir_touchpad_click_mode_opts[i].first,
            miracle::mir_touchpad_click_mode_opts[i].second
        };
    }

    uint miracle_config_get_touchpad_scroll_mode_options_count()
    {
        return miracle::mir_touchpad_scroll_mode_opts.size();
    }

    miracle_config_option_t miracle_config_get_touchpad_scroll_mode_option(uint i)
    {
        return {
            miracle::mir_touchpad_scroll_mode_opts[i].first,
            miracle::mir_touchpad_scroll_mode_opts[i].second
        };
    }

    uint miracle_config_get_primary_button(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->primary_button;
    }

    void miracle_config_set_primary_button(miracle_config_data_t* config, uint button)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        for (auto const& [fst, scd] : miracle::mir_mouse_buttons_opts)
        {
            if (scd == button)
            {
                data->primary_button = button;
                return;
            }
        }

        // TODO: Error handling
    }

    uint32_t miracle_config_get_inner_gaps_x(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->inner_gaps->left;
    }

    void miracle_config_set_inner_gaps_x(miracle_config_data_t* config, uint32_t value)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->inner_gaps->left = value;
        data->inner_gaps->right = value;
    }

    uint32_t miracle_config_get_inner_gaps_y(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->inner_gaps->top;
    }

    void miracle_config_set_inner_gaps_y(miracle_config_data_t* config, uint32_t value)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->inner_gaps->top = value;
        data->inner_gaps->bottom = value;
    }

    uint32_t miracle_config_get_outer_gaps_x(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->outer_gaps->left;
    }

    void miracle_config_set_outer_gaps_x(miracle_config_data_t* config, uint value)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->outer_gaps->left = value;
        data->outer_gaps->right = value;
    }

    uint miracle_config_get_outer_gaps_y(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->outer_gaps->top;
    }

    void miracle_config_set_outer_gaps_y(miracle_config_data_t* config, uint value)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->outer_gaps->top = value;
        data->outer_gaps->bottom = value;
    }

    int miracle_config_get_resize_jump(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->resize_jump;
    }

    void miracle_config_set_resize_jump(miracle_config_data_t* config, int value)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->resize_jump = value;
    }

    bool miracle_config_get_animations_enabled(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->animations_enabled;
    }

    void miracle_config_set_animations_enabled(miracle_config_data_t* config, bool enabled)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->animations_enabled = enabled;
    }

    const char* miracle_config_get_terminal(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return *data->terminal ? data->terminal->value().c_str() : nullptr;
    }

    void miracle_config_set_terminal(miracle_config_data_t* config, const char* terminal)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->terminal = terminal ? std::optional<std::string>(terminal) : std::nullopt;
    }

    size_t miracle_config_get_custom_key_command_count(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->custom_key_commands->size();
    }

    miracle_custom_key_command_t miracle_config_get_custom_key_command(
        const miracle_config_data_t* config,
        size_t index)
    {

        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        if (index >= data->custom_key_commands->size())
            return { 0, 0, 0, nullptr };

        static thread_local std::string command_copy;
        const auto& cmd = data->custom_key_commands.value[index];
        command_copy = cmd.command;

        return {
            static_cast<uint>(cmd.action),
            cmd.modifiers,
            cmd.key,
            command_copy.c_str()
        };
    }

    void miracle_config_add_custom_key_command(
        miracle_config_data_t* config,
        miracle_custom_key_command_t* key_command)
    {

        bool found_action = false;
        for (auto const& [fst, snd] : miracle::mir_keyboard_actions_strings)
        {
            if (snd == key_command->action)
                found_action = true;
        }

        if (!found_action)
            return;

        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->custom_key_commands->push_back(miracle::CustomKeyCommand { static_cast<MirKeyboardAction>(key_command->action),
            key_command->modifiers,
            key_command->key,
            key_command->command });
    }

    void miracle_config_edit_custom_key_command(
        miracle_config_data_t* config,
        size_t index,
        miracle_custom_key_command_t* key_command)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->custom_key_commands->size())
            return;

        bool found_action = false;
        for (auto const& [fst, snd] : miracle::mir_keyboard_actions_strings)
        {
            if (snd == key_command->action)
                found_action = true;
        }

        if (!found_action)
            return;

        data->custom_key_commands.value[index] = { static_cast<MirKeyboardAction>(key_command->action),
            key_command->modifiers,
            key_command->key,
            key_command->command };
    }

    bool miracle_config_remove_custom_key_command(miracle_config_data_t* config, size_t index)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->custom_key_commands->size())
            return false;

        data->custom_key_commands->erase(data->custom_key_commands->begin() + static_cast<std::vector<miracle::CustomKeyCommand>::difference_type>(index));
        return true;
    }

    size_t miracle_config_get_built_in_key_command_override_count(const miracle_config_data_t* config)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        return data->built_in_key_command_overrides->size();
    }

    miracle_built_in_key_command_override_t miracle_config_get_built_in_key_command_override(
        const miracle_config_data_t* config,
        size_t index)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        auto const& command = data->built_in_key_command_overrides.value[index];
        return {
            .action = static_cast<uint>(command.action),
            .modifiers = command.modifiers,
            .key = command.key,
            .command = static_cast<uint>(command.default_key_command)
        };
    }

    void miracle_config_add_built_in_key_command_override(
        miracle_config_data_t* config,
        miracle_built_in_key_command_override_t* key_command_override)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        data->built_in_key_command_overrides->push_back(miracle::BuiltInKeyCommandOverride {
            static_cast<MirKeyboardAction>(key_command_override->action),
            key_command_override->modifiers,
            key_command_override->key,
            static_cast<miracle::DefaultKeyCommand>(key_command_override->command) });
    }

    void miracle_config_set_built_in_key_command_override(
        miracle_config_data_t* config,
        size_t index,
        miracle_built_in_key_command_override_t* key_command_override)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->built_in_key_command_overrides->size())
            return;

        data->built_in_key_command_overrides.value[index] = miracle::BuiltInKeyCommandOverride {
            static_cast<MirKeyboardAction>(key_command_override->action),
            key_command_override->modifiers,
            key_command_override->key,
            static_cast<miracle::DefaultKeyCommand>(key_command_override->command)
        };
    }

    bool miracle_config_remove_built_in_key_command_override(
        const miracle_config_data_t* config,
        size_t index)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->built_in_key_command_overrides->size())
            return false;

        data->built_in_key_command_overrides->erase(data->built_in_key_command_overrides->begin() + +static_cast<std::vector<miracle::BuiltInKeyCommandOverride>::difference_type>(index));
        return true;
    }

    size_t miracle_config_get_startup_app_count(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->startup_apps->size();
    }

    miracle_startup_app_t miracle_config_get_startup_app(const miracle_config_data_t* config, size_t index)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        if (index >= data->startup_apps->size())
            return { nullptr, false, false, false, false };

        static thread_local std::string command_copy;
        const auto& app = data->startup_apps.value[index];
        command_copy = app.command;

        return {
            command_copy.c_str(),
            app.restart_on_death,
            app.no_startup_id,
            app.should_halt_compositor_on_death,
            app.in_systemd_scope
        };
    }

    void miracle_config_add_startup_app(
        miracle_config_data_t* config,
        miracle_startup_app_t* startup_app)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        data->startup_apps->push_back({ startup_app->command,
            startup_app->restart_on_death,
            startup_app->no_startup_id,
            startup_app->should_halt_compositor_on_death,
            startup_app->in_systemd_scope });
    }

    void miracle_config_set_startup_app(
        miracle_config_data_t* config,
        size_t index,
        miracle_startup_app_t* startup_app)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->startup_apps->size())
            return;

        data->startup_apps.value[index] = { startup_app->command,
            startup_app->restart_on_death,
            startup_app->no_startup_id,
            startup_app->should_halt_compositor_on_death,
            startup_app->in_systemd_scope };
    }

    void miracle_config_clear_startup_apps(miracle_config_data_t* config)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->startup_apps->clear();
    }

    bool miracle_config_remove_startup_app(miracle_config_data_t* config, size_t index)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->startup_apps->size())
            return false;

        data->startup_apps->erase(data->startup_apps->begin() + static_cast<std::vector<miracle::StartupApp>::difference_type>(index));
        return true;
    }

    size_t miracle_config_get_environment_variable_count(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->environment_variables->size();
    }

    miracle_environment_variable_t miracle_config_get_environment_variable(
        const miracle_config_data_t* config,
        size_t index)
    {

        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        if (index >= data->environment_variables->size())
            return { nullptr, nullptr };

        static thread_local std::string key_copy;
        static thread_local std::string value_copy;
        const auto& var = data->environment_variables.value[index];
        key_copy = var.key;
        value_copy = var.value;

        return {
            key_copy.c_str(),
            value_copy.c_str()
        };
    }

    void miracle_config_add_environment_variable(
        miracle_config_data_t* config,
        miracle_environment_variable_t* variable)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->environment_variables->push_back({ variable->key,
            variable->value });
    }

    void miracle_config_set_environment_variable(
        miracle_config_data_t* config,
        size_t index,
        miracle_environment_variable_t* variable)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->environment_variables->size())
            return;

        data->environment_variables.value[index] = { variable->key, variable->value };
    }

    void miracle_config_clear_environment_variables(miracle_config_data_t* config)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->environment_variables->clear();
    }

    bool miracle_config_remove_environment_variable(miracle_config_data_t* config, size_t index)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->environment_variables->size())
            return false;

        data->environment_variables->erase(data->environment_variables->begin() + +static_cast<std::vector<miracle::EnvironmentVariable>::difference_type>(index));
        return true;
    }

    size_t miracle_config_get_plugin_count(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->plugins->size();
    }

    miracle_plugin_t miracle_config_get_plugin(const miracle_config_data_t* config, size_t index)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        if (index >= data->plugins->size())
            return { nullptr };

        static thread_local std::string path_copy;
        auto const& plugin = data->plugins.value[index];
        path_copy = plugin.path;
        return { path_copy.c_str() };
    }

    void miracle_config_add_plugin(miracle_config_data_t* config, miracle_plugin_t* plugin)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->plugins->push_back(miracle::PluginConfiguration { plugin->path });
    }

    void miracle_config_set_plugin(miracle_config_data_t* config, size_t index, miracle_plugin_t* plugin)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->plugins->size())
            return;
        data->plugins.value[index] = miracle::PluginConfiguration { plugin->path };
    }

    bool miracle_config_remove_plugin(miracle_config_data_t* config, size_t index)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->plugins->size())
            return false;
        data->plugins->erase(data->plugins->begin() + static_cast<std::vector<miracle::PluginConfiguration>::difference_type>(index));
        return true;
    }

    size_t miracle_config_get_key_command_count()
    {
        return static_cast<int>(miracle::DefaultKeyCommand::MAX);
    }

    miracle_border_config_t miracle_config_get_border_config(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        miracle_border_config_t result;
        result.size = data->border_config->size;
        result.radius = data->border_config->radius;

        // Copy glm::vec4 to float[4]
        for (int i = 0; i < 4; i++)
        {
            result.focus_color[i] = data->border_config->focus_color[i];
            result.color[i] = data->border_config->color[i];
        }

        return result;
    }

    void miracle_config_set_border_config(
        miracle_config_data_t* config,
        miracle_border_config_t* border_config)
    {

        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        data->border_config->size = border_config->size;
        data->border_config->radius = border_config->radius;

        for (int i = 0; i < 4; i++)
            data->border_config->focus_color[i] = border_config->focus_color[i];

        for (int i = 0; i < 4; i++)
            data->border_config->color[i] = border_config->color[i];
    }

    size_t miracle_config_get_animateable_event_count()
    {
        return static_cast<int>(miracle::AnimateableEvent::max);
    }

    miracle_animateable_event_t miracle_config_get_animateable_event(
        miracle_config_data_t* config,
        size_t index)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        auto const& def = &data->animation_definitions.value[index];

        auto const built_in_animations = def->data.size();

        return {
            miracle::animateable_event_strings[index],
            def->is_default,
            static_cast<uint>(def->type),
            def->duration_seconds,
            built_in_animations,
            static_cast<void*>(def)
        };
    }

    void miracle_config_set_animateable_event(
        miracle_config_data_t* config,
        size_t index,
        const miracle_animateable_event_t* definition)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        auto& def = data->animation_definitions.value[index];
        def.is_default = false;
        def.type = static_cast<miracle::AnimationType>(definition->type);
        def.duration_seconds = definition->duration_seconds;
    }

    void miracle_config_reset_animation_definition(
        miracle_config_data_t* config,
        size_t index)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        auto& def = data->animation_definitions.value[index];
        def = miracle::ConfigData::get_default_animation_definition(static_cast<miracle::AnimateableEvent>(index));
    }

    miracle_built_in_animation_t miracle_animateable_event_get_animation_part(
        miracle_animateable_event_t* animateable_event,
        size_t index)
    {
        auto def = static_cast<miracle::AnimationDefinition*>(animateable_event->_internal);
        auto const animation = def->data[index];
        return {
            static_cast<uint>(animation.type),
            static_cast<uint>(animation.function),
            animation.c1,
            animation.c2,
            animation.c3,
            animation.c4,
            animation.c5,
            animation.n1,
            animation.d1
        };
    }

    void miracle_animateable_event_add_animation(
        miracle_animateable_event_t* animateable_event,
        miracle_built_in_animation_t animation)
    {
        auto const def = static_cast<miracle::AnimationDefinition*>(animateable_event->_internal);
        if (def->type != miracle::AnimationType::built_in)
        {
            def->type = miracle::AnimationType::built_in;
            def->data = miracle::BuiltInAnimationList {};
        }

        def->is_default = false;
        def->data.push_back(miracle::BuiltInAnimationDefinition {
            static_cast<miracle::BultInAnimationType>(animation.type),
            static_cast<miracle::EaseFunction>(animation.function),
            animation.c1,
            animation.c2,
            animation.c3,
            animation.c4,
            animation.c5,
            animation.n1,
            animation.d1 });
    }

    void miracle_animateable_event_set_animation(
        miracle_animateable_event_t* animateable_event,
        size_t index,
        miracle_built_in_animation_t animation)
    {
        auto const def = static_cast<miracle::AnimationDefinition*>(animateable_event->_internal);
        if (def->type != miracle::AnimationType::built_in)
        {
            def->type = miracle::AnimationType::built_in;
            def->data = miracle::BuiltInAnimationList {};
        }

        def->is_default = false;
        auto& animation_def = def->data[index];
        animation_def.type = static_cast<miracle::BultInAnimationType>(animation.type);
        animation_def.function = static_cast<miracle::EaseFunction>(animation.function);
        animation_def.c1 = animation.c1;
        animation_def.c2 = animation.c2;
        animation_def.c3 = animation.c3;
        animation_def.c4 = animation.c4;
        animation_def.c5 = animation.c5;
        animation_def.n1 = animation.n1;
        animation_def.d1 = animation.d1;
    }

    void miracle_animateable_event_remove_animation(
        miracle_animateable_event_t* animateable_event,
        size_t index)
    {
        auto def = static_cast<miracle::AnimationDefinition*>(animateable_event->_internal);
        if (def->type != miracle::AnimationType::built_in)
        {
            def->type = miracle::AnimationType::built_in;
            def->data = miracle::BuiltInAnimationList {};
        }

        def->is_default = false;

        if (index >= def->data.size())
            return;
        def->data.erase(def->data.begin() + static_cast<std::vector<miracle::BuiltInAnimationDefinition>::difference_type>(index));
    }

    size_t miracle_config_get_workspace_config_count(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->workspace_configs->size();
    }

    miracle_workspace_config_t miracle_config_get_workspace_config(
        const miracle_config_data_t* config,
        size_t index)
    {

        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        if (index >= data->workspace_configs->size())
            return { false, -1, false, nullptr };

        static thread_local std::string name_copy;
        const auto& ws = data->workspace_configs.value[index];

        if (ws.name)
            name_copy = *ws.name;
        else
            name_copy.clear();

        return {
            ws.num.has_value(),
            ws.num.value_or(-1),
            ws.name.has_value(),
            name_copy.c_str(),
        };
    }

    void miracle_config_add_workspace_config(
        miracle_config_data_t* config,
        miracle_workspace_config_t* workspace_config)
    {

        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        miracle::WorkspaceConfig ws;

        if (workspace_config->has_num)
            ws.num = workspace_config->num;
        if (workspace_config->has_name)
            ws.name = workspace_config->name;

        data->workspace_configs->push_back(ws);
    }

    void miracle_config_set_workspace_config(
        miracle_config_data_t* config,
        size_t index,
        miracle_workspace_config_t* workspace_config)
    {

        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->workspace_configs->size())
            return;

        miracle::WorkspaceConfig ws;

        if (workspace_config->has_num)
            ws.num = workspace_config->num;
        if (workspace_config->has_name)
            ws.name = workspace_config->name;

        data->workspace_configs.value[index] = ws;
    }

    bool miracle_config_remove_workspace_config(
        miracle_config_data_t* config,
        size_t index)
    {

        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->workspace_configs->size())
            return false;

        data->workspace_configs->erase(data->workspace_configs->begin() + +static_cast<std::vector<miracle::WorkspaceConfig>::difference_type>(index));
        return true;
    }

    uint miracle_config_get_move_modifier(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->move_modifier;
    }

    void miracle_config_set_move_modifier(miracle_config_data_t* config, uint modifier)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->move_modifier = modifier;
    }

    miracle_drag_and_drop_config_t miracle_config_get_drag_and_drop(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return {
            data->drag_and_drop->enabled,
            data->drag_and_drop->modifiers
        };
    }

    void miracle_config_set_drag_and_drop(
        miracle_config_data_t* config,
        miracle_drag_and_drop_config_t* drag_and_drop_config)
    {

        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->drag_and_drop->enabled = drag_and_drop_config->enabled;
        data->drag_and_drop->modifiers = drag_and_drop_config->modifiers;
    }

    miracle_mouse_config_t miracle_config_get_mouse_config(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return {
            static_cast<uint>(data->mouse_configuration->handedness().value_or(mir_pointer_handedness_right)),
            data->mouse_configuration->acceleration_bias().value_or(0.0),
            data->mouse_configuration->vscroll_speed().value_or(1.0),
            data->mouse_configuration->hscroll_speed().value_or(1.0),
            static_cast<uint>(data->mouse_configuration->acceleration().value_or(mir_pointer_acceleration_none))
        };
    }

    void miracle_config_set_mouse_config(
        miracle_config_data_t* config,
        miracle_mouse_config_t* mouse_config)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        data->mouse_configuration->handedness(static_cast<MirPointerHandedness>(mouse_config->handedness));
        data->mouse_configuration->acceleration_bias(mouse_config->acceleration_bias);
        data->mouse_configuration->vscroll_speed(mouse_config->vscroll_speed);
        data->mouse_configuration->hscroll_speed(mouse_config->hscroll_speed);
        data->mouse_configuration->acceleration(static_cast<MirPointerAcceleration>(mouse_config->acceleration));
    }

    miracle_touchpad_config_t miracle_config_get_touchpad_config(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return {
            data->touchpad->disable_while_typing,
            data->touchpad->disable_with_external_mouse,
            data->touchpad->acceleration_bias,
            data->touchpad->vscroll_speed,
            data->touchpad->hscroll_speed,
            data->touchpad->tap_to_click,
            data->touchpad->middle_mouse_button_emulation,
            static_cast<uint>(data->touchpad->click_mode),
            static_cast<uint>(data->touchpad->scroll_mode)
        };
    }

    void miracle_config_set_touchpad_config(
        miracle_config_data_t* config,
        miracle_touchpad_config_t* touchpad_config)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        data->touchpad->disable_while_typing = touchpad_config->disable_while_typing;
        data->touchpad->disable_with_external_mouse = touchpad_config->disable_with_external_mouse;
        data->touchpad->acceleration_bias = touchpad_config->acceleration_bias;
        data->touchpad->vscroll_speed = touchpad_config->vscroll_speed;
        data->touchpad->hscroll_speed = touchpad_config->hscroll_speed;
        data->touchpad->tap_to_click = touchpad_config->tap_to_click;
        data->touchpad->middle_mouse_button_emulation = touchpad_config->middle_mouse_button_emulation;
        data->touchpad->click_mode = static_cast<MirTouchpadClickMode>(touchpad_config->click_mode);
        data->touchpad->scroll_mode = static_cast<MirTouchpadScrollMode>(touchpad_config->scroll_mode);
    }

    miracle_keymap_t miracle_config_get_keymap(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        if (!*data->keymap)
            return { .is_set = false,
                .language = "",
                .has_variant = false,
                .variant = "",
                .options_count = 0 };

        return {
            .is_set = true,
            .language = data->keymap->value().language.c_str(),
            .has_variant = data->keymap->value().variant.has_value(),
            .variant = data->keymap->value().variant ? data->keymap->value().variant->c_str() : "",
            .options_count = data->keymap->value().options.size()
        };
    }

    void miracle_config_set_keymap(
        miracle_config_data_t* config,
        miracle_keymap_t* keymap)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (!keymap->is_set)
            data->keymap = std::nullopt;
        else if (!*data->keymap)
            data->keymap = miracle::KeymapConfiguration();

        if (!*data->keymap)
            return;

        data->keymap->value().language = keymap->language;
        data->keymap->value().variant = keymap->has_variant ? keymap->variant : std::optional<std::string>();
    }

    const char* miracle_config_get_keymap_option(const miracle_config_data_t* config, size_t index)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        if (index >= data->keymap->value().options.size())
            return nullptr;
        return data->keymap->value().options[index].c_str();
    }

    void miracle_config_set_keymap_option(
        miracle_config_data_t* config,
        size_t index,
        const char* option)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->keymap->value().options.size())
            return;
        data->keymap->value().options[index] = option;
    }

    void miracle_config_add_keymap_option(miracle_config_data_t* config, const char* option)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->keymap->value().options.push_back(option);
    }
    void miracle_config_remove_keymap_option(miracle_config_data_t* config, size_t index)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->keymap->value().options.size())
            return;
        data->keymap->value().options.erase(data->keymap->value().options.begin() + +static_cast<std::vector<std::string>::difference_type>(index));
    }

    int miracle_config_get_key_repeat_delay(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
        return data->keyboard_configuration->repeat_delay().value_or(600);
#endif
    }

    void miracle_config_set_key_repeat_delay(miracle_config_data_t* config, int delay)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
        data->keyboard_configuration->set_repeat_delay(delay);
#else
        (void)delay;
#endif
    }

    int miracle_config_get_key_repeat_rate(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
        return data->keyboard_configuration->repeat_rate().value_or(25);
#endif
    }

    void miracle_config_set_key_repeat_rate(miracle_config_data_t* config, int rate)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
        data->keyboard_configuration->set_repeat_rate(rate);
#else
        (void)rate;
#endif
    }

    miracle_hover_click_t miracle_config_get_hover_click(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return {
            data->hover_click->enabled,
            data->hover_click->hover_duration_milliseconds,
            data->hover_click->cancel_displacement_threshold,
            data->hover_click->reclick_displacement_threshold
        };
    }

    void miracle_config_set_hover_click(
        miracle_config_data_t* config,
        miracle_hover_click_t* hover_click)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->hover_click->enabled = hover_click->enabled;
        data->hover_click->hover_duration_milliseconds = hover_click->hover_duration_milliseconds;
        data->hover_click->cancel_displacement_threshold = hover_click->cancel_displacement_threshold;
        data->hover_click->reclick_displacement_threshold = hover_click->reclick_displacement_threshold;
    }

    miracle_simulated_secondary_click_t miracle_config_get_simulated_secondary_click(miracle_config_data_t const* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return {
            data->simulated_secondary_click->enabled,
            data->simulated_secondary_click->hold_duration_milliseconds,
            data->simulated_secondary_click->displacement_threshold
        };
    }

    void miracle_config_set_simulated_secondary_click(
        miracle_config_data_t* config,
        miracle_simulated_secondary_click_t* simulated_secondary_click)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->simulated_secondary_click->enabled = simulated_secondary_click->enabled;
        data->simulated_secondary_click->hold_duration_milliseconds = simulated_secondary_click->hold_duration_milliseconds;
        data->simulated_secondary_click->displacement_threshold = simulated_secondary_click->displacement_threshold;
    }

    miracle_output_filter_t miracle_config_get_output_filter(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return {
            data->output_filter->shader_path.has_value(),
            data->output_filter->shader_path ? data->output_filter->shader_path->c_str() : ""
        };
    }

    void miracle_config_set_output_filter(
        miracle_config_data_t* config,
        miracle_output_filter_t* output_filter)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (output_filter->shader_path_enabled)
            data->output_filter->shader_path = output_filter->shader_path;
        else
            data->output_filter->shader_path = std::nullopt;
    }

    miracle_cursor_t miracle_config_get_cursor(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return {
            data->cursor->scale,
            static_cast<uint>(data->cursor->focus_mode)
        };
    }

    void miracle_config_set_cursor(miracle_config_data_t* config, miracle_cursor_t* cursor)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        data->cursor->scale = cursor->scale;
        data->cursor->focus_mode = static_cast<miracle::CursorFocusMode>(cursor->focus_mode);
    }

    miracle_slow_keys_t miracle_config_get_slow_keys(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return {
            data->slow_keys->enabled,
            data->slow_keys->hold_delay_milliseconds,
        };
    }
    void miracle_config_set_slow_keys(miracle_config_data_t* config, miracle_slow_keys_t* slow_keys)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->slow_keys->enabled = slow_keys->enabled;
        data->slow_keys->hold_delay_milliseconds = slow_keys->hold_duration_milliseconds;
    }

    miracle_sticky_keys_t miracle_config_get_sticky_keys(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return {
            data->sticky_keys->enabled,
            data->sticky_keys->should_disable_if_two_keys_are_pressed_together,
        };
    }
    void miracle_config_set_sticky_keys(miracle_config_data_t* config, miracle_sticky_keys_t* sticky_keys)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->sticky_keys->enabled = sticky_keys->enabled;
        data->sticky_keys->should_disable_if_two_keys_are_pressed_together = sticky_keys->should_disable_if_two_keys_are_pressed_together;
    }

    miracle_magnifier_t miracle_config_get_magnifier(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return {
            data->magnifier->enabled,
            data->magnifier->scale,
            data->magnifier->scale_increment,
            data->magnifier->width,
            data->magnifier->height,
            data->magnifier->size_increment
        };
    }

    void miracle_config_set_magnifier(miracle_config_data_t* config, miracle_magnifier_t magnifier)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->magnifier->enabled = magnifier.enabled;
        data->magnifier->scale = magnifier.scale;
        data->magnifier->scale_increment = magnifier.scale_increment;
        data->magnifier->width = magnifier.width;
        data->magnifier->height = magnifier.height;
        data->magnifier->size_increment = magnifier.size_increment;
    }

    bool miracle_config_get_workspace_back_and_forth(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->workspace_back_and_forth;
    }

    void miracle_config_set_workspace_back_and_forth(miracle_config_data_t* config, bool enabled)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->workspace_back_and_forth = enabled;
    }
} // extern "C"
