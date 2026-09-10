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

#ifndef MIRACLEWM_WINDOW_HELPERS_H
#define MIRACLEWM_WINDOW_HELPERS_H

#include "mir_version_manager.h"

#include <miral/window_info.h>
#include <miral/window_specification.h>

namespace miracle
{
namespace window_helpers
{
    miral::WindowSpecification copy_from(miral::WindowInfo const&);

    /// Unset (clear) a miral WindowSpecification optional field, supporting both the
    /// modern std::optional API (Mir >= 2.29) and the legacy mir::optional_value API.
    template <typename Optional>
    inline void reset_optional(Optional& optional)
    {
#ifdef MIR_VERSION_2_29_OR_GREATER
        optional.reset();
#else
        optional.consume();
#endif
    }
}
}

#endif // MIRACLEWM_WINDOW_HELPERS_H
