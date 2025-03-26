#include <miracle/miracle-wm-config-c.h>
#include <miracle/miracle-wm-config.h>
#include <vector>

extern "C" {

miracle_config_load_result_t* miracle_config_load(const char* path) {
    auto result = new miracle::ConfigLoadResult(miracle::load_config(path));
    return reinterpret_cast<miracle_config_load_result_t*>(result);
}

size_t miracle_config_get_error_count(const miracle_config_load_result_t* result) {
    auto internal = reinterpret_cast<const miracle::ConfigLoadResult*>(result->_internal);
    return internal->errors.size();
}

const miracle_config_error_t* miracle_config_get_error(
    const miracle_config_load_result_t* result, 
    size_t index) {
    
    auto internal = reinterpret_cast<const miracle::ConfigLoadResult*>(result->_internal);
    if (index >= internal->errors.size())
        return nullptr;

    static thread_local miracle_config_error_t error;
    const auto& cpp_error = internal->errors[index];
    
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
        delete reinterpret_cast<miracle::ConfigLoadResult*>(result->_internal);
        delete result;
    }
}

} // extern "C"
