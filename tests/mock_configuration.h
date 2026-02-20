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

#ifndef MIRACLE_WM_MOCK_CONFIGURATION_H
#define MIRACLE_WM_MOCK_CONFIGURATION_H

#include "config.h"
#include <gmock/gmock.h>

namespace mir
{
class Server;
}

namespace miracle
{
namespace test
{
    class MockConfig : public Config
    {
    public:
        MOCK_METHOD(void, Call, (mir::Server & server));

        void operator()(mir::Server& server) override
        {
            Call(server);
        }

        MOCK_METHOD(void, reload, (), (override));
        MOCK_METHOD(std::string const&, get_filename, (), (const, override));
        MOCK_METHOD(std::vector<PluginConfiguration> const&, get_plugins, (), (const, override));
        MOCK_METHOD(MirInputEventModifier, get_input_event_modifier, (), (const, override));
        MOCK_METHOD(CustomKeyCommand const*, matches_custom_key_command, (MirKeyboardAction action, uint32_t keysym, unsigned int modifiers), (const, override));
        MOCK_METHOD(bool, matches_key_command, (MirKeyboardAction action, uint32_t keysym, unsigned int modifiers, std::function<bool(DefaultKeyCommand)> const& f), (const, override));
        MOCK_METHOD(Gaps, get_inner_gaps, (), (const, override));
        MOCK_METHOD(Gaps, get_outer_gaps, (), (const, override));
        MOCK_METHOD(void, override_inner_gaps, (Gaps const&), (override));
        MOCK_METHOD(void, override_outer_gaps, (Gaps const&), (override));
        MOCK_METHOD(std::vector<StartupApp> const&, get_startup_apps, (), (const, override));
        MOCK_METHOD(std::optional<std::string> const&, get_terminal_command, (), (const, override));
        MOCK_METHOD(int, get_resize_jump, (), (const, override));
        MOCK_METHOD(std::vector<EnvironmentVariable> const&, get_env_variables, (), (const, override));
        MOCK_METHOD(BorderConfig const&, get_border_config, (), (const, override));
        MOCK_METHOD((AnimationDefinition const&), get_animation_definition, (AnimateableEvent), (const, override));
        MOCK_METHOD(bool, are_animations_enabled, (), (const, override));
        MOCK_METHOD(WorkspaceConfig, get_workspace_config, (std::optional<int> const& num, std::optional<std::string> const& name), (const, override));
        MOCK_METHOD(LayoutScheme, get_default_layout_scheme, (), (const, override));
        MOCK_METHOD(DragAndDropConfiguration, drag_and_drop, (), (const, override));
        MOCK_METHOD(uint, get_primary_modifier, (), (const, override));
        MOCK_METHOD(uint, move_modifier, (), (const, override));
        MOCK_METHOD(uint, get_primary_button, (), (const, override));
        MOCK_METHOD(miral::InputConfiguration::Mouse, mouse, (), (const, override));
#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
        MOCK_METHOD(miral::InputConfiguration::Keyboard, keyboard, (), (const, override));
#endif
        MOCK_METHOD(std::optional<std::string>, keymap, (), (const, override));
        MOCK_METHOD(MagnifierConfiguration, magnifier, (), (const, override));
        MOCK_METHOD(HoverClickConfiguration, hover_click, (), (const, override));
        MOCK_METHOD(SimulatedSecondaryClickConfiguration, simulated_secondary_click, (), (const, override));
        MOCK_METHOD(OutputFilterConfiguration, output_filter, (), (const, override));
        MOCK_METHOD(CursorConfiguration, cursor, (), (const, override));
        MOCK_METHOD(SlowKeysConfiguration, slow_keys, (), (const, override));
        MOCK_METHOD(StickyKeysConfiguration, sticky_keys, (), (const, override));
        MOCK_METHOD(TouchpadConfiguration, touchpad, (), (const, override));
        MOCK_METHOD(bool, get_workspace_back_and_forth, (), (const, override));
    };
}
}

#endif // MIRACLE_WM_MOCK_CONFIGURATION_H
