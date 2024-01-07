#include "window_helpers.h"

bool miracle::window_helpers::is_window_fullscreen(MirWindowState state)
{
    return state == mir_window_state_fullscreen
           || state == mir_window_state_maximized
           || state == mir_window_state_horizmaximized
           || state == mir_window_state_vertmaximized;
}