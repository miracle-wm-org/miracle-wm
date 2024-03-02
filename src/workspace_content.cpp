#define MIR_LOG_COMPONENT "workspace_content"

#include "workspace_content.h"
#include "tree.h"
#include "window_helpers.h"
#include <mir/log.h>

using namespace miracle;

WorkspaceContent::WorkspaceContent(
    miracle::OutputContent *screen,
    miral::WindowManagerTools const& tools,
    int workspace,
    std::shared_ptr<MiracleConfig> const& config)
    : tools{tools},
      tree(std::make_shared<Tree>(screen, tools, config)),
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

void WorkspaceContent::show()
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
}

void WorkspaceContent::hide()
{
    tree->hide();

    for (auto const& window : floating_windows)
    {
        auto metadata = window_helpers::get_metadata(window, tools);
        if (!metadata)
        {
            mir::log_error("hide: floating window lacks metadata");
            continue;
        }

        metadata->set_restore_state(tools.info_for(window).state());
        miral::WindowSpecification spec;
        spec.state() = mir_window_state_hidden;
        tools.modify_window(window, spec);
    }
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