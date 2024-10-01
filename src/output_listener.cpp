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

#include "output_listener.h"

using namespace miracle;

void OutputListenerMultiplexer::output_created(miral::Output const& output)
{
    for (auto const& listener : listeners)
        listener->output_created(output);
}

void OutputListenerMultiplexer::output_updated(miral::Output const& updated, miral::Output const& original)
{
    for (auto const& listener : listeners)
        listener->output_updated(updated, original);
}

void OutputListenerMultiplexer::output_deleted(miral::Output const& output)
{
    for (auto const& listener : listeners)
        listener->output_deleted(output);
}

void OutputListenerMultiplexer::register_listener(OutputListener* listener)
{
    listeners.push_back(listener);
}

void OutputListenerMultiplexer::unregister_listener(OutputListener* listener)
{
    listeners.push_back(listener);
}