#include <miracle/animation_definition_internal.h>
#include <miracle/default_key_command.h>
#include <miracle/miracle-wm-config-c.h>
#include <miracle/miracle-wm-config.h>
#include <miracle/mouse_button.h>
#include <vector>

extern "C"
{

    miracle_config_load_result_t* miracle_config_load(const char* path)
    {
        auto const cpp_result = new miracle::ConfigLoadResult(miracle::load_config(path));
        auto const result = new miracle_config_load_result_t();
        result->config._internal = &cpp_result->config;
        result->_errors = &cpp_result->errors;
        return result;
    }

    const miracle_config_data_t* miracle_config_get_data(const miracle_config_load_result_t* result)
    {
        return &result->config;
    }

    size_t miracle_config_get_error_count(const miracle_config_load_result_t* result)
    {
        auto errors = reinterpret_cast<const std::vector<miracle::Error>*>(result->_errors);
        return errors->size();
    }

    const miracle_config_error_t* miracle_config_get_error(
        const miracle_config_load_result_t* result,
        size_t index)
    {

        auto errors = reinterpret_cast<const std::vector<miracle::Error>*>(result->_errors);
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

    void miracle_config_free(miracle_config_load_result_t* result)
    {
        if (result)
        {
            delete reinterpret_cast<miracle::ConfigLoadResult*>(result);
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

    uint miracle_config_get_primary_button(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        return data->primary_button;
    }

    void miracle_config_set_primary_button(miracle_config_data_t* config, uint button)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
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

    int miracle_config_get_inner_gaps_x(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        return data->inner_gaps_x;
    }

    void miracle_config_set_inner_gaps_x(miracle_config_data_t* config, int value)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        if (value < 0)
            value = 0;
        data->inner_gaps_x = value;
    }

    int miracle_config_get_inner_gaps_y(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        return data->inner_gaps_y;
    }

    void miracle_config_set_inner_gaps_y(miracle_config_data_t* config, int value)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        if (value < 0)
            value = 0;
        data->inner_gaps_y = value;
    }

    int miracle_config_get_outer_gaps_x(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        return data->outer_gaps_x;
    }

    void miracle_config_set_outer_gaps_x(miracle_config_data_t* config, int value)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        if (value < 0)
            value = 0;
        data->outer_gaps_x = value;
    }

    int miracle_config_get_outer_gaps_y(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        return data->outer_gaps_y;
    }

    void miracle_config_set_outer_gaps_y(miracle_config_data_t* config, int value)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        if (value < 0)
            value = 0;
        data->outer_gaps_y = value;
    }

    int miracle_config_get_resize_jump(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        return data->resize_jump;
    }

    void miracle_config_set_resize_jump(miracle_config_data_t* config, int value)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        if (value < 0)
            value = 0;
        data->resize_jump = value;
    }

    bool miracle_config_get_animations_enabled(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        return data->animations_enabled;
    }

    void miracle_config_set_animations_enabled(miracle_config_data_t* config, bool enabled)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        data->animations_enabled = enabled;
    }

    const char* miracle_config_get_terminal(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        return data->terminal ? data->terminal->c_str() : nullptr;
    }

    void miracle_config_set_terminal(miracle_config_data_t* config, const char* terminal)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        data->terminal = terminal ? std::optional<std::string>(terminal) : std::nullopt;
    }

    size_t miracle_config_get_custom_key_command_count(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        return data->custom_key_commands.size();
    }

    miracle_custom_key_command_t miracle_config_get_custom_key_command(
        const miracle_config_data_t* config,
        size_t index)
    {

        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
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
        int key,
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

        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        data->custom_key_commands.push_back({ static_cast<MirKeyboardAction>(action),
            modifiers,
            key,
            command ? command : "" });
    }

    void miracle_config_edit_custom_key_command(
        miracle_config_data_t* config,
        int index,
        uint action,
        uint modifiers,
        int key,
        const char* command)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        if (index < 0 || index >= data->custom_key_commands.size())
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
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        data->custom_key_commands.clear();
    }

    bool miracle_config_remove_custom_key_command(miracle_config_data_t* config, size_t index)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->custom_key_commands.size())
            return false;

        data->custom_key_commands.erase(data->custom_key_commands.begin() + index);
        return true;
    }

    size_t miracle_config_get_built_in_key_command_override_count(const miracle_config_data_t* config)
    {
        auto const data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        return data->built_in_key_command_overrides.size();
    }

    miracle_built_in_key_command_t miracle_config_get_built_in_key_command_override(
        const miracle_config_data_t* config,
        size_t index)
    {
        auto const data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
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
        int key,
        uint command)
    {
        auto const data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        data->built_in_key_command_overrides.push_back(miracle::BuiltInKeyCommandOverride {
            static_cast<MirKeyboardAction>(action),
            modifiers,
            key,
            static_cast<miracle::DefaultKeyCommand>(command) });
    }

    void miracle_config_set_built_in_key_command_override(
        miracle_config_data_t* config,
        int index,
        uint action,
        uint modifiers,
        int key,
        uint command)
    {
        auto const data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
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
        auto const data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->built_in_key_command_overrides.size())
            return false;

        data->built_in_key_command_overrides.erase(data->built_in_key_command_overrides.begin() + index);
        return true;
    }

    size_t miracle_config_get_startup_app_count(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        return data->startup_apps.size();
    }

    miracle_startup_app_t miracle_config_get_startup_app(const miracle_config_data_t* config, size_t index)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
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
        auto const data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        data->startup_apps.push_back({ command ? command : "",
            restart_on_death,
            no_startup_id,
            should_halt_compositor_on_death,
            in_systemd_scope });
    }

    void miracle_config_set_startup_app(
        miracle_config_data_t* config,
        int index,
        const char* command,
        bool restart_on_death,
        bool no_startup_id,
        bool should_halt_compositor_on_death,
        bool in_systemd_scope)
    {
        auto const data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
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
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        data->startup_apps.clear();
    }

    bool miracle_config_remove_startup_app(miracle_config_data_t* config, size_t index)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->startup_apps.size())
            return false;

        data->startup_apps.erase(data->startup_apps.begin() + index);
        return true;
    }

    size_t miracle_config_get_environment_variable_count(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        return data->environment_variables.size();
    }

    miracle_environment_variable_t miracle_config_get_environment_variable(
        const miracle_config_data_t* config,
        size_t index)
    {

        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
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

        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        data->environment_variables.push_back({ key ? key : "",
            value ? value : "" });
    }

    void miracle_config_set_environment_variable(
        miracle_config_data_t* config,
        int index,
        const char* key,
        const char* value)
    {
        auto const data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->environment_variables.size())
            return;

        data->environment_variables[index] = { key ? key : "",
            value ? value : "" };
    }

    void miracle_config_clear_environment_variables(miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        data->environment_variables.clear();
    }

    bool miracle_config_remove_environment_variable(miracle_config_data_t* config, size_t index)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->environment_variables.size())
            return false;

        data->environment_variables.erase(data->environment_variables.begin() + index);
        return true;
    }

    size_t miracle_config_get_key_command_count()
    {
        return static_cast<int>(miracle::DefaultKeyCommand::MAX);
    }

    miracle_border_config_t miracle_config_get_border_config(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        miracle_border_config_t result;
        result.size = data->border_config.size;

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
        const float focus_color[4],
        const float color[4])
    {

        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        data->border_config.size = size;

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
        miracle_animatable_event_t event)
    {

        if (event >= MIRACLE_ANIMATABLE_EVENT_MAX)
            return { true, MIRACLE_ANIMATION_TYPE_DISABLED, MIRACLE_EASE_FUNCTION_LINEAR };

        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        const auto& def = data->animation_definitions[event];

        return {
            def.is_default,
            static_cast<miracle_animation_type_t>(def.type),
            static_cast<miracle_ease_function_t>(def.function),
            def.duration_seconds,
            def.c1,
            def.c2,
            def.c3,
            def.c4,
            def.c5,
            def.n1,
            def.d1
        };
    }

    void miracle_config_set_animation_definition(
        miracle_config_data_t* config,
        miracle_animatable_event_t event,
        const miracle_animation_definition_t* definition)
    {
        if (event >= MIRACLE_ANIMATABLE_EVENT_MAX || !definition)
            return;

        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        auto& def = data->animation_definitions[event];

        def.type = static_cast<miracle::AnimationType>(definition->type);
        def.function = static_cast<miracle::EaseFunction>(definition->function);
        def.duration_seconds = definition->duration_seconds;
        def.c1 = definition->c1;
        def.c2 = definition->c2;
        def.c3 = definition->c3;
        def.c4 = definition->c4;
        def.c5 = definition->c5;
        def.n1 = definition->n1;
        def.d1 = definition->d1;
    }

    void miracle_config_reset_animation_definition(
        miracle_config_data_t* config,
        miracle_animatable_event_t event)
    {
        if (event >= MIRACLE_ANIMATABLE_EVENT_MAX)
            return;

        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        auto& def = data->animation_definitions[event];
        def = miracle::internal::default_animation_definitions[event];
    }

    size_t miracle_config_get_workspace_config_count(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        return data->workspace_configs.size();
    }

    miracle_workspace_config_t miracle_config_get_workspace_config(
        const miracle_config_data_t* config,
        size_t index)
    {

        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
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

        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        miracle::WorkspaceConfig ws;

        if (num >= 0)
            ws.num = num;
        if (container_type >= 0)
            ws.layout = static_cast<miracle::ContainerType>(container_type);
        if (name)
            ws.name = name;

        data->workspace_configs.push_back(ws);
    }

    void miracle_config_clear_workspace_configs(miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        data->workspace_configs.clear();
    }

    bool miracle_config_remove_workspace_config(
        miracle_config_data_t* config,
        size_t index)
    {

        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        if (index >= data->workspace_configs.size())
            return false;

        data->workspace_configs.erase(data->workspace_configs.begin() + index);
        return true;
    }

    uint miracle_config_get_move_modifier(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
        return data->move_modifier;
    }

    void miracle_config_set_move_modifier(miracle_config_data_t* config, uint modifier)
    {
        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        data->move_modifier = modifier;
    }

    miracle_drag_and_drop_config_t miracle_config_get_drag_and_drop(const miracle_config_data_t* config)
    {
        auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
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

        auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
        data->drag_and_drop.enabled = enabled;
        data->drag_and_drop.modifiers = modifiers;
    }

} // extern "C"
