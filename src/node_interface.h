#ifndef MIRACLEWM_NODE_INTERFACE_H
#define MIRACLEWM_NODE_INTERFACE_H

#include <miral/window.h>

namespace geom = mir::geometry;

namespace miracle
{

class NodeInterface
{
public:
    virtual bool is_fullscreen(miral::Window const&);
    virtual void set_rectangle(miral::Window const&, geom::Rectangle const&);
    virtual void show(miral::Window const&);
    virtual void hide(miral::Window const&);
};

}


#endif
