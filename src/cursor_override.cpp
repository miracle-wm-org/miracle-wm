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

#define MIR_LOG_COMPONENT "miracle-cursor-override"

#include "cursor_override.h"

#include "config.h"
#include "xcursor_images.h"

#include <mir/graphics/cursor_image.h>
#include <mir/input/cursor_images.h>
#include <mir/log.h>
#include <mir/options/option.h>
#include <mir/server.h>

namespace mg = mir::graphics;
namespace mi = mir::input;

namespace miracle
{

OverridableCursor::OverridableCursor(
    std::shared_ptr<mg::Cursor> wrapped, std::shared_ptr<mg::CursorImage> initial_image) :
    wrapped { std::move(wrapped) },
    last_requested { std::move(initial_image) }
{
}

void OverridableCursor::set_override(std::shared_ptr<mg::CursorImage> const& image)
{
    std::shared_ptr<mg::CursorImage> to_show;
    bool should_hide = false;

    {
        std::lock_guard lock { mutex_ };
        if (override_image == image)
            return;

        override_image = image;
        if (image)
            to_show = image;
        else if (last_requested)
            to_show = last_requested;
        else
            should_hide = true;
    }

    if (should_hide)
        wrapped->hide();
    else
        wrapped->show(to_show);
}

void OverridableCursor::show(std::shared_ptr<mg::CursorImage> const& cursor_image)
{
    {
        std::lock_guard lock { mutex_ };
        last_requested = cursor_image;
        if (override_image)
            return;
    }

    wrapped->show(cursor_image);
}

void OverridableCursor::hide()
{
    {
        std::lock_guard lock { mutex_ };
        last_requested = nullptr;
        if (override_image)
            return;
    }

    wrapped->hide();
}

void OverridableCursor::move_to(mir::geometry::Point position)
{
    wrapped->move_to(position);
}

void OverridableCursor::scale(float new_scale)
{
    wrapped->scale(new_scale);
}

auto OverridableCursor::renderable() -> std::shared_ptr<mg::Renderable>
{
    return wrapped->renderable();
}

auto OverridableCursor::needs_compositing() const -> bool
{
    return wrapped->needs_compositing();
}

CursorOverrideService::CursorOverrideService(std::shared_ptr<Config> const& config) :
    config { config }
{
}

void CursorOverrideService::operator()(mir::Server& server)
{
    char const* const theme_option = "cursor-theme";
    server.add_configuration_option(
        theme_option, "Colon separated cursor theme list, e.g. default:DMZ-Black.", "default");

    // Both builders below are invoked lazily during server initialization, which is
    // after the configuration file has been read, so the configured theme is available.
    server.override_the_cursor_images([this, &server, theme_option]() -> std::shared_ptr<mi::CursorImages>
    {
        auto const options = server.get_options();
        auto const configured = config->cursor().theme;
        auto const theme_list = options->is_set(theme_option) || !configured
            ? options->get<std::string>(theme_option)
            : *configured;

        auto loader = std::make_shared<XCursorImages>(theme_list);
        if (!loader->has_default_cursor())
        {
            mir::log_warning("Failed to load cursor theme '%s'. Using built-in cursors.", theme_list.c_str());
            return {};
        }

        std::lock_guard lock { mutex_ };
        images = loader;
        return loader;
    });

    server.wrap_cursor([this, &server](std::shared_ptr<mg::Cursor> const& wrapped) -> std::shared_ptr<mg::Cursor>
    {
        auto result = std::make_shared<OverridableCursor>(wrapped, server.the_default_cursor_image());
        std::lock_guard lock { mutex_ };
        cursor = result;
        return result;
    });
}

void CursorOverrideService::set_override(std::optional<std::string> const& cursor_name)
{
    std::lock_guard lock { mutex_ };
    if (current_override == cursor_name)
        return;

    current_override = cursor_name;

    if (!cursor)
        return;

    if (!cursor_name)
    {
        cursor->set_override(nullptr);
        return;
    }

    if (!images)
        return;

    cursor->set_override(images->image(*cursor_name, mi::default_cursor_size));
}

} // namespace miracle
