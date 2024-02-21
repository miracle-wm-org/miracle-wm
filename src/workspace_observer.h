#ifndef MIRACLEWM_WORKSPACE_OBSERVER_H
#define MIRACLEWM_WORKSPACE_OBSERVER_H

#include "screen.h"
#include <mir/executor.h>
#include <memory>

namespace miracle
{

class Screen;

class WorkspaceObserver
{
public:
    virtual ~WorkspaceObserver() = default;
    virtual void on_created(std::shared_ptr<Screen> const&, int) = 0;
    virtual void on_removed(std::shared_ptr<Screen> const&, int) = 0;
    virtual void on_focused(std::shared_ptr<Screen> const& previous, int, std::shared_ptr<Screen> const& current, int) = 0;
};

class WorkspaceObserverRegistrar
{
public:
    WorkspaceObserverRegistrar() = default;
    void register_interest(std::weak_ptr<WorkspaceObserver>);
    void unregister_interest(WorkspaceObserver&);
    void advise_created(std::shared_ptr<Screen> const&, int);
    void advise_removed(std::shared_ptr<Screen> const&, int);
    void advise_focused(std::shared_ptr<Screen> const& previous, int, std::shared_ptr<Screen> const& current, int);

private:
    std::vector<std::weak_ptr<WorkspaceObserver>> observers;
};

} // miracle

#endif //MIRACLEWM_WORKSPACE_OBSERVER_H
