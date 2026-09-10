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

#ifndef MIRACLE_WM_STUB_CONTAINER_H
#define MIRACLE_WM_STUB_CONTAINER_H

#include "window_container.h"

namespace miracle
{
namespace test
{
    class StubContainer : public WindowContainer
    {
    public:
        StubContainer() :
            WindowContainer(0, nullptr, nullptr, false)
        {
        }

        void show() override
        {
        }

        void hide() override
        {
        }

        void commit_changes() override
        {
        }

        mir::geometry::Rectangle get_logical_area() const override
        {
            return {};
        }

        void set_logical_area(mir::geometry::Rectangle const& rectangle, bool) override
        {
        }

        mir::geometry::Rectangle get_visible_area() const override
        {
            return {};
        }

        void constrain() override
        {
        }

        std::weak_ptr<ParentContainer> get_parent() const override
        {
            return {};
        }

        void set_parent(std::shared_ptr<ParentContainer> const& ptr) override
        {
        }

        size_t get_min_height() const override
        {
            return 0;
        }

        size_t get_min_width() const override
        {
            return 0;
        }

        void handle_ready() override
        {
        }

        void handle_modify(miral::WindowSpecification const& specification, bool hidden) override
        {
        }

        void handle_request_move(MirInputEvent const* input_event) override
        {
        }

        void handle_raise() override
        {
        }

        bool resize(Direction direction, int pixels) override
        {
            return false;
        }

        bool set_size(std::optional<int> const& width, std::optional<int> const& height) override
        {
            return false;
        }

        bool toggle_fullscreen() override
        {
            return false;
        }

        void request_horizontal_layout() override
        {
        }

        void request_vertical_layout() override
        {
        }

        void toggle_layout(bool cycle_thru_all) override
        {
        }

        void on_open() override
        {
        }

        void on_focus_gained() override
        {
        }

        void on_focus_lost() override
        {
        }

        void on_move_to(mir::geometry::Point const& top_left) override
        {
        }

        void on_resize(mir::geometry::Size const& size) override
        {
        }

        mir::geometry::Rectangle confirm_placement(MirWindowState state, mir::geometry::Rectangle const& rectangle) override
        {
            return {};
        }

        std::shared_ptr<AbstractWorkspace> get_workspace() const override
        {
            return nullptr;
        }

        void set_workspace(std::shared_ptr<AbstractWorkspace> const&) override
        {
        }

        std::shared_ptr<AbstractOutput> get_output() const override
        {
            return nullptr;
        }

        glm::mat4 get_animation_transform() const override
        {
            return glm::mat4(1.f);
        }

        void set_animation_transform(glm::mat4 transform) override
        {
        }

        void set_workspace_transform(glm::mat4 const& transform) override { }
        void set_workspace_alpha(float alpha) override { }

        glm::mat4 get_workspace_transform() const override
        {
            return Container::get_workspace_transform();
        }

        glm::mat4 get_output_transform() const override
        {
            return Container::get_output_transform();
        }

        void set_animation_alpha(float const alpha) override
        {
        }

        uint32_t animation_handle() const override
        {
            return 0;
        }

        void animation_handle(uint32_t uint32) override
        {
        }

        bool is_focused() const override
        {
            return false;
        }

        bool is_fullscreen() const override
        {
            return false;
        }

        std::optional<miral::Window> window() const override
        {
            return std::nullopt;
        }

        bool select_next(Direction direction) override
        {
            return false;
        }

        bool pinned() const override
        {
            return false;
        }

        bool pinned(bool b) override
        {
            return false;
        }

        bool move(Direction direction) override
        {
            return false;
        }

        bool move_by(Direction direction, int pixels) override
        {
            return false;
        }

        bool move_by(float dx, float dy) override
        {
            return false;
        }

        bool move_to(int x, int y, bool with_animations) override
        {
            return false;
        }

        bool move_to(Container& other) override
        {
            return false;
        }

        bool toggle_tabbing() override
        {
            return false;
        }

        bool toggle_stacking() override
        {
            return false;
        }

        bool drag_start() override
        {
            return false;
        }

        void drag(int x, int y) override
        {
        }

        bool drag_stop() override
        {
            return false;
        }

        bool set_layout(LayoutScheme scheme) override
        {
            return false;
        }

        bool anchored() const override
        {
            return true;
        }

        void scratchpad_state(ScratchpadState) override
        {
        }

        ScratchpadState scratchpad_state() const override
        {
            return ScratchpadState::none;
        }

        LayoutScheme get_layout() const override
        {
            return LayoutScheme::horizontal;
        }

        bool matches(ContainerScope const&) const override
        {
            return false;
        }

        nlohmann::json to_json(bool) const override
        {
            return {};
        }
    };
}
}

#endif // MIRACLE_WM_STUB_CONTAINER_H
