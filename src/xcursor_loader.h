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


#ifndef MIRAL_CURSOR_LOADER_H_
#define MIRAL_CURSOR_LOADER_H_

#include <mir/input/cursor_images.h>

#include <memory>
#include <string>
#include <map>
#include <mutex>

// Unfortunately this library does not compile as C++ so we can not namespace it.
extern "C"
{
struct _XcursorImages;
}

namespace mir { namespace graphics { class CursorImage; } }

namespace miral
{
class XCursorLoader : public mir::input::CursorImages
{
public:
    XCursorLoader();

    explicit XCursorLoader(std::string const& theme);

    virtual ~XCursorLoader() = default;

    std::shared_ptr<mir::graphics::CursorImage> image(std::string const& cursor_name, mir::geometry::Size const& size);

    void load_cursor_theme(std::string const& theme_name);

protected:
    XCursorLoader(XCursorLoader const&) = delete;
    XCursorLoader& operator=(XCursorLoader const&) = delete;

private:
    std::mutex guard;

    std::map<std::string, std::shared_ptr<mir::graphics::CursorImage>> loaded_images;

    void load_appropriately_sized_image(_XcursorImages *images);
};
}


#endif /* MIRAL_CURSOR_LOADER_H_ */
