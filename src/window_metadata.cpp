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