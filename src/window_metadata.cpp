#include "window_metadata.h"

using namespace miracle;

WindowMetadata::WindowMetadata(miracle::WindowType type, miral::Window const& window)
    : type{type},
      window{window}
{}

void WindowMetadata::associate_to_node(std::shared_ptr<Node> const& node)
{
    tiling_node = node;
}