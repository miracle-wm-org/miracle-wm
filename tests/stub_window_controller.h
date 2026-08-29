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

#ifndef MIRACLE_WM_STUB_WINDOW_CONTROLLER_H
#define MIRACLE_WM_STUB_WINDOW_CONTROLLER_H

#include "compositor_state.h"
#include "window_container.h"
#include "window_controller.h"
#include <miral/window.h>
#include <miral/window_management_options.h>

using namespace miracle;

struct StubWindowData
{
    miral::Window window;
    std::shared_ptr<Container> container;
    mir::geometry::Rectangle rectangle;
    MirWindowState state;
    std::optional<mir::geometry::Rectangle> clip;
};

class StubWindowController : public miracle::WindowController
{
public:
    explicit StubWindowController(std::vector<StubWindowData>& pairs) :
        pairs { pairs }
    {
    }

    void set_rectangle(miral::Window const& window, geom::Rectangle const& from, geom::Rectangle const& to, bool with_animations) override
    {
        get_window_data(window)->rectangle = to;
    }

    MirWindowState get_state(miral::Window const& window) override
    {
        if (auto data = get_window_data(window))
            return data->state;
        return mir_window_state_restored;
    }

    void change_state(miral::Window const& window, MirWindowState state) override
    {
        get_window_data(window)->state = state;
    }

    void clip(miral::Window const& window, geom::Rectangle const& clip) override
    {
        if (auto data = get_window_data(window))
            data->clip = clip;
    }

    void noclip(miral::Window const& window) override
    {
        get_window_data(window)->clip = std::nullopt;
    }

    void select_active_window(miral::Window const& window) override
    {
        selected_windows.push_back(window);
    }

    /// Every window passed to [select_active_window], in order. A default
    /// constructed [miral::Window] means that focus was cleared.
    std::vector<miral::Window> selected_windows;

    std::shared_ptr<Container> get_container(miral::Window const& window)
    {
        for (auto const& p : pairs)
        {
            if (p.window == window)
                return p.container;
        }
        return nullptr;
    }

    std::shared_ptr<WindowContainer> get_window_container(miral::Window const& window) override
    {
        return std::dynamic_pointer_cast<WindowContainer>(get_container(window));
    }

    void raise(miral::Window const&) override { }
    void send_to_back(miral::Window const&) override { }
    void open(miral::Window const&) override { }
    void close(miral::Window const&) override { }
    void set_user_data(miral::Window const&, std::shared_ptr<void> const&) override { }
    void modify(miral::Window const& window, miral::WindowSpecification const& spec) override
    {
        if (spec.top_left())
            get_window_data(window)->rectangle.top_left = spec.top_left().value();
        if (spec.size())
            get_window_data(window)->rectangle.size = spec.size().value();
    }

    miral::WindowInfo& info_for(miral::Window const& window) override
    {
        return stub_win_info;
    }

    miral::ApplicationInfo& info_for(miral::Application const&) override
    {
        return stub_app_info;
    }

    miral::ApplicationInfo& app_info(miral::Window const&) override
    {
        return stub_app_info;
    }

    void move_cursor_to(float x, float y) override { }

    StubWindowData const& get_window_data(std::shared_ptr<Container> const& container)
    {
        for (auto const& p : pairs)
        {
            if (p.container == container)
                return p;
        }

        throw std::runtime_error("get_window_data should resolve");
    }

    miral::Window window_at(float x, float y) override
    {
        return miral::Window();
    }

    void process_animation(AnimationFrameResult const&, std::shared_ptr<Container> const&) override
    {
    }

    void invoke_under_lock(std::function<void()> const& f) override
    {
        f();
    }

    void move_to_offscreen_workspace(miral::Window const&) override { }
    void move_to_onscreen_workspace(miral::Window const&) override { }
    miracle::RestoreResult hide(miral::Window const&) override { return { mir_window_state_restored }; }
    void show(miral::Window const&, miracle::RestoreResult const&) override { }

private:
    std::vector<StubWindowData>& pairs;
    miral::WindowInfo stub_win_info;
    miral::ApplicationInfo stub_app_info;

    StubWindowData* get_window_data(miral::Window const& window)
    {
        for (auto& p : pairs)
        {
            if (p.window == window)
                return &p;
        }

        return nullptr;
    }
};

#endif // MIRACLE_WM_STUB_WINDOW_CONTROLLER_H
