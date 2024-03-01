#include "workspace_content.h"
#include "tree.h"

using namespace miracle;

WorkspaceContent::WorkspaceContent(
    miracle::OutputContent *screen,
    miral::WindowManagerTools const& tools,
    int workspace,
    std::shared_ptr<MiracleConfig> const& config)
    : tree(std::make_shared<Tree>(screen, tools, config)),
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
}

void WorkspaceContent::hide()
{
    tree->hide();
}