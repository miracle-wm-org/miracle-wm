//
// Created by mattkae on 9/8/23.
//

#ifndef MIRCOMPOSITOR_DISPLAY_LISTENER_H
#define MIRCOMPOSITOR_DISPLAY_LISTENER_H


namespace mir
{
class Server;
}

namespace mirie
{
class DisplayListener
{
public:
    DisplayListener() = default;
    void operator()(mir::Server& server) const;
};
}


#endif //MIRCOMPOSITOR_DISPLAY_LISTENER_H
