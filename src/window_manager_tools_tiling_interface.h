#ifndef MIRACLEWM_WINDOW_MANAGER_TOOLS_TILING_INTERFACE_H
#define MIRACLEWM_WINDOW_MANAGER_TOOLS_TILING_INTERFACE_H

#include "tiling_interface.h"
#include <miral/window_manager_tools.h>

namespace miracle
{
class WindowManagerToolsTilingInterface : public TilingInterface
{
public:
    WindowManagerToolsTilingInterface(miral::WindowManagerTools const&);
    bool is_fullscreen(miral::Window const&) override;
    void set_rectangle(miral::Window const&, geom::Rectangle const&) override;
    MirWindowState get_state(miral::Window const&) override;
    void change_state(miral::Window const&, MirWindowState state)override;
    void clip(miral::Window const&, geom::Rectangle const&) override;
    void noclip(miral::Window const&) override;

private:
    miral::WindowManagerTools tools;
};
}


#endif //MIRACLEWM_WINDOW_MANAGER_TOOLS_TILING_INTERFACE_H
