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

#ifndef MIRACLE_CURSOR_OVERRIDE_H
#define MIRACLE_CURSOR_OVERRIDE_H

#include <memory>
#include <mir/graphics/cursor.h>
#include <mutex>
#include <optional>
#include <string>

namespace mir
{
class Server;
namespace input
{
    class CursorImages;
}
}

namespace miracle
{
class Config;

/// Forces the on-screen cursor to a named shape, irrespective of what the
/// surface under the pointer asks for.
///
/// Mir renders the cursor image belonging to whichever surface is under the
/// pointer ([mir::input::CursorController::update_cursor_image_locked]). A shell
/// that wants to show its own cursor over an area that it does not own a surface
/// for - such as miracle's window borders, which live in the window margins and
/// are therefore outside of the client's input area - has no surface to hang that
/// image off of. This interface is that escape hatch.
class CursorOverrideController
{
public:
    virtual ~CursorOverrideController() = default;

    /// Force the cursor to \p cursor_name, or release the override when
    /// [std::nullopt] is given. Names are those from `mir_toolkit/cursors.h`.
    /// Repeated calls with the same value are cheap no-ops.
    virtual void set_override(std::optional<std::string> const& cursor_name) = 0;
};

/// A [CursorOverrideController] that does nothing. Used when the cursor could not
/// be wrapped, and by tests.
class NullCursorOverrideController : public CursorOverrideController
{
public:
    void set_override(std::optional<std::string> const&) override { }
};

/// A [mir::graphics::Cursor] decorator that filters [show] and [hide].
///
/// While an override is set, whatever Mir asks to display is remembered but not
/// shown. Releasing the override replays the last request, so the client's cursor
/// comes back exactly as it was. Filtering here rather than calling
/// `the_cursor()->show(...)` directly matters, because
/// [mir::input::CursorController] de-duplicates against the last image it pushed;
/// bypassing it would leave that bookkeeping stale and the cursor stuck.
class OverridableCursor : public mir::graphics::Cursor
{
public:
    /// \param initial_image what Mir has already put on screen. Mir shows the default
    ///                      cursor before handing it to [mir::Server::wrap_cursor], so
    ///                      without this an override released before Mir next asks for
    ///                      an image would hide the cursor rather than restore it.
    OverridableCursor(
        std::shared_ptr<mir::graphics::Cursor> wrapped,
        std::shared_ptr<mir::graphics::CursorImage> initial_image = nullptr);
    ~OverridableCursor() override = default;

    void set_override(std::shared_ptr<mir::graphics::CursorImage> const& image);

    void show(std::shared_ptr<mir::graphics::CursorImage> const& cursor_image) override;
    void hide() override;
    void move_to(mir::geometry::Point position) override;
    void scale(float new_scale) override;
    auto renderable() -> std::shared_ptr<mir::graphics::Renderable> override;
    auto needs_compositing() const -> bool override;

private:
    std::shared_ptr<mir::graphics::Cursor> const wrapped;
    std::mutex mutex_;
    std::shared_ptr<mir::graphics::CursorImage> last_requested;
    std::shared_ptr<mir::graphics::CursorImage> override_image;
};

/// Installs miracle's cursor stack on the server and exposes the override.
///
/// This takes the place of [miral::CursorTheme]: it owns theme loading (via
/// [miracle::XCursorImages]) so that the loader can be kept around for our own
/// lookups, which Mir otherwise gives no access to.
class CursorOverrideService : public CursorOverrideController
{
public:
    explicit CursorOverrideService(std::shared_ptr<Config> const& config);

    void operator()(mir::Server& server);

    void set_override(std::optional<std::string> const& cursor_name) override;

private:
    std::shared_ptr<Config> config;
    std::mutex mutex_;
    std::shared_ptr<mir::input::CursorImages> images;
    std::shared_ptr<OverridableCursor> cursor;
    std::optional<std::string> current_override;
};

} // namespace miracle

#endif // MIRACLE_CURSOR_OVERRIDE_H
