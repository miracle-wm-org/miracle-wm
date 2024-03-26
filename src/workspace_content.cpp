#define MIR_LOG_COMPONENT "workspace_content"

#include "workspace_content.h"
#include "window_metadata.h"
#include "tree.h"
#include "window_helpers.h"
#include <mir/log.h>

using namespace miracle;

WorkspaceContent::WorkspaceContent(
    miracle::OutputContent *screen,
    miral::WindowManagerTools const& tools,
    int workspace,
    std::shared_ptr<MiracleConfig> const& config,
    std::shared_ptr<NodeInterface> const& node_interface)
    : tools{tools},
      tree(std::make_shared<Tree>(screen, node_interface, tools, config)),
      workspace{workspace}
{
}

int WorkspaceContent::get_workspace() const
{
    return workspace;
}

std::shared_ptr<Tree> WorkspaceContent::get_tree() const
{
    return tree;
}

void WorkspaceContent::show(std::vector<std::shared_ptr<WindowMetadata>> const& pinned_windows)
{
    tree->show();

    for (auto const& window : floating_windows)
    {
        auto metadata = window_helpers::get_metadata(window, tools);
        if (!metadata)
        {
            mir::log_error("show: floating window lacks metadata");
            continue;
        }

        miral::WindowSpecification spec;
        spec.state() = metadata->consume_restore_state();
        tools.modify_window(window, spec);
    }

    for (auto const& metadata: pinned_windows)
    {
        floating_windows.push_back(metadata->get_window());
    }
}

std::vector<std::shared_ptr<WindowMetadata>> WorkspaceContent::hide()
{
    tree->hide();

    std::vector<std::shared_ptr<WindowMetadata>> pinned_windows;
    for (auto const& window : floating_windows)
    {
        auto metadata = window_helpers::get_metadata(window, tools);
        if (!metadata)
        {
            mir::log_error("hide: floating window lacks metadata");
            continue;
        }

        if (metadata->get_is_pinned())
        {
            pinned_windows.push_back(metadata);
            break;
        }

        metadata->set_restore_state(tools.info_for(window).state());
        miral::WindowSpecification spec;
        spec.state() = mir_window_state_hidden;
        tools.modify_window(window, spec);
    }

    floating_windows.erase(std::remove_if(floating_windows.begin(), floating_windows.end(), [&](const auto&x) {
        auto metadata = window_helpers::get_metadata(x, tools);
        return std::find(pinned_windows.begin(), pinned_windows.end(), metadata) != pinned_windows.end();
    }), floating_windows.end());

    return pinned_windows;
}

bool WorkspaceContent::has_floating_window(miral::Window const& window)
{
    for (auto const& other : floating_windows)
    {
        if (other == window)
            return true;
    }

    return false;
}

void WorkspaceContent::add_floating_window(miral::Window const& window)
{
    floating_windows.push_back(window);
}

void WorkspaceContent::remove_floating_window(miral::Window const& window)
{
    floating_windows.erase(std::remove(floating_windows.begin(), floating_windows.end(), window));
}