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

#ifndef MIRACLEWM_CONFIG_OBSERVER_H
#define MIRACLEWM_CONFIG_OBSERVER_H

#include "miracle/cpp/config-cpp.h"
#include "observer_registrar.h"

namespace miracle
{
class Config;

class ConfigObserver
{
public:
    virtual ~ConfigObserver() = default;
    virtual void on_config_changed(Config const&) = 0;
    virtual void on_plugins_changed(std::vector<PluginConfiguration> const& plugins);
};

class ConfigObserverRegistrar : public ObserverRegistrar<ConfigObserver>
{
public:
    void advise_config_changed(Config const& config);
    void advise_load_plugins(std::vector<PluginConfiguration> const& plugins);
};
}

#endif // MIRACLEWM_CONFIG_OBSERVER_H
