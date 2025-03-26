#include <miracle/miracle-wm-config-c.h>
#include <miracle/miracle-wm-config.h>
#include <vector>

extern "C" {

miracle_config_load_result_t* miracle_config_load(const char* path) {
    auto cpp_result = new miracle::ConfigLoadResult(miracle::load_config(path));
    auto result = new miracle_config_load_result_t();
    result->config._internal = &cpp_result->config;
    result->_errors = &cpp_result->errors;
    return result;
}

const miracle_config_data_t* miracle_config_get_data(const miracle_config_load_result_t* result) {
    return &result->config;
}

size_t miracle_config_get_error_count(const miracle_config_load_result_t* result) {
    auto errors = reinterpret_cast<const std::vector<miracle::Error>*>(result->_errors);
    return errors->size();
}

const miracle_config_error_t* miracle_config_get_error(
    const miracle_config_load_result_t* result, 
    size_t index) {
    
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

void miracle_config_free(miracle_config_load_result_t* result) {
    if (result) {
        delete reinterpret_cast<miracle::ConfigLoadResult*>(result);
    }
}

// ConfigData accessors implementation
uint miracle_config_get_primary_modifier(const miracle_config_data_t* config) {
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    return data->primary_modifier;
}

void miracle_config_set_primary_modifier(miracle_config_data_t* config, uint modifier) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->primary_modifier = modifier;
}

uint miracle_config_get_primary_button(const miracle_config_data_t* config) {
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    return data->primary_button;
}

void miracle_config_set_primary_button(miracle_config_data_t* config, uint button) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->primary_button = button;
}

int miracle_config_get_inner_gaps_x(const miracle_config_data_t* config) {
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    return data->inner_gaps_x;
}

void miracle_config_set_inner_gaps_x(miracle_config_data_t* config, int value) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->inner_gaps_x = value;
}

int miracle_config_get_inner_gaps_y(const miracle_config_data_t* config) {
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    return data->inner_gaps_y;
}

void miracle_config_set_inner_gaps_y(miracle_config_data_t* config, int value) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->inner_gaps_y = value;
}

int miracle_config_get_outer_gaps_x(const miracle_config_data_t* config) {
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    return data->outer_gaps_x;
}

void miracle_config_set_outer_gaps_x(miracle_config_data_t* config, int value) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->outer_gaps_x = value;
}

int miracle_config_get_outer_gaps_y(const miracle_config_data_t* config) {
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    return data->outer_gaps_y;
}

void miracle_config_set_outer_gaps_y(miracle_config_data_t* config, int value) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->outer_gaps_y = value;
}

int miracle_config_get_resize_jump(const miracle_config_data_t* config) {
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    return data->resize_jump;
}

void miracle_config_set_resize_jump(miracle_config_data_t* config, int value) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->resize_jump = value;
}

bool miracle_config_get_animations_enabled(const miracle_config_data_t* config) {
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    return data->animations_enabled;
}

void miracle_config_set_animations_enabled(miracle_config_data_t* config, bool enabled) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->animations_enabled = enabled;
}

const char* miracle_config_get_terminal(const miracle_config_data_t* config) {
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    return data->terminal ? data->terminal->c_str() : nullptr;
}

void miracle_config_set_terminal(miracle_config_data_t* config, const char* terminal) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->terminal = terminal ? std::optional<std::string>(terminal) : std::nullopt;
}

size_t miracle_config_get_custom_key_command_count(const miracle_config_data_t* config) {
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    return data->custom_key_commands.size();
}

miracle_custom_key_command_t miracle_config_get_custom_key_command(
    const miracle_config_data_t* config, 
    size_t index) {
    
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    if (index >= data->custom_key_commands.size())
        return {0, 0, 0, nullptr};

    static thread_local std::string command_copy;
    const auto& cmd = data->custom_key_commands[index];
    command_copy = cmd.command;

    return {
        static_cast<int>(cmd.action),
        cmd.modifiers,
        cmd.key,
        command_copy.c_str()
    };
}

void miracle_config_add_custom_key_command(
    miracle_config_data_t* config,
    int action,
    uint modifiers,
    int key,
    const char* command) {
    
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->custom_key_commands.push_back({
        static_cast<MirKeyboardAction>(action),
        modifiers,
        key,
        command ? command : ""
    });
}

