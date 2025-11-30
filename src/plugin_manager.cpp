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

#include "plugin_manager.h"

using namespace miracle;

#if ENABLE_PLUGIN_SYSTEM
namespace
{
WasmEdge_ConfigureContext* create_configure_context()
{
    auto const context = WasmEdge_ConfigureCreate();
    WasmEdge_ConfigureAddHostRegistration(context, WasmEdge_HostRegistration_Wasi);
    return context;
}
}

PluginManager::PluginManager() :
    configure_context(create_configure_context()),
    store_context(WasmEdge_StoreCreate()),
    vm_context(WasmEdge_VMCreate(configure_context.get(), store_context.get()))
{
}
#endif
