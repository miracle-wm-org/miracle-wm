#ifndef MIRACLEWM_WORKSPACE_CONTENT_H
#define MIRACLEWM_WORKSPACE_CONTENT_H

#include <miral/window_manager_tools.h>

namespace miracle
{
class OutputContent;
class MiracleConfig;
class Tree;

class WorkspaceContent
{
public:
    WorkspaceContent(
        OutputContent* screen,
        miral::WindowManagerTools const& tools,
        int workspace,
        std::shared_ptr<MiracleConfig> const& config);

    [[nodiscard]] int get_workspace() const;
    [[nodiscard]] std::shared_ptr<Tree> get_tree() const;
    void show();
    void hide();

private:
    std::shared_ptr<Tree> tree;
    int workspace;
};

} // miracle

#endif //MIRACLEWM_WORKSPACE_CONTENT_H
