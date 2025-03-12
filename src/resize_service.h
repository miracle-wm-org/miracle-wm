#ifndef MIRACLE_RESIZE_SERVICE_H
#define MIRACLE_RESIZE_SERVICE_H

#include "command_controller.h"
#include "compositor_state.h"
#include "config.h"
#include "output_manager.h"

namespace miracle
{

class ResizeService
{
public:
    ResizeService(
        std::shared_ptr<CommandController> const& command_controller,
        std::shared_ptr<Config> const& config,
        std::shared_ptr<CompositorState> const& state,
        std::shared_ptr<OutputManager> const& output_manager);

    bool handle_pointer_event(float x, float y, MirPointerAction action, MirPointerButtons buttons);
    void handle_request_resize(std::shared_ptr<Container> const& container, MirPointerAction action, MirResizeEdge edge);

private:
    void stop();

    std::shared_ptr<CommandController> command_controller;
    std::shared_ptr<Config> config;
    std::shared_ptr<CompositorState> state;
    std::shared_ptr<OutputManager> output_manager;

    std::weak_ptr<Container> resizing_container;
    MirResizeEdge resize_edge = mir_resize_edge_none;
    bool is_resizing = false;
};

} // namespace miracle

#endif // MIRACLE_RESIZE_SERVICE_H
