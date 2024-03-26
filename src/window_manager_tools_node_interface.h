#ifndef MIRACLEWM_WINDOW_MANAGER_TOOLS_NODE_INTERFACE_H
#define MIRACLEWM_WINDOW_MANAGER_TOOLS_NODE_INTERFACE_H

#include "node_interface.h"
#include <miral/window_manager_tools.h>

namespace miracle
{
class WindowManagerToolsNodeInterface : public NodeInterface
{
public:
    WindowManagerToolsNodeInterface(miral::WindowManagerTools const&);
    bool is_fullscreen(miral::Window const&) override;
    void set_rectangle(miral::Window const&, geom::Rectangle const&) override;
    void show(miral::Window const&) override;
    void hide(miral::Window const&) override;
    void clip(miral::Window const&, geom::Rectangle const&) override;
    void noclip(miral::Window const&) override;

private:
    miral::WindowManagerTools tools;
};
}


#endif //MIRACLEWM_WINDOW_MANAGER_TOOLS_NODE_INTERFACE_H
