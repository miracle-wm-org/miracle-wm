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

#define MIR_LOG_COMPONENT "plugin_manager_loader"

#include "feature_flags.h"
#include "plugin_manager.h"

#include <cstdlib>
#include <dlfcn.h>
#include <memory>
#include <mir/log.h>

#ifndef MIRACLE_PLUGIN_MODULE_PATH
#define MIRACLE_PLUGIN_MODULE_PATH ""
#endif

namespace miracle
{
namespace
{
    constexpr char const* soname = "libmiracle-wm-plugins.so";

    /// dlopen the plugin module from the first location that works:
    ///   1. the $MIRACLE_WM_PLUGIN_MODULE override (handy for the build tree / tests),
    ///   2. the configured install path, then
    ///   3. the bare soname (resolved via the normal loader search path).
    ///
    /// RTLD_LOCAL is essential: it keeps the module and its dependencies
    /// (libwasmedge -> libLLVM) out of the global symbol scope, so they cannot
    /// interpose on Mesa's software renderer (llvmpipe), which also uses LLVM.
    /// That interposition is what broke rendering in #865.
    void* open_module()
    {
        char const* const candidates[] = {
            std::getenv("MIRACLE_WM_PLUGIN_MODULE"),
            MIRACLE_PLUGIN_MODULE_PATH,
            soname,
        };

        for (char const* candidate : candidates)
        {
            if (!candidate || candidate[0] == '\0')
                continue;

            if (void* handle = dlopen(candidate, RTLD_NOW | RTLD_LOCAL))
                return handle;

            mir::log_debug("Could not load plugin module from '%s': %s", candidate, dlerror());
        }

        return nullptr;
    }
}

std::shared_ptr<PluginManager> load_plugin_manager()
{
    if (!feature::plugin_system)
        return make_null_plugin_manager();

    void* handle = open_module();
    if (!handle)
    {
        mir::log_error("Failed to load plugin module %s; plugins disabled", soname);
        return make_null_plugin_manager();
    }

    using factory_t = PluginManager* (*)();
    auto const factory = reinterpret_cast<factory_t>(
        dlsym(handle, "miracle_wm_create_plugin_manager"));
    if (!factory)
    {
        mir::log_error("Plugin module is missing miracle_wm_create_plugin_manager: %s; plugins disabled", dlerror());
        dlclose(handle);
        return make_null_plugin_manager();
    }

    PluginManager* raw = factory();
    if (!raw)
    {
        mir::log_error("Plugin module failed to create a PluginManager; plugins disabled");
        dlclose(handle);
        return make_null_plugin_manager();
    }

    mir::log_info("Loaded plugin module %s", soname);

    // The deleter must destroy the manager (whose code lives in the module)
    // before dlclose()-ing the module.
    return std::shared_ptr<PluginManager>(raw, [handle](PluginManager* p)
    {
        delete p;
        dlclose(handle);
    });
}
}
