/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "xcursor_loader.h"

#include <mir/graphics/cursor_image.h>

#include <stdexcept>

#include <mir_toolkit/cursors.h>

// Unfortunately this can not be compiled as C++...so we can not namespace
// these symbols. In order to differentiate from internal symbols
//  we refer to them via their _ prefixed version, i.e. _XcursorImage
extern "C"
{
#include "xcursor.h"
}

namespace mg = mir::graphics;
namespace mi = mir::input;
namespace geom = mir::geometry;

namespace
{
class XCursorImage : public mg::CursorImage
{
public:
    XCursorImage(_XcursorImage *image, std::shared_ptr<_XcursorImages> const& save_resource)
        : image(image),
          save_resource(save_resource)
    {
    }

    ~XCursorImage()
    {
    }

    void const* as_argb_8888() const override
    {
        return image->pixels;
    }
    geom::Size size() const override
    {
        return {image->width, image->height};
    }
    geom::Displacement hotspot() const override
    {
        return {image->xhot, image->yhot};
    }

private:
    _XcursorImage *image;
    std::shared_ptr<_XcursorImages> const save_resource;
};

std::string const
xcursor_name_for_mir_cursor(std::string const& mir_cursor_name)
{
    if (mir_cursor_name == mir_default_cursor_name)
    {
        return "arrow";
    }
    else if (mir_cursor_name == mir_arrow_cursor_name)
    {
        return "arrow";
    }
    else if (mir_cursor_name == mir_busy_cursor_name)
    {
        return "watch";
    }
    else if (mir_cursor_name == mir_caret_cursor_name)
    {
        return "xterm"; // Yep
    }
    else if (mir_cursor_name == mir_pointing_hand_cursor_name)
    {
        return "hand2";
    }
    else if (mir_cursor_name == mir_open_hand_cursor_name)
    {
        return "hand";
    }
    else if (mir_cursor_name == mir_closed_hand_cursor_name)
    {
        return "grabbing";
    }
    else if (mir_cursor_name == mir_horizontal_resize_cursor_name)
    {
        return "h_double_arrow";
    }
    else if (mir_cursor_name == mir_vertical_resize_cursor_name)
    {
        return "v_double_arrow";
    }
    else if (mir_cursor_name == mir_diagonal_resize_bottom_to_top_cursor_name)
    {
        return "top_right_corner";
    }
    else if (mir_cursor_name == mir_diagonal_resize_top_to_bottom_cursor_name)
    {
        return "bottom_right_corner";
    }
    // else if (mir_cursor_name == mir_diagonal_resize_top_to_left_cursor_name)
    // {
    //     return "top_left_corner";
    // }
    // else if (mir_cursor_name == mir_diagonal_resize_bottom_to_left_cursor_name)
    // {
    //     return "bottom_left_corner";
    // }
    else if (mir_cursor_name == mir_omnidirectional_resize_cursor_name)
    {
        return "fleur";
    }
    else if (mir_cursor_name == mir_vsplit_resize_cursor_name)
    {
        return "v_double_arrow";
    }
    else if (mir_cursor_name == mir_hsplit_resize_cursor_name)
    {
        return "h_double_arrow";
    }
    else if (mir_cursor_name == mir_crosshair_cursor_name)
    {
        return "crosshair";
    }
    else
    {
        return mir_cursor_name;
    }
}
}

miral::XCursorLoader::XCursorLoader()
{
    load_cursor_theme("default");
}

miral::XCursorLoader::XCursorLoader(std::string const& theme)
{
    load_cursor_theme(theme);
}

// Each XcursorImages represents images for the different sizes of a given symbolic cursor.
void miral::XCursorLoader::load_appropriately_sized_image(_XcursorImages *images)
{
    // We would rather take this lock in load_cursor_theme but the Xcursor lib style
    // makes it difficult to use our standard 'pass the lg around to _locked members' pattern
    std::lock_guard lg(guard);

    // We have to save all the images as XCursor expects us to free them.
    // This contains the actual image data though, so we need to ensure they stay alive
    // with the lifetime of the mg::CursorImage instance which refers to them.
    auto saved_xcursor_library_resource = std::shared_ptr<_XcursorImages>(images, [](_XcursorImages *images)
        {
            XcursorImagesDestroy(images);
        });

    for (int i = 0; i < images->nimage; i++)
    {
        _XcursorImage *candidate = images->images[i];
        if (candidate->width == mi::default_cursor_size.width.as_uint32_t() &&
            candidate->height == mi::default_cursor_size.height.as_uint32_t())
        {
            loaded_images[std::string(images->name)] = std::make_shared<XCursorImage>(candidate, saved_xcursor_library_resource);
            return;
        }
    }

    loaded_images[std::string(images->name)] = std::make_shared<XCursorImage>(images->images[0], saved_xcursor_library_resource);
}

void miral::XCursorLoader::load_cursor_theme(std::string const& theme_name)
{
    // Cursors are named by their square dimension...called the nominal size in XCursor terminology, so we just look up by width.
    // Later we verify the actual size.
    xcursor_load_theme(theme_name.c_str(), mi::default_cursor_size.width.as_uint32_t(),
        [](XcursorImages* images, void *this_ptr)  -> void
        {
            // Can't use lambda capture as this lambda is thunked to a C function ptr
            auto p = static_cast<miral::XCursorLoader*>(this_ptr);
            p->load_appropriately_sized_image(images);
        }, this);
}

std::shared_ptr<mg::CursorImage> miral::XCursorLoader::image(
    std::string const& cursor_name,
    geom::Size const& /*size*/)
{

    auto const potential_cursor_names = {
        cursor_name,
        xcursor_name_for_mir_cursor(cursor_name),
        std::string{"arrow"},
    };

    std::lock_guard lg(guard);
    for(auto const& name : potential_cursor_names)
    {
        if (auto it = loaded_images.find(name); it != loaded_images.end())
            return it->second;
    }

    return nullptr;
}
