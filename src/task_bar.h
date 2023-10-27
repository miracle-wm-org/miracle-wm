/*
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRCOMPOSITOR_TASK_BAR_H
#define MIRCOMPOSITOR_TASK_BAR_H

#include "miracle_internal_client.h"
#include <wayland-client.h>
#include <mir/fd.h>


namespace miracle
{

struct TaskBarOptions
{

};

class TaskBarClient;

/// Creates a task bar that can be on any part of the screen
class TaskBar: public MiracleInternalClient
{
public:
    TaskBar();
    ~TaskBar() = default;
    void operator()(wl_display* display) override;
    void operator()(std::weak_ptr<mir::scene::Session> const& session);
    void stop() override;

private:
    TaskBarClient* client;
};

} // miracle

#endif //MIRCOMPOSITOR_TASK_BAR_H
