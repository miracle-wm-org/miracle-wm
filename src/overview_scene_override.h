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

#ifndef OVERVIEW_SCENE_OVERRIDE_H
#define OVERVIEW_SCENE_OVERRIDE_H

#include "animation.h"
#include "carousel_layout.h"
#include "scene_override.h"

#include <functional>
#include <memory>
#include <mir/geometry/rectangle.h>
#include <mir_toolkit/common.h>
#include <unordered_map>
#include <vector>

namespace mir::scene
{
class Surface;
}

namespace miracle
{
class AbstractWorkspace;
class Animator;
class CompositorState;
class Config;
class OutputManager;
class OverviewSceneOverrideDelegate;
class WindowContainer;
class WindowController;
class WorkspacePreview;

/// Returns true when \p keysym is one of the keys that produce \p modifier
/// (e.g. Super_L/Super_R for meta).
bool is_modifier_keysym(MirInputEventModifier modifier, unsigned int keysym);

/// True when \p container should get a slot in the overview.
///
/// Shell components (panels, docks) and scratchpad-parked windows have no
/// workspace and never join the overview. Tiled leaves always join. Floating
/// and plugin-managed windows join only when they are toplevel, so that
/// dialogs, menus and tooltips stay visually attached to whatever spawned them
/// instead of riding around as slots of their own.
bool is_overview_window(
    std::shared_ptr<WindowContainer> const& container,
    WindowController& window_controller);

/// True when \p container is a desktop background: a shell component that sits
/// below the application depth layer and is attached on all four edges.
bool is_background_window(
    std::shared_ptr<WindowContainer> const& container,
    WindowController& window_controller);

/// True when \p container is a panel or dock: a shell component attached to at
/// least one edge of its output that is not a background.
///
/// Panels belong to an output rather than to a workspace, so there is only ever
/// one of them no matter how many workspaces the overview is showing. The
/// overview draws it once per workspace tile.
bool is_panel_window(
    std::shared_ptr<WindowContainer> const& container,
    WindowController& window_controller);

/// The overview scene: a pair of horizontal strips, one nested inside the other.
///
/// At [Level::windows] the windows of each output's active workspace are laid
/// out side by side, with the focused one front and center at full opacity and
/// its neighbours progressively smaller and dimmer as they run off both edges
/// of the screen. Tapping the primary action modifier again drops into
/// [Level::workspaces], where each of the output's workspaces is drawn as a
/// miniature desktop - windows, panel and wallpaper and all - with the active
/// one centered and the rest dimmed and cut off by the edges.
///
/// Scrolling and the left/right arrow keys move whichever strip is showing, and
/// clicking something off to one side brings it to the center. Clicking what is
/// centered, or tapping the modifier, takes it: at [Level::windows] that focuses
/// the window and dismisses the overview, and at [Level::workspaces] it switches
/// to the workspace and dismisses the overview. Escape backs out one level, and
/// out of the overview entirely from [Level::windows].
class OverviewSceneOverride : public SceneOverride
{
public:
    /// What the overview is currently showing.
    enum class Level
    {
        /// The windows of each output's active workspace.
        windows,
        /// Each output's workspaces, drawn as miniature desktops.
        workspaces
    };

    struct Entry
    {
        miral::Window window;
        /// The window's real geometry when the overview was entered.
        mir::geometry::Rectangle real;
        /// Where the entry is animating from / to.
        carousel_layout::Placement from;
        carousel_layout::Placement target;
        /// What [place] currently serves; written by the animation tick.
        carousel_layout::Placement current;
        /// Index into the group list of the output this window rides on.
        size_t group = 0;
        /// Which of that output's workspaces the window lives on.
        size_t workspace = 0;
        /// Position within the output's window strip, or nullopt for a window
        /// that is not on the active workspace and therefore only ever appears
        /// as part of a workspace tile.
        std::optional<size_t> window_index = std::nullopt;
    };

    /// A panel or wallpaper: output-level furniture that belongs to no
    /// workspace, and so is drawn once per workspace tile.
    struct ShellEntry
    {
        miral::Window window;
        mir::geometry::Rectangle real;
        size_t group = 0;
        bool is_background = false;
        /// One placement per workspace of the group, at every level, so that the
        /// animation can always lerp element by element.
        std::vector<carousel_layout::Placement> from;
        std::vector<carousel_layout::Placement> target;
        std::vector<carousel_layout::Placement> current;
    };

