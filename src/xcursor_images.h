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

#ifndef MIRACLE_XCURSOR_IMAGES_H
#define MIRACLE_XCURSOR_IMAGES_H

#include <map>
#include <memory>
#include <mir/input/cursor_images.h>
#include <mutex>
#include <string>
#include <vector>

namespace miracle
{

/// A [mir::input::CursorImages] backed by libXcursor.
///
/// Mir provides no way for a downstream shell to retrieve the server's own
/// [mir::input::CursorImages] (there is a `mir::Server::override_the_cursor_images`
/// but no matching getter), so miracle installs its own loader and holds onto it.
/// That lets us look up named resize cursors ourselves when the pointer hovers a
/// window border. This replaces [miral::CursorTheme], which installs an equivalent
/// loader but keeps it private.
///
/// Images are loaded lazily and cached; the same [std::shared_ptr] is returned for
/// a repeated lookup because [mir::input::CursorController] de-duplicates by pointer
/// identity before pushing an image to the screen.
class XCursorImages : public mir::input::CursorImages
{
public:
    /// \param theme_list a colon separated list of theme names, tried in order
    explicit XCursorImages(std::string const& theme_list);

    std::shared_ptr<mir::graphics::CursorImage> image(
        std::string const& cursor_name,
        mir::geometry::Size const& size) override;

    /// Whether any of the configured themes provided a default cursor. If this is
    /// false the theme is unusable and Mir's built-in cursors should be used instead.
    [[nodiscard]] bool has_default_cursor();

private:
    std::shared_ptr<mir::graphics::CursorImage> load(std::string const& name, uint32_t size);

    std::vector<std::string> themes;
    std::mutex mutex_;
    std::map<std::pair<std::string, uint32_t>, std::shared_ptr<mir::graphics::CursorImage>> cache;
};

} // namespace miracle

#endif // MIRACLE_XCURSOR_IMAGES_H
