#ifndef MIRACLEWM_NODE_INTERFACE_H
#define MIRACLEWM_NODE_INTERFACE_H

#include <miral/window.h>

namespace geom = mir::geometry;

namespace miracle
{

class NodeInterface
{
public:
    virtual bool is_fullscreen(miral::Window const&) = 0;
    virtual void set_rectangle(miral::Window const&, geom::Rectangle const&) = 0;
    virtual MirWindowState get_state(miral::Window const&) = 0;
    virtual void change_state(miral::Window const&, MirWindowState state) = 0;
    virtual void clip(miral::Window const&, geom::Rectangle const&) = 0;
    virtual void noclip(miral::Window const&) = 0;
};

}


#endif
