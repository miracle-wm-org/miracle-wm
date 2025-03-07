/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_MAIN_LOOP_H_
#define MIR_TEST_DOUBLES_MOCK_MAIN_LOOP_H_

#include "mir/main_loop.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
    namespace doubles
    {
        class MockMainLoop : public MainLoop
        {
        public:
            ~MockMainLoop() noexcept { }

            void run() override { }
            void stop() override { }
            bool running() const override { return true; }

            MOCK_METHOD2(register_signal_handler,
                void(std::initializer_list<int>,
                    std::function<void(int)> const&));

            MOCK_METHOD2(register_signal_handler_module_ptr,
                void(std::initializer_list<int>,
                    std::function<void(int)> const&));

            MOCK_METHOD3(register_fd_handler,
                void(std::initializer_list<int>, void const*,
                    std::function<void(int)> const&));

            MOCK_METHOD3(register_fd_handler_module_ptr,
                void(std::initializer_list<int>, void const*,
                    std::function<void(int)> const&));

            MOCK_METHOD1(unregister_fd_handler, void(void const*));

            MOCK_METHOD2(enqueue, void(void const*, ServerAction const&));
            MOCK_METHOD(void, enqueue_with_guaranteed_execution, (ServerAction const&), (override));
            MOCK_METHOD1(pause_processing_for, void(void const*));
            MOCK_METHOD1(resume_processing_for, void(void const*));

            MOCK_METHOD1(spawn, void(std::function<void()>&));

            MOCK_METHOD1(create_alarm, std::unique_ptr<time::Alarm>(std::function<void()> const& callback));
            MOCK_METHOD1(create_alarm, std::unique_ptr<time::Alarm>(LockableCallback* callback));

            std::unique_ptr<time::Alarm> create_alarm(std::unique_ptr<LockableCallback> callback) override
            {
                return create_alarm(callback.get());
            }

            void register_signal_handler(
                std::initializer_list<int> signals,
                mir::UniqueModulePtr<std::function<void(int)>> handler) override
            {
                register_signal_handler_module_ptr(signals, *handler);
            }

            void register_fd_handler(
                std::initializer_list<int> fds,
                void const* owner,
                mir::UniqueModulePtr<std::function<void(int)>> handler) override
            {
                register_fd_handler_module_ptr(fds, owner, *handler);
            }

            void spawn(std::function<void()>&& work) override
            {
                std::function<void()> work_copy { std::move(work) };
                spawn(work_copy);
            }
        };

    }
}
}

#endif
