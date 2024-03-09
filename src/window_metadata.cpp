#include "window_metadata.h"

using namespace miracle;

WindowMetadata::WindowMetadata(
    miracle::WindowType type,
    miral::Window const& window,
    OutputContent* output)
    : type{type},
      window{window},
      output{output}
{}

void WindowMetadata::associate_to_node(std::shared_ptr<Node> const& node)
{
    tiling_node = node;
}

void WindowMetadata::set_restore_state(MirWindowState state)
{
    restore_state = state;
}

MirWindowState WindowMetadata::consume_restore_state()
{
    return restore_state;
}

void WindowMetadata::toggle_pin_to_desktop()
{
    if (type == WindowType::floating)
    {
        is_pinned = !is_pinned;
    }
}