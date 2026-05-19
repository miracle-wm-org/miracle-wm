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

#ifndef MIRACLE_WM_MOCK_SHELL_APPLICATION_SPAWNER_H
#define MIRACLE_WM_MOCK_SHELL_APPLICATION_SPAWNER_H

#include "shell_application_spawner.h"
#include <gmock/gmock.h>

namespace miracle::test
{
class MockShellApplication : public ShellApplication
{
public:
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(miral::Application, application, (), (override));
};

class MockShellApplicationSpawner : public ShellApplicationSpawner
{
public:
    MOCK_METHOD(std::unique_ptr<ShellApplication>, spawn, (ShellApplicationRole role, std::shared_ptr<ShellApplicationDelegate> const& delegate), (override));
};
}

#endif // MIRACLE_WM_MOCK_SHELL_APPLICATION_SPAWNER_H