void miracle_config_clear_custom_key_commands(miracle_config_data_t* config) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->custom_key_commands.clear();
}

bool miracle_config_remove_custom_key_command(miracle_config_data_t* config, size_t index) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    if (index >= data->custom_key_commands.size())
        return false;
    
    data->custom_key_commands.erase(data->custom_key_commands.begin() + index);
    return true;
}

size_t miracle_config_get_startup_app_count(const miracle_config_data_t* config) {
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    return data->startup_apps.size();
}

miracle_startup_app_t miracle_config_get_startup_app(const miracle_config_data_t* config, size_t index) {
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    if (index >= data->startup_apps.size())
        return {nullptr, false, false, false, false};

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
    bool in_systemd_scope) {
    
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->startup_apps.push_back({
        command ? command : "",
        restart_on_death,
        no_startup_id,
        should_halt_compositor_on_death,
        in_systemd_scope
    });
}

void miracle_config_clear_startup_apps(miracle_config_data_t* config) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->startup_apps.clear();
}

bool miracle_config_remove_startup_app(miracle_config_data_t* config, size_t index) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    if (index >= data->startup_apps.size())
        return false;
    
    data->startup_apps.erase(data->startup_apps.begin() + index);
    return true;
}

size_t miracle_config_get_environment_variable_count(const miracle_config_data_t* config) {
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    return data->environment_variables.size();
}

miracle_environment_variable_t miracle_config_get_environment_variable(
    const miracle_config_data_t* config, 
    size_t index) {
    
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    if (index >= data->environment_variables.size())
        return {nullptr, nullptr};

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
    const char* value) {
    
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->environment_variables.push_back({
        key ? key : "",
        value ? value : ""
    });
}

void miracle_config_clear_environment_variables(miracle_config_data_t* config) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->environment_variables.clear();
}

bool miracle_config_remove_environment_variable(miracle_config_data_t* config, size_t index) {
    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    if (index >= data->environment_variables.size())
        return false;
    
    data->environment_variables.erase(data->environment_variables.begin() + index);
    return true;
}

size_t miracle_config_get_key_command_count() {
    return static_cast<int>(miracle::DefaultKeyCommand::MAX);
}

size_t miracle_config_get_key_command_list_count(const miracle_config_data_t* config, int command_type) {
    if (command_type < 0 || command_type >= static_cast<int>(miracle::DefaultKeyCommand::MAX))
        return 0;
    
    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    return data->key_commands[command_type].size();
}

miracle_key_command_t miracle_config_get_key_command(
    const miracle_config_data_t* config,
    int command_type,
    size_t index) {
    
    if (command_type < 0 || command_type >= static_cast<int>(miracle::DefaultKeyCommand::MAX))
        return {0, 0, 0};

    auto data = reinterpret_cast<const miracle::ConfigData*>(config->_internal);
    if (index >= data->key_commands[command_type].size())
        return {0, 0, 0};

    const auto& cmd = data->key_commands[command_type][index];
    return {
        static_cast<int>(cmd.action),
        cmd.modifiers,
        cmd.key
    };
}

void miracle_config_add_key_command(
    miracle_config_data_t* config,
    int command_type,
    int action,
    uint modifiers,
    int key) {
    
    if (command_type < 0 || command_type >= static_cast<int>(miracle::DefaultKeyCommand::MAX))
        return;

    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->key_commands[command_type].push_back({
        static_cast<MirKeyboardAction>(action),
        modifiers,
        key
    });
}

bool miracle_config_remove_key_command(
    miracle_config_data_t* config,
    int command_type,
    size_t index) {
    
    if (command_type < 0 || command_type >= static_cast<int>(miracle::DefaultKeyCommand::MAX))
        return false;

    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    if (index >= data->key_commands[command_type].size())
        return false;
    
    data->key_commands[command_type].erase(
        data->key_commands[command_type].begin() + index);
    return true;
}

void miracle_config_clear_key_commands(
    miracle_config_data_t* config,
    int command_type) {
    
    if (command_type < 0 || command_type >= static_cast<int>(miracle::DefaultKeyCommand::MAX))
        return;

    auto data = reinterpret_cast<miracle::ConfigData*>(config->_internal);
    data->key_commands[command_type].clear();
}

} // extern "C"
