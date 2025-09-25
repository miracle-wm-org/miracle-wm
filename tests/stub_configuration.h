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

#ifndef MIRACLE_WM_STUB_CONFIGURATION_H
#define MIRACLE_WM_STUB_CONFIGURATION_H

#include "config.h"

namespace miracle
{
namespace test
{
    class StubConfiguration : public miracle::Config
    {
    public:
        void operator()(mir::Server& server) override { }
        void reload() override { }
        [[nodiscard]] std::string const& get_filename() const override { return filename; }
        [[nodiscard]] MirInputEventModifier get_input_event_modifier() const override { return mir_input_event_modifier_none; }
        [[nodiscard]] CustomKeyCommand const* matches_custom_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers) const override
        {
            return nullptr;
        }

        bool matches_key_command(MirKeyboardAction action, int scan_code, unsigned int modifiers, std::function<bool(DefaultKeyCommand)> const& f) const override
        {
            return false;
        }

        [[nodiscard]] Gaps get_inner_gaps() const override
        {
            return Gaps {};
        }

        [[nodiscard]] Gaps get_outer_gaps() const override
        {
            return Gaps {};
        }

        void override_inner_gaps(Gaps const&) override { }
        void override_outer_gaps(Gaps const&) override { }

        [[nodiscard]] std::vector<StartupApp> const& get_startup_apps() const override
        {
            return startup_apps;
        }

        [[nodiscard]] std::optional<std::string> const& get_terminal_command() const override
        {
            return terminal_command;
        }

        [[nodiscard]] int get_resize_jump() const override
        {
            return 0;
        }

        [[nodiscard]] std::vector<EnvironmentVariable> const& get_env_variables() const override
        {
            return env;
        }

        [[nodiscard]] BorderConfig const& get_border_config() const override
        {
            return border_config;
        }

        [[nodiscard]] AnimationDefinition const& get_animation_definition(AnimateableEvent event) const override
        {
            return animations[static_cast<size_t>(event)];
        }

        [[nodiscard]] bool are_animations_enabled() const override
        {
            return false;
        }

        [[nodiscard]] WorkspaceConfig get_workspace_config(std::optional<int> const& num, std::optional<std::string> const& name) const override
        {
            return { num, ContainerType::leaf, name };
        }

        [[nodiscard]] uint get_primary_modifier() const override
        {
            return mir_input_event_modifier_meta;
        }

        [[nodiscard]] LayoutScheme get_default_layout_scheme() const override
        {
            return LayoutScheme::horizontal;
        }

        [[nodiscard]] DragAndDropConfiguration drag_and_drop() const override
        {
            return {};
        }

        [[nodiscard]] uint move_modifier() const override
        {
            return 0;
        }

        [[nodiscard]] uint get_primary_button() const override
        {
            return mir_pointer_button_primary;
        }

        miral::InputConfiguration::Mouse mouse() const override
        {
            return miral::InputConfiguration::Mouse {};
        }

#if MIRAL_VERSION >= MIR_VERSION_NUMBER(5, 3, 0)
        miral::InputConfiguration::Keyboard keyboard() const override
        {
            return miral::InputConfiguration::Keyboard {};
        }
#endif

        std::optional<std::string> keymap() const override
        {
            return std::nullopt;
        }

        HoverClickConfiguration hover_click() const override
        {
            return {};
        }

        SimulatedSecondaryClickConfiguration simulated_secondary_click() const override
        {
            return {};
        }

        OutputFilterConfiguration output_filter() const override
        {
            return {};
        }

        CursorConfiguration cursor() const override
        {
            return {};
        }

        SlowKeysConfiguration slow_keys() const override
        {
            return {};
        }

        StickyKeysConfiguration sticky_keys() const override
        {
            return {};
        }

    private:
        miracle::BorderConfig border_config;
        std::array<AnimationDefinition, static_cast<int>(AnimateableEvent::max)> animations;
        std::string filename;
        std::vector<StartupApp> startup_apps;
        std::optional<std::string> terminal_command;
        std::vector<EnvironmentVariable> env;
    };
}
}

#endif // MIRACLE_WM_STUB_CONFIGURATION_H
