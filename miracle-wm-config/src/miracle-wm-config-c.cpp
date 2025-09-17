#include <miracle/animation_definition_internal.h>
#include <miracle/default_key_command.h>
#include <miracle/keyboard.h>
#include <miracle/miracle-wm-config-c.h>
#include <miracle/miracle-wm-config.h>
#include <miracle/mouse_button.h>
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
        return static_cast<uint>(miracle::BultInAnimationType::max);
    }

    miracle_config_option_t miracle_config_get_animation_type_option(uint i)
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

    uint miracle_config_get_animateable_event_options_count()
    {
        return static_cast<uint>(miracle::AnimateableEvent::max);
    }

    miracle_config_option_t miracle_config_get_animateable_event_option(uint i)
    {
        return {
            miracle::animateable_event_strings[i],
            i
        };
    }

    uint miracle_config_get_layout_options_count()
    {
        return static_cast<uint>(miracle::ContainerType::max);
    }

    miracle_config_option_t miracle_config_get_layout_option(uint i)
    {
        return {
            miracle::container_type_strings[i],
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

    uint miracle_config_get_inner_gaps_x(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->inner_gaps.left;
    }

    void miracle_config_set_inner_gaps_x(miracle_config_data_t* config, uint value)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->inner_gaps.left = value;
        data->inner_gaps.right = value;
    }

    uint miracle_config_get_inner_gaps_y(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->inner_gaps.top;
    }

    void miracle_config_set_inner_gaps_y(miracle_config_data_t* config, uint value)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->inner_gaps.top = value;
        data->inner_gaps.bottom = value;
    }

    uint miracle_config_get_outer_gaps_x(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->outer_gaps.left;
    }

    void miracle_config_set_outer_gaps_x(miracle_config_data_t* config, uint value)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->outer_gaps.left = value;
        data->outer_gaps.right = value;
    }

    uint miracle_config_get_outer_gaps_y(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->outer_gaps.top;
    }

    void miracle_config_set_outer_gaps_y(miracle_config_data_t* config, uint value)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->outer_gaps.top = value;
        data->outer_gaps.bottom = value;
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
        return data->terminal ? data->terminal->c_str() : nullptr;
    }

    void miracle_config_set_terminal(miracle_config_data_t* config, const char* terminal)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->terminal = terminal ? std::optional<std::string>(terminal) : std::nullopt;
    }

    size_t miracle_config_get_custom_key_command_count(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->custom_key_commands.size();
    }

    miracle_custom_key_command_t miracle_config_get_custom_key_command(
        const miracle_config_data_t* config,
        size_t index)
    {

        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        if (index >= data->custom_key_commands.size())
            return { 0, 0, 0, nullptr };

        static thread_local std::string command_copy;
        const auto& cmd = data->custom_key_commands[index];
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
        uint action,
        uint modifiers,
        uint key,
        const char* command)
    {

        bool found_action = false;
        for (auto const& [fst, snd] : miracle::mir_keyboard_actions_strings)
        {
            if (snd == action)
                found_action = true;
        }

        if (!found_action)
            return;

        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->custom_key_commands.push_back({ static_cast<MirKeyboardAction>(action),
            modifiers,
            key,
            command ? command : "" });
    }

    void miracle_config_edit_custom_key_command(
        miracle_config_data_t* config,
        size_t index,
        uint action,
        uint modifiers,
        uint key,
        const char* command)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->custom_key_commands.size())
            return;

        bool found_action = false;
        for (auto const& [fst, snd] : miracle::mir_keyboard_actions_strings)
        {
            if (snd == action)
                found_action = true;
        }

        if (!found_action)
            return;

        data->custom_key_commands[index] = { static_cast<MirKeyboardAction>(action),
            modifiers,
            key,
            command ? command : "" };
    }

    void miracle_config_clear_custom_key_commands(miracle_config_data_t* config)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->custom_key_commands.clear();
    }

    bool miracle_config_remove_custom_key_command(miracle_config_data_t* config, size_t index)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->custom_key_commands.size())
            return false;

        data->custom_key_commands.erase(data->custom_key_commands.begin() + static_cast<std::vector<miracle::CustomKeyCommand>::difference_type>(index));
        return true;
    }

    size_t miracle_config_get_built_in_key_command_override_count(const miracle_config_data_t* config)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        return data->built_in_key_command_overrides.size();
    }

    miracle_built_in_key_command_t miracle_config_get_built_in_key_command_override(
        const miracle_config_data_t* config,
        size_t index)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        auto const& command = data->built_in_key_command_overrides[index];
        return {
            .action = static_cast<uint>(command.action),
            .modifiers = command.modifiers,
            .key = command.key,
            .command = static_cast<uint>(command.default_key_command)
        };
    }

    void miracle_config_add_built_in_key_command_override(
        miracle_config_data_t* config,
        uint action,
        uint modifiers,
        uint key,
        uint command)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        data->built_in_key_command_overrides.push_back(miracle::BuiltInKeyCommandOverride {
            static_cast<MirKeyboardAction>(action),
            modifiers,
            key,
            static_cast<miracle::DefaultKeyCommand>(command) });
    }

    void miracle_config_set_built_in_key_command_override(
        miracle_config_data_t* config,
        size_t index,
        uint action,
        uint modifiers,
        uint key,
        uint command)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->built_in_key_command_overrides.size())
            return;

        data->built_in_key_command_overrides[index] = miracle::BuiltInKeyCommandOverride {
            static_cast<MirKeyboardAction>(action),
            modifiers,
            key,
            static_cast<miracle::DefaultKeyCommand>(command)
        };
    }

    bool miracle_config_remove_built_in_key_command_override(
        const miracle_config_data_t* config,
        size_t index)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->built_in_key_command_overrides.size())
            return false;

        data->built_in_key_command_overrides.erase(data->built_in_key_command_overrides.begin() + +static_cast<std::vector<miracle::BuiltInKeyCommandOverride>::difference_type>(index));
        return true;
    }

    size_t miracle_config_get_startup_app_count(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->startup_apps.size();
    }

    miracle_startup_app_t miracle_config_get_startup_app(const miracle_config_data_t* config, size_t index)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        if (index >= data->startup_apps.size())
            return { nullptr, false, false, false, false };

        static thread_local std::string command_copy;
        const auto& app = data->startup_apps[index];
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
        const char* command,
        bool restart_on_death,
        bool no_startup_id,
        bool should_halt_compositor_on_death,
        bool in_systemd_scope)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        data->startup_apps.push_back({ command ? command : "",
            restart_on_death,
            no_startup_id,
            should_halt_compositor_on_death,
            in_systemd_scope });
    }

    void miracle_config_set_startup_app(
        miracle_config_data_t* config,
        size_t index,
        const char* command,
        bool restart_on_death,
        bool no_startup_id,
        bool should_halt_compositor_on_death,
        bool in_systemd_scope)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->startup_apps.size())
            return;

        data->startup_apps[index] = { command ? command : "",
            restart_on_death,
            no_startup_id,
            should_halt_compositor_on_death,
            in_systemd_scope };
    }

    void miracle_config_clear_startup_apps(miracle_config_data_t* config)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->startup_apps.clear();
    }

    bool miracle_config_remove_startup_app(miracle_config_data_t* config, size_t index)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->startup_apps.size())
            return false;

        data->startup_apps.erase(data->startup_apps.begin() + static_cast<std::vector<miracle::StartupApp>::difference_type>(index));
        return true;
    }

    size_t miracle_config_get_environment_variable_count(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->environment_variables.size();
    }

    miracle_environment_variable_t miracle_config_get_environment_variable(
        const miracle_config_data_t* config,
        size_t index)
    {

        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        if (index >= data->environment_variables.size())
            return { nullptr, nullptr };

        static thread_local std::string key_copy;
        static thread_local std::string value_copy;
        const auto& var = data->environment_variables[index];
        key_copy = var.key;
        value_copy = var.value;

        return {
            key_copy.c_str(),
            value_copy.c_str()
        };
    }

    void miracle_config_add_environment_variable(
        miracle_config_data_t* config,
        const char* key,
        const char* value)
    {

        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->environment_variables.push_back({ key ? key : "",
            value ? value : "" });
    }

    void miracle_config_set_environment_variable(
        miracle_config_data_t* config,
        size_t index,
        const char* key,
        const char* value)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->environment_variables.size())
            return;

        data->environment_variables[index] = { key ? key : "",
            value ? value : "" };
    }

    void miracle_config_clear_environment_variables(miracle_config_data_t* config)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->environment_variables.clear();
    }

    bool miracle_config_remove_environment_variable(miracle_config_data_t* config, size_t index)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->environment_variables.size())
            return false;

        data->environment_variables.erase(data->environment_variables.begin() + +static_cast<std::vector<miracle::EnvironmentVariable>::difference_type>(index));
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
        result.size = data->border_config.size;
        result.radius = data->border_config.radius;

        // Copy glm::vec4 to float[4]
        for (int i = 0; i < 4; i++)
        {
            result.focus_color[i] = data->border_config.focus_color[i];
            result.color[i] = data->border_config.color[i];
        }

        return result;
    }

    void miracle_config_set_border_config(
        miracle_config_data_t* config,
        int size,
        float radius,
        const float focus_color[4],
        const float color[4])
    {

        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->border_config.size = size;
        data->border_config.radius = radius;

        // Copy float[4] to glm::vec4
        if (focus_color)
        {
            for (int i = 0; i < 4; i++)
            {
                data->border_config.focus_color[i] = focus_color[i];
            }
        }

        if (color)
        {
            for (int i = 0; i < 4; i++)
            {
                data->border_config.color[i] = color[i];
            }
        }
    }

    size_t miracle_config_get_animation_definition_count()
    {
        return static_cast<int>(miracle::AnimateableEvent::max);
    }

    miracle_animation_definition_t miracle_config_get_animation_definition(
        const miracle_config_data_t* config,
        size_t index)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        const auto& def = data->animation_definitions[index];

        return {
            def.is_default,
            def.animations.empty() ? 0 : static_cast<uint>(def.animations[0].type),
            def.animations.empty() ? 0 : static_cast<uint>(def.animations[0].function),
            def.duration_seconds,
            def.animations.empty() ? 0 : def.animations[0].c1,
            def.animations.empty() ? 0 : def.animations[0].c2,
            def.animations.empty() ? 0 : def.animations[0].c3,
            def.animations.empty() ? 0 : def.animations[0].c4,
            def.animations.empty() ? 0 : def.animations[0].c5,
            def.animations.empty() ? 0 : def.animations[0].n1,
            def.animations.empty() ? 0 : def.animations[0].d1
        };
    }

    void miracle_config_set_animation_definition(
        miracle_config_data_t* config,
        size_t index,
        const miracle_animation_definition_t* definition)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        auto& def = data->animation_definitions[index];

        def.is_default = false;
        def.animations[0].type = static_cast<miracle::BultInAnimationType>(definition->type);
        def.animations[0].function = static_cast<miracle::EaseFunction>(definition->function);
        def.duration_seconds = definition->duration_seconds;
        def.animations[0].c1 = definition->c1;
        def.animations[0].c2 = definition->c2;
        def.animations[0].c3 = definition->c3;
        def.animations[0].c4 = definition->c4;
        def.animations[0].c5 = definition->c5;
        def.animations[0].n1 = definition->n1;
        def.animations[0].d1 = definition->d1;
    }

    void miracle_config_reset_animation_definition(
        miracle_config_data_t* config,
        size_t index)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        auto& def = data->animation_definitions[index];
        def = miracle::internal::default_animation_definitions[index];
    }

    size_t miracle_config_get_workspace_config_count(const miracle_config_data_t* config)
    {
        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        return data->workspace_configs.size();
    }

    miracle_workspace_config_t miracle_config_get_workspace_config(
        const miracle_config_data_t* config,
        size_t index)
    {

        auto data = static_cast<const miracle::ConfigData*>(config->_internal);
        if (index >= data->workspace_configs.size())
            return { -1, -1, nullptr };

        static thread_local std::string name_copy;
        const auto& ws = data->workspace_configs[index];

        if (ws.name)
            name_copy = *ws.name;
        else
            name_copy.clear();

        return {
            ws.num ? *ws.num : -1,
            ws.layout ? static_cast<int>(*ws.layout) : -1,
            ws.name ? name_copy.c_str() : nullptr
        };
    }

    void miracle_config_add_workspace_config(
        miracle_config_data_t* config,
        int num,
        int container_type,
        const char* name)
    {

        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        miracle::WorkspaceConfig ws;

        if (num >= 0)
            ws.num = num;
        if (container_type >= 0)
            ws.layout = static_cast<miracle::ContainerType>(container_type);
        if (name)
            ws.name = name;

        data->workspace_configs.push_back(ws);
    }

    void miracle_config_set_workspace_config(
        miracle_config_data_t* config,
        size_t index,
        int num,
        int container_type,
        const char* name)
    {

        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->workspace_configs.size())
            return;

        miracle::WorkspaceConfig ws;

        if (num >= 0)
            ws.num = num;
        if (container_type >= 0)
            ws.layout = static_cast<miracle::ContainerType>(container_type);
        if (name)
            ws.name = name;

        data->workspace_configs[index] = ws;
    }

    bool miracle_config_remove_workspace_config(
        miracle_config_data_t* config,
        size_t index)
    {

        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->workspace_configs.size())
            return false;

        data->workspace_configs.erase(data->workspace_configs.begin() + +static_cast<std::vector<miracle::WorkspaceConfig>::difference_type>(index));
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
            data->drag_and_drop.enabled,
            data->drag_and_drop.modifiers
        };
    }

    void miracle_config_set_drag_and_drop(
        miracle_config_data_t* config,
        bool enabled,
        uint modifiers)
    {

        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->drag_and_drop.enabled = enabled;
        data->drag_and_drop.modifiers = modifiers;
    }

    miracle_mouse_config_t miracle_config_get_mouse_config(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return {
            data->mouse_configuration.handedness().value_or(mir_pointer_handedness_right),
            data->mouse_configuration.acceleration_bias().value_or(0.0),
            data->mouse_configuration.vscroll_speed().value_or(1.0),
            data->mouse_configuration.hscroll_speed().value_or(1.0),
            data->mouse_configuration.acceleration().value_or(mir_pointer_acceleration_none)
        };
    }

    void miracle_config_set_mouse_config(
        miracle_config_data_t* config,
        MirPointerHandedness handedness,
        double acceleration_bias,
        double vscroll_speed,
        double hscroll_speed,
        MirPointerAcceleration acceleration)
    {
        auto const data = static_cast<miracle::ConfigData*>(config->_internal);
        data->mouse_configuration.handedness(handedness);
        data->mouse_configuration.acceleration_bias(acceleration_bias);
        data->mouse_configuration.vscroll_speed(vscroll_speed);
        data->mouse_configuration.hscroll_speed(hscroll_speed);
        data->mouse_configuration.acceleration(acceleration);
    }

    miracle_keymap_t miracle_config_get_keymap(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        if (!data->keymap)
            return { .is_set = false };

        return {
            .is_set = true,
            .language = data->keymap->language.c_str(),
            .has_variant = data->keymap->variant.has_value(),
            .variant = data->keymap->variant ? data->keymap->variant->c_str() : "",
            .options_count = data->keymap->options.size()
        };
    }

    void miracle_config_set_keymap(
        miracle_config_data_t* config,
        bool is_set,
        const char* language,
        bool has_variant,
        const char* variant)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (!is_set)
            data->keymap = std::nullopt;
        else if (!data->keymap)
            data->keymap = miracle::KeymapConfiguration();

        if (!data->keymap)
            return;

        data->keymap->language = language;
        data->keymap->variant = has_variant ? variant : std::optional<std::string>();
    }

    const char* miracle_config_get_keymap_option(const miracle_config_data_t* config, size_t index)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        if (index >= data->keymap->options.size())
            return nullptr;
        return data->keymap->options[index].c_str();
    }

    void miracle_config_set_keymap_option(
        miracle_config_data_t* config,
        size_t index,
        const char* option)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->keymap->options.size())
            return;
        data->keymap->options[index] = option;
    }

    void miracle_config_add_keymap_option(miracle_config_data_t* config, const char* option)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->keymap->options.push_back(option);
    }
    void miracle_config_remove_keymap_option(miracle_config_data_t* config, size_t index)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->keymap->options.size())
            return;
        data->keymap->options.erase(data->keymap->options.begin() + +static_cast<std::vector<std::string>::difference_type>(index));
    }

    int miracle_config_get_key_repeat_delay(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
        return data->keyboard_configuration.repeat_delay().value_or(600);
#endif
    }

    void miracle_config_set_key_repeat_delay(miracle_config_data_t* config, int delay)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
        data->keyboard_configuration.set_repeat_delay(delay);