    /// One workspace of one output.
    struct WorkspaceSlot
    {
        std::weak_ptr<AbstractWorkspace> workspace;
        uint32_t id = 0;
        /// Identity only, for matching a container to its slot. Never dereferenced.
        AbstractWorkspace const* key = nullptr;
    };

    /// Everything about one output's strips that is fixed for the lifetime of
    /// the overview. Output changes cancel the overview outright, so none of
    /// this ever has to be updated.
    struct GroupInfo
    {
        /// Where a strip is laid out: the application zone, so that neither
        /// strip slides underneath a real panel.
        mir::geometry::Rectangle bounds;
        /// The whole output, which is what a workspace tile is a picture of.
        mir::geometry::Rectangle source;
        std::vector<WorkspaceSlot> workspaces;
        size_t active_workspace = 0;
        /// Identity of the output this group describes, so that dismissing the
        /// overview can hand the output focus over to it.
        int output_id = -1;
    };

    /// The mutable position of one strip.
    struct Strip
    {
        /// How many entries ride this strip.
        size_t count = 0;
        /// The index that is currently front and center.
        size_t position = 0;
        /// Vertical scroll that has not yet added up to a whole step.
        float scroll_accumulator = 0.f;
    };

    /// One output's pair of strips. The two keep separate positions so that
    /// backing out of the workspace strip lands on the window you left.
    struct Group
    {
        Strip windows;
        Strip workspaces;
        /// The workspace that the outro zooms up to fill the output. Normally
        /// the one that was active all along, but picking a workspace out of the
        /// workspace strip retargets it, so that the chosen tile grows into the
        /// desktop while the rest of the overview fades away.
        size_t exit_workspace = 0;
    };

    /// Builds an overview of every eligible window on every output's active
    /// workspace, laid out within that output's bounds.
    ///
    /// \p delegate is what the overview acts on the desktop through, and must
    /// outlive the override.
    ///
    /// \returns the override, or nullptr if no output carries a workspace, or the
    ///          scene is empty - no windows and no furniture - and so there would
    ///          be nothing to draw at any level.
    static std::unique_ptr<OverviewSceneOverride> create(
        OutputManager& output_manager,
        std::shared_ptr<Animator> const& animator,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<CompositorState> const& compositor_state,
        std::shared_ptr<Config> const& config,
        OverviewSceneOverrideDelegate& delegate);

    ~OverviewSceneOverride() override;

    /// Begins the intro animation. Call once, after the override has been
    /// successfully installed on the [SceneOverrideManager].
    void start();

    void handle_keyboard_event(MirKeyboardEvent const* event) override;
    void handle_pointer_event(MirPointerEvent const* event) override;
    void place(
        mir::scene::Surface const& surface,
        mir::geometry::Rectangle const& real,
        std::vector<SceneOverridePlacement>& out) override;
    void handle_window_added(miral::Window const& window) override;
    void handle_window_closed(miral::Window const& window) override;
    void handle_output_changed() override;

private:
    OverviewSceneOverride(
        std::vector<Entry> entries,
        std::vector<ShellEntry> shell_entries,
        std::vector<GroupInfo> groups,
        size_t active_group,
        std::shared_ptr<Animator> const& animator,
        std::shared_ptr<WindowController> const& window_controller,
        std::shared_ptr<CompositorState> const& compositor_state,
        std::shared_ptr<Config> const& config,
        OverviewSceneOverrideDelegate& delegate);

    enum class Phase
    {
        intro,
        idle,
        outro,
        done
    };

    /// All mutable state lives here so that animation callbacks can hold a
    /// weak reference to it instead of `this`, making it safe to destroy the
    /// override on any thread while a tick is in flight.
    struct State
    {
        std::mutex mutex;
        Phase phase = Phase::intro;
        Level level = Level::windows;
        float t = 0.f;
        /// Hit-test order: most recently used first.
        std::vector<mir::scene::Surface const*> order;
        std::unordered_map<mir::scene::Surface const*, Entry> entries;
        std::unordered_map<mir::scene::Surface const*, ShellEntry> shell_entries;
        /// One pair of strips per output, indexed by [Entry::group].
        std::vector<Group> groups;
        /// The output that scrolling and dismissal act on.
        size_t active_group = 0;
        /// Whether the inactive workspaces have been forced into the scene yet.
        /// The reveal is deferred until [Level::workspaces] is first entered, so
        /// that a user who only ever taps once never pays for it.
        bool revealed = false;
    };

