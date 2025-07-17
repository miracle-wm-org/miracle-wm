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

#include "binding_event.h"
#include <libevdev-1.0/libevdev/libevdev.h>
#include <mir_toolkit/events/enums.h>

using namespace miracle;

namespace
{
nlohmann::json modifiers_to_array(unsigned int modifiers)
{
    nlohmann::json array;
    if (modifiers & mir_input_event_modifier_alt)
        array.push_back("alt");
    if (modifiers & mir_input_event_modifier_shift)
        array.push_back("shift");
    if (modifiers & mir_input_event_modifier_sym)
        array.push_back("sym");
    if (modifiers & mir_input_event_modifier_function)
        array.push_back("function");
    if (modifiers & mir_input_event_modifier_ctrl)
        array.push_back("ctrl");
    if (modifiers & mir_input_event_modifier_meta)
        array.push_back("meta");
    if (modifiers & mir_input_event_modifier_caps_lock)
        array.push_back("caps_lock");
    if (modifiers & mir_input_event_modifier_num_lock)
        array.push_back("num_lock");
    if (modifiers & mir_input_event_modifier_scroll_lock)
        array.push_back("scroll_lock");
    return array;
}
}

BindingEvent::BindingEvent(
    std::string const& mode, std::string const& command, unsigned int modifiers, xkb_keysym_t keysym, BindingEventType type) :
    mode(mode),
    command(command),
    modifiers(modifiers),
    keysym(keysym),
    type(type)
{
}

nlohmann::json BindingEvent::to_json() const
{
    nlohmann::json json;
    json["change"] = change;

    nlohmann::json binding;
    binding["command"] = command;
    binding["event_state_mask"] = modifiers_to_array(modifiers);
    binding["input_code"] = keysym;

    char symbol[64]; // big enough for most UTF-8 chars
    int const len = xkb_keysym_to_utf8(keysym, symbol, sizeof(symbol));
    if (len > 0)
        binding["symbol"] = symbol;

    switch (type)
    {
    case BindingEventType::keyboard:
        binding["type"] = "keyboard";
        break;
    case BindingEventType::mouse:
        binding["type"] = "mouse";
        break;
    default:
        break;
    }

    json["binding"] = binding;
    return json;
}
