#ifndef MIRACLEWM_WINDOW_METADATA_H
#define MIRACLEWM_WINDOW_METADATA_H

#include <memory>
#include <miral/window.h>
#include <miral/window_manager_tools.h>

namespace miracle
{

class Node;

enum class WindowType
{
    none,
    tiled,
    floating,
    other
};

/// Applied to WindowInfo to enable
class WindowMetadata
{
public:
    WindowMetadata(WindowType type, miral::Window const& window);
    void associate_to_node(std::shared_ptr<Node> const&);
    miral::Window& get_window() { return window; }
    std::shared_ptr<Node> get_tiling_node() {
        if (type == WindowType::tiled)
            return tiling_node;
        return nullptr;
    }
    WindowType get_type() { return type; }

private:

    WindowType type;
    miral::Window window;
    std::shared_ptr<Node> tiling_node;
};

}

#endif //MIRACLEWM_WINDOW_METADATA_H
