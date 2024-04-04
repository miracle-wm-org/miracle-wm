#ifndef MIRACLEWM_STUB_TILING_INTERFACE_H
#define MIRACLEWM_STUB_TILING_INTERFACE_H

#include "tiling_interface.h"

namespace miracle
{
namespace test
{

class StubTilingInterface : public TilingInterface
{
public:
    bool is_fullscreen(miral::Window const &window) override
    {
        return false;
    }

    void set_rectangle(miral::Window const &window, mir::geometry::Rectangle const &rectangle) override
    {
    }

    MirWindowState get_state(miral::Window const &window) override
    {
        return mir_window_state_attached;
    }

    void change_state(miral::Window const &window, MirWindowState state) override
    {
    }

    void clip(miral::Window const &window, mir::geometry::Rectangle const &rectangle) override
    {
    }

    void noclip(miral::Window const &window) override
    {
    }

    void select_active_window(miral::Window const &window) override
    {
    }

    std::shared_ptr<WindowMetadata> get_metadata(miral::Window const &window) override
    {
        return std::shared_ptr<WindowMetadata>();
    }

    std::shared_ptr<WindowMetadata>
    get_metadata(miral::Window const &window, TilingWindowTree const *tree) override
    {
        return std::shared_ptr<WindowMetadata>();
    }

    void raise(miral::Window const &window) override
    {
    }

    void send_to_back(miral::Window const &window) override
    {
    }
};

}
}

#endif
