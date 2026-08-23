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

#ifndef CAROUSEL_CONTROLLER_H
#define CAROUSEL_CONTROLLER_H

#include <atomic>
#include <cstdint>
#include <memory>
#include <mir_toolkit/events/event.h>

namespace miracle
{
class AbstractCommandController;
class Animator;
class CompositorState;
class Config;
class OutputManager;
class WindowController;

/// Owns the window management side of the carousel scene override: the token of
/// the active override, and the gesture that enters it.
class CarouselController
{
public:
    CarouselController(
        std::shared_ptr<CompositorState> const& compositor_state,
        std::shared_ptr<OutputManager> const& output_manager,
        std::shared_ptr<Animator> const& animator,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<Config> const& config,
        std::shared_ptr<AbstractCommandController> const& command_controller);

    /// Watches for a "tap" of the primary action modifier - pressed and
    /// released with no other key or pointer button in between - and enters the
    /// carousel when one completes.
    ///
    /// \p modifiers is the event's modifier mask, already reduced by
    /// [MODIFIER_MASK]. The event is never consumed, so clients keep a
    /// consistent modifier state.
    void handle_keyboard_event(MirKeyboardEvent const* event, unsigned int modifiers);

    /// Breaks the pending tap, so that the next modifier release does not enter
    /// the carousel. Called for any input that is not part of the gesture.
    void break_tap();

private:
    /// Enters the carousel scene override, if possible. Every output carousels
    /// the windows on its own active workspace.
    void try_start();

    std::shared_ptr<CompositorState> compositor_state;
    std::shared_ptr<OutputManager> output_manager;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<Config> config;
    std::shared_ptr<AbstractCommandController> command_controller;

    /// Token of the active carousel override (0 = none). Atomic because the
    /// override's completion callback runs on the animator thread.
    std::atomic<uint32_t> token { 0 };

    /// True while the primary action modifier is held down without any other
    /// key or button press in between; releasing it then triggers the carousel.
    bool tap_latched = false;
};
}

#endif