#else
        (void)delay;
#endif
    }

    int miracle_config_get_key_repeat_rate(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
        return data->keyboard_configuration.repeat_rate().value_or(25);
#endif
    }

    void miracle_config_set_key_repeat_rate(miracle_config_data_t* config, int rate)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
        data->keyboard_configuration.set_repeat_rate(rate);
#else
        (void)rate;
#endif
    }

    miracle_hover_click_t miracle_config_get_hover_click(const miracle_config_data_t* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return {
            data->hover_click.enabled,
            data->hover_click.hover_duration_milliseconds,
            data->hover_click.cancel_displacement_threshold,
            data->hover_click.reclick_displacement_threshold
        };
    }

    void miracle_config_set_hover_click(
        miracle_config_data_t* config,
        bool enabled,
        uint hover_duration_milliseconds,
        int cancel_displacement_threshold,
        int reclick_displacement_threshold)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->hover_click.enabled = enabled;
        data->hover_click.hover_duration_milliseconds = hover_duration_milliseconds;
        data->hover_click.cancel_displacement_threshold = cancel_displacement_threshold;
        data->hover_click.reclick_displacement_threshold = reclick_displacement_threshold;
    }

    miracle_simulated_secondary_click_t miracle_config_get_simulated_secondary_click(miracle_config_data_t const* config)
    {
        auto const data = static_cast<const miracle::ConfigData*>(config->_internal);
        return {
            data->simulated_secondary_click.enabled,
            data->simulated_secondary_click.hold_duration_milliseconds,
            data->simulated_secondary_click.displacement_threshold
        };
    }

    void miracle_config_set_simulated_secondary_click(
        miracle_config_data_t* config,
        bool enabled,
        uint hold_duration_milliseconds,
        int displacement_threshold)
    {
        auto data = static_cast<miracle::ConfigData*>(config->_internal);
        data->simulated_secondary_click.enabled = enabled;
        data->simulated_secondary_click.hold_duration_milliseconds = hold_duration_milliseconds;
        data->simulated_secondary_click.displacement_threshold = displacement_threshold;
    }
} // extern "C"
