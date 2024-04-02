#ifndef MIRACLEWM_TILING_INTERFACE_H
#define MIRACLEWM_TILING_INTERFACE_H

#include <miral/window.h>

namespace geom = mir::geometry;

namespace miracle
{
class WindowMetadata;
class TilingWindowTree;

class TilingInterface
{
public:
    virtual bool is_fullscreen(miral::Window const&) = 0;
    virtual void set_rectangle(miral::Window const&, geom::Rectangle const&) = 0;
    virtual MirWindowState get_state(miral::Window const&) = 0;
    virtual void change_state(miral::Window const&, MirWindowState state) = 0;
    virtual void clip(miral::Window const&, geom::Rectangle const&) = 0;
    virtual void noclip(miral::Window const&) = 0;
    virtual void select_active_window(miral::Window const&) = 0;
    virtual std::shared_ptr<WindowMetadata> get_metadata(miral::Window const&) = 0;
    virtual std::shared_ptr<WindowMetadata> get_metadata(miral::Window const&, TilingWindowTree const*) = 0;
    virtual void raise(miral::Window const&) = 0;
    virtual void send_to_back(miral::Window const&) = 0;
};

}


#endif
