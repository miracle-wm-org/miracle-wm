/**
Copyright (C) 2025  Matthew Kosarek

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <mir/server_action_queue.h>

namespace miracle
{
class PassthroughServerActionQueue : public mir::ServerActionQueue
{
public:
    void enqueue(void const* owner, mir::ServerAction const& action) override
    {
        action();
    }

    void enqueue_with_guaranteed_execution(mir::ServerAction const& action) override
    {
        action();
    }

    void pause_processing_for(void const* owner) override { }
    void resume_processing_for(void const* owner) override { }
};
}
