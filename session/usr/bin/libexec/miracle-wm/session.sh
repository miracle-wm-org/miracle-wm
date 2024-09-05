#!/bin/sh
#
#
# Copyright (C) 2024  Matthew Kosarek
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Portions of this code originate from https://github.com/alebastr/sway-systemd,
# licensed under the MIT license. See the LICENSE.sway-systemd file for details.
#
#
#
# Address several issues with DBus activation and systemd user sessions
#
# 1. DBus-activated and systemd services do not share the environment with user
#    login session. In order to make the applications that have GUI or interact
#    with the compositor work as a systemd user service, certain variables must
#    be propagated to the systemd and dbus.
#    Possible (but not exhaustive) list of variables:
#    - DISPLAY - for X11 applications that are started as user session services
#    - WAYLAND_DISPLAY - similarly, this is needed for wayland-native services
#    - I3SOCK/SWAYSOCK - allow services to talk with sway using i3 IPC protocol
#
# 2. `xdg-desktop-portal` requires XDG_CURRENT_DESKTOP to be set in order to
#    select the right implementation for screenshot and screencast portals.
#    With all the numerous ways to start sway, it's not possible to rely on the
#    right value of the XDG_CURRENT_DESKTOP variable within the login session,
#    therefore the script will ensure that it is always set to `sway`.
#
# 3. GUI applications started as a systemd service (or via xdg-autostart-generator)
#    may rely on the XDG_SESSION_TYPE variable to select the backend.
#    Ensure that it is always set to `wayland`.
#
# 4. The common way to autostart a systemd service along with the desktop
#    environment is to add it to a `graphical-session.target`. However, systemd
#    forbids starting the graphical session target directly and encourages use
#    of an environment-specific target units. Therefore, the integration
#    package here provides and uses `miracle-wm-session.target` which would bind to
#    the `graphical-session.target`.
#
# 5. Stop the target and unset the variables when the compositor exits.
#
# References:
#  - https://github.com/swaywm/sway/wiki#gtk-applications-take-20-seconds-to-start
#  - https://github.com/emersion/xdg-desktop-portal-wlr/wiki/systemd-user-services,-pam,-and-environment-variables
#  - https://www.freedesktop.org/software/systemd/man/systemd.special.html#graphical-session.target
#  - https://systemd.io/DESKTOP_ENVIRONMENTS/
#
export XDG_CURRENT_DESKTOP=mir:miracle-wm
export XDG_SESSION_DESKTOP="${XDG_SESSION_DESKTOP:-miracle-wm}"
export XDG_SESSION_TYPE=wayland
VARIABLES="DESKTOP_SESSION XDG_CURRENT_DESKTOP XDG_SESSION_DESKTOP XDG_SESSION_TYPE"
VARIABLES="${VARIABLES} DISPLAY I3SOCK SWAYSOCK WAYLAND_DISPLAY"
SESSION_TARGET="miracle-wm-session.target"
SESSION_SHUTDOWN_TARGET="miracle-wm-session-shutdown.target"
WITH_CLEANUP=1

print_usage() {
    cat <<EOH
Usage:
  --help            Show this help message and exit.
  --add-env NAME, -E NAME
                    Add a variable name to the subset of environment passed
                    to the user session. Can be specified multiple times.
  --no-cleanup      Skip cleanup code at compositor exit.
EOH
}

while [ $# -gt 0 ]; do
    case "$1" in
    --help)
        print_usage
        exit 0 ;;
    # The following flag is intentionally not exposed in the usage info:
    #  - I don't believe that's the right or safe thing to do;
    #  - systemd upstream is of the same opinion and has already deprecated
    #    the ability to import the full environment (systemd/systemd#18137)
    --all-environment)
        VARIABLES="" ;;
    --add-env=?*)
        VARIABLES="${VARIABLES} ${1#*=}" ;;
    --add-env | -E)
        shift
        VARIABLES="${VARIABLES} ${1}" ;;
    --with-cleanup)
        ;; # ignore (enabled by default)
    --no-cleanup)
        unset WITH_CLEANUP ;;
    -*)
        echo "Unexpected option: $1" 1>&2
        print_usage
        exit 1 ;;
    *)
        break ;;
    esac
    shift
done

# Check if another Miracle session is already active.
#
# Ignores all other kinds of parallel or nested sessions
# (Miracle on Gnome/KDE/X11/etc.), as the only way to detect these is to check
# for (WAYLAND_)?DISPLAY and that is know to be broken on Arch.
if systemctl --user -q is-active "$SESSION_TARGET"; then
    echo "Another session found; refusing to overwrite the variables"
    exit 1
fi

# DBus activation environment is independent from systemd. While most of
# dbus-activated services are already using `SystemdService` directive, some
# still don't and thus we should set the dbus environment with a separate
# command.
if hash dbus-update-activation-environment 2>/dev/null; then
    # shellcheck disable=SC2086
    dbus-update-activation-environment --systemd ${VARIABLES:- --all}
fi

# reset failed state of all user units
systemctl --user reset-failed

# shellcheck disable=SC2086
systemctl --user import-environment $VARIABLES
systemctl --user start "$SESSION_TARGET"

# Optionally, wait until the compositor exits and cleanup variables and services.
if [ -z "$WITH_CLEANUP" ] ||
    [ -z "$SWAYSOCK" ] ||
    ! hash miraclemsg 2>/dev/null
then
    exit 0;
fi

# declare cleanup handler and run it on script termination via kill or Ctrl-C
session_cleanup () {
    # stop the session target and unset the variables
    systemctl --user start --job-mode=replace-irreversibly "$SESSION_SHUTDOWN_TARGET"
    if [ -n "$VARIABLES" ]; then
        # shellcheck disable=SC2086
        systemctl --user unset-environment $VARIABLES
    fi
}
trap session_cleanup INT TERM
# wait until the compositor exits
miraclemsg -t subscribe '["shutdown"]'
# run cleanup handler on normal exit
session_cleanup
