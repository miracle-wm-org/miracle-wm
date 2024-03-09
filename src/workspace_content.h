#ifndef MIRACLEWM_WORKSPACE_CONTENT_H
#define MIRACLEWM_WORKSPACE_CONTENT_H

#include <miral/window_manager_tools.h>
#include <miral/minimal_window_manager.h>

namespace miracle
{
class OutputContent;
class MiracleConfig;
class Tree;
class WindowMetadata;

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
    void show(std::vector<std::shared_ptr<WindowMetadata>> const&);
    std::vector<std::shared_ptr<WindowMetadata>> hide();

    bool has_floating_window(miral::Window const&);
    void add_floating_window(miral::Window const&);
    void remove_floating_window(miral::Window const&);
    std::vector<miral::Window> const& get_floating_windows() { return floating_windows; }

private:
    miral::WindowManagerTools tools;
    std::shared_ptr<Tree> tree;
    int workspace;
    std::vector<miral::Window> floating_windows;
};

} // miracle

#endif //MIRACLEWM_WORKSPACE_CONTENT_H
