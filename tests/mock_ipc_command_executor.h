/**
Copyright (C) 2024  Matthew Kosarek

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

#ifndef MOCK_IPC_COMMAND_EXECUTOR_H
#define MOCK_IPC_COMMAND_EXECUTOR_H

#include "ipc_command_executor.h"
#include <gmock/gmock.h>

namespace miracle
{
namespace test
{
    class MockIpcCommandExecutor : public AbstractIpcCommandExecutor
    {
    public:
        MOCK_METHOD(std::vector<IpcValidationResult>, process, (IpcParseResult const&), (override));
        MOCK_METHOD(void, apply_startup_commands_to, (std::shared_ptr<Container> const&), (override));
    };
}
}

#endif // MOCK_IPC_COMMAND_EXECUTOR_H