    /// Animates every entry from its `from` placement to its `target`
    /// placement, then runs \p on_complete (on the animator thread).
    ///
    /// \p duration overrides the configured animation duration, which the
    /// short retargeting animations of a scroll or a click use so that the
    /// overview keeps up with the input.
    void animate(std::function<void()> on_complete, std::optional<float> duration = std::nullopt);

    /// Recomputes \p group's targets for the current level and its strips'
    /// positions, and rebases every entry's animation on wherever it happens to
    /// be right now.
    ///
    /// The caller must hold `state->mutex`, and must run [animate] afterwards.
    void retarget(size_t group);

    /// The workspace tiles of \p group at its current workspace position.
    ///
    /// The caller must hold `state->mutex`.
    [[nodiscard]] std::vector<carousel_layout::Placement> tiles_of(size_t group) const;

    /// Moves the active output's current strip \p delta along, stopping at
    /// either end, and animates it there.
    void step(int delta);

    /// Moves \p group's current strip \p delta along, clamped to the ends, and
    /// retargets it if it actually moved.
    ///
    /// The caller must hold `state->mutex`, and must run [settle] afterwards
    /// when this returns true.
    ///
    /// \returns whether the strip moved.
    bool advance(size_t group, int delta);

    /// The strip that the current level scrolls.
    ///
    /// The caller must hold `state->mutex`.
    Strip& current_strip(size_t group);

    /// Runs the short animation that settles the overview after it has been
    /// moved by a scroll, an arrow key, a click or a change of level.
    void settle();

    /// The keys of \p group's window-strip entries, ordered by
    /// [Entry::window_index].
    ///
    /// The caller must hold `state->mutex`.
    [[nodiscard]] std::vector<mir::scene::Surface const*> window_strip_keys(size_t group) const;

    /// The window that is front and center on \p group's window strip, if any.
    ///
    /// The caller must hold `state->mutex`.
    [[nodiscard]] std::optional<miral::Window> centered_window(size_t group) const;

    /// Drops into [Level::workspaces], revealing every workspace that is not
    /// currently in the scene. Runs on the window management thread.
    void enter_workspaces();

    /// Backs out of [Level::workspaces]. The reveal is deliberately kept, so
    /// that flipping between levels stays cheap.
    void return_to_windows();

    /// Focuses whatever is front and center on the active output's window
    /// strip, then exits.
    void commit_and_exit();

    /// Switches to whatever workspace is front and center on the active
    /// output's workspace strip, then exits.
    ///
    /// The outro zooms that workspace's tile up until it fills the output while
    /// everything else fades out, so the workspace switch itself must not
    /// animate: by the time it runs, the workspace is already on screen and it
    /// only has to be adopted.
    void commit_workspace_and_exit();

    void begin_exit();
    /// Tears the override down immediately, without an exit animation.
    void cancel();
    /// Puts every workspace that [enter_workspaces] revealed back into hiding
    /// and hands focus back to whatever had it before the reveal.
    ///
    /// Only for the paths that end the overview without choosing anything; the
    /// outro releases the preview itself, after whatever it landed on has taken
    /// the focus.
    ///
    /// Must run on the window management thread.
    void conceal_workspaces();
    static void nudge(miral::Window const& window);

    /// Which group and workspace \p container belongs to, or nullopt when it is
    /// on a workspace that this overview does not cover.
    [[nodiscard]] std::optional<std::pair<size_t, size_t>> locate(
        std::shared_ptr<WindowContainer> const& container) const;

    std::shared_ptr<State> state;
    /// One description per output, indexed by [Entry::group].
    std::vector<GroupInfo> const groups;
    std::shared_ptr<Animator> animator;
    std::shared_ptr<WindowController> window_controller;
    std::shared_ptr<CompositorState> compositor_state;
    std::shared_ptr<WorkspacePreview> preview;
    OverviewSceneOverrideDelegate* delegate;
    AnimationHandle animation_handle;
    AnimationDefinition definition;
    MirInputEventModifier primary_modifier;
    bool primary_tap_latched = false;
    std::optional<miral::Window> focus_before_reveal;
    std::optional<uint32_t> selected_workspace;
};
}

#endif
