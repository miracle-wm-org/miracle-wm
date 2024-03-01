#ifndef MIRACLEWM_WORKSPACE_OBSERVER_H
#define MIRACLEWM_WORKSPACE_OBSERVER_H

#include "output_content.h"
#include <mir/executor.h>
#include <memory>

namespace miracle
{

class OutputContent;

class WorkspaceObserver
{
public:
    virtual ~WorkspaceObserver() = default;
    virtual void on_created(std::shared_ptr<OutputContent> const&, int) = 0;
    virtual void on_removed(std::shared_ptr<OutputContent> const&, int) = 0;
    virtual void on_focused(std::shared_ptr<OutputContent> const& previous, int, std::shared_ptr<OutputContent> const& current, int) = 0;

    int get_id() const;
protected:
    WorkspaceObserver();

private:
    int id;
};

class WorkspaceObserverRegistrar
{
public:
    WorkspaceObserverRegistrar() = default;
    void register_interest(std::weak_ptr<WorkspaceObserver>);
    void unregister_interest(WorkspaceObserver&);
    void advise_created(std::shared_ptr<OutputContent> const&, int);
    void advise_removed(std::shared_ptr<OutputContent> const&, int);
    void advise_focused(std::shared_ptr<OutputContent> const& previous, int, std::shared_ptr<OutputContent> const& current, int);

private:
    std::vector<std::weak_ptr<WorkspaceObserver>> observers;
};

} // miracle

#endif //MIRACLEWM_WORKSPACE_OBSERVER_H
