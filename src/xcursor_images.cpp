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

#define MIR_LOG_COMPONENT "miracle-xcursor-images"

#include "xcursor_images.h"

#include <X11/Xcursor/Xcursor.h>
#include <mir/graphics/cursor_image.h>
#include <mir/log.h>
#include <mir_toolkit/cursors.h>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
/// Owns the [XcursorImages] that the pixels of [image] live inside of.
class LoadedCursorImage : public mg::CursorImage
{
public:
    LoadedCursorImage(XcursorImage* image, std::shared_ptr<XcursorImages> const& owner) :
        image { image },
        owner { owner }
    {
    }

    void const* as_argb_8888() const override { return image->pixels; }

    geom::Size size() const override
    {
        return { image->width, image->height };
    }

    geom::Displacement hotspot() const override
    {
        return { image->xhot, image->yhot };
    }

private:
    XcursorImage* image;
    std::shared_ptr<XcursorImages> owner;
};

/// Themes in the wild are still shipped with the legacy X11 cursor names, whereas
/// [mir_toolkit/cursors.h] hands out the XDG/CSS names ("ew-resize" and friends).
/// This is the same aliasing table that Mir's own XCursor loader applies.
std::string legacy_name_for(std::string const& name)
{
    if (name == mir_default_cursor_name || name == mir_arrow_cursor_name)
        return "arrow";
    if (name == mir_busy_cursor_name)
        return "watch";
    if (name == mir_caret_cursor_name)
        return "xterm";
    if (name == mir_pointing_hand_cursor_name)
        return "hand2";
    if (name == mir_open_hand_cursor_name)
        return "hand";
    if (name == mir_closed_hand_cursor_name)
        return "grabbing";
    if (name == mir_horizontal_resize_cursor_name || name == mir_hsplit_resize_cursor_name)
        return "h_double_arrow";
    if (name == mir_vertical_resize_cursor_name || name == mir_vsplit_resize_cursor_name)
        return "v_double_arrow";
    if (name == mir_diagonal_resize_bottom_to_top_cursor_name)
        return "top_right_corner";
    if (name == mir_diagonal_resize_top_to_bottom_cursor_name)
        return "bottom_right_corner";
    if (name == mir_diagonal_resize_top_to_left_cursor_name)
        return "top_left_corner";
    if (name == mir_diagonal_resize_bottom_to_left_cursor_name)
        return "bottom_left_corner";
    if (name == mir_omnidirectional_resize_cursor_name)
        return "fleur";
    if (name == mir_crosshair_cursor_name)
        return "crosshair";
    return name;
}

std::vector<std::string> split_themes(std::string const& theme_list)
{
    std::vector<std::string> result;
    size_t start = 0;
    while (start <= theme_list.size())
    {
        auto const end = theme_list.find(':', start);
        auto const theme = theme_list.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!theme.empty())
            result.push_back(theme);

        if (end == std::string::npos)
            break;

        start = end + 1;
    }

    if (result.empty())
        result.emplace_back("default");

    return result;
}
}

namespace miracle
{

XCursorImages::XCursorImages(std::string const& theme_list) :
    themes { split_themes(theme_list) }
{
}

std::shared_ptr<mg::CursorImage> XCursorImages::load(std::string const& name, uint32_t size)
{
    for (auto const& theme : themes)
    {
        auto* const images = XcursorLibraryLoadImages(name.c_str(), theme.c_str(), static_cast<int>(size));
        if (!images)
            continue;

        if (images->nimage <= 0)
        {
            XcursorImagesDestroy(images);
            continue;
        }

        auto const owner = std::shared_ptr<XcursorImages>(images, [](XcursorImages* i)
        { XcursorImagesDestroy(i); });

        // XCursor may hand back an image at a different nominal size than requested,
        // so prefer an exact match and fall back to whatever came first.
        for (int i = 0; i < owner->nimage; i++)
        {
            auto* const candidate = owner->images[i];
            if (candidate->width == size && candidate->height == size)
                return std::make_shared<LoadedCursorImage>(candidate, owner);
        }

        return std::make_shared<LoadedCursorImage>(owner->images[0], owner);
    }

    return nullptr;
}

std::shared_ptr<mg::CursorImage> XCursorImages::image(
    std::string const& cursor_name, geom::Size const& size)
{
    auto const width = size.width.as_uint32_t();
    auto const key = std::make_pair(cursor_name, width);

    std::lock_guard lock { mutex_ };
    if (auto const it = cache.find(key); it != cache.end())
        return it->second;

    std::shared_ptr<mg::CursorImage> result;
    for (auto const& candidate : { cursor_name, legacy_name_for(cursor_name), std::string { "arrow" } })
    {
        result = load(candidate, width);
        if (result)
            break;
    }

    cache[key] = result;
    return result;
}

bool XCursorImages::has_default_cursor()
{
    return image(mir_default_cursor_name, mir::input::default_cursor_size) != nullptr;
}

} // namespace miracle
