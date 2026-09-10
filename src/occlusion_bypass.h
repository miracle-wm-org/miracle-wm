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

#ifndef OCCLUSION_BYPASS_H
#define OCCLUSION_BYPASS_H

#include <memory>
#include <vector>

namespace mir::scene
{
class Surface;
}

namespace miracle
{
class WindowContainer;

/// Keeps windows that are covered up out of the compositor's occlusion cull for
/// as long as the bypass is held.
///
/// TODO: This is largely a hack for the wffects system.
///
/// Every method must be called on the window management thread. An effect whose
/// teardown runs elsewhere (the animator thread, say) should hand the release
/// to [WindowController::invoke_under_lock].
class OcclusionBypass
{
public:
    OcclusionBypass() = default;
    OcclusionBypass(OcclusionBypass const&) = delete;
    OcclusionBypass& operator=(OcclusionBypass const&) = delete;

    /// Releases the bypass, in case the holder forgot to.
    ~OcclusionBypass();

    /// Bypasses the cull for every container in \p containers that is not
    /// already bypassed, remembering the ones it flagged.
    ///
    /// Additive and idempotent, so an effect can call this again as windows
    /// join it.
    void acquire(std::vector<std::shared_ptr<WindowContainer>> const& containers);

    /// Forgets the container holding \p surface without touching it.
    ///
    /// For a window that is going away under us: its transform belongs to
    /// whoever is animating it out, and clearing the bypass would fight them.
    void forget(mir::scene::Surface const* surface);

    /// Puts everything [acquire] flagged back into the cull. Idempotent.
    void release();

    /// Whether anything is currently bypassed.
    [[nodiscard]] bool held() const;

private:
    struct Flagged
    {
        /// Held weakly: a container that is destroyed mid-bypass simply stops
        /// resolving, and there is nothing left to undo - its surface is dying
        /// and its transform is no longer ours.
        std::weak_ptr<WindowContainer> container;

        /// The container's surface, kept for identity only so that [forget] can
        /// find it without resolving the container.
        mir::scene::Surface const* key = nullptr;
    };

    std::vector<Flagged> flagged;
};
}

#endif
