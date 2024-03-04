#ifndef MIRACLEWM_WINDOW_METADATA_H
#define MIRACLEWM_WINDOW_METADATA_H

#include <memory>
#include <miral/window.h>
#include <miral/window_manager_tools.h>

namespace miracle
{
class OutputContent;
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
    WindowMetadata(WindowType type, miral::Window const& window, OutputContent* output);
    void associate_to_node(std::shared_ptr<Node> const&);
    miral::Window& get_window() { return window; }
    std::shared_ptr<Node> get_tiling_node() {
        if (type == WindowType::tiled)
            return tiling_node;
        return nullptr;
    }
    WindowType get_type() { return type; }
    OutputContent* get_output() { return output; }
    void set_restore_state(MirWindowState state);
    MirWindowState consume_restore_state();

private:

    WindowType type;
    miral::Window window;
    OutputContent* output;
    std::shared_ptr<Node> tiling_node;
    MirWindowState restore_state;
};

}

#endif //MIRACLEWM_WINDOW_METADATA_H
