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

#include "animator_loop.h"
#include "animator.h"

#include <iostream>
#include <mir/server_action_queue.h>

using namespace miracle;
using namespace std::chrono_literals;

ThreadedAnimatorLoop::ThreadedAnimatorLoop(std::shared_ptr<Animator> const& animator) :
    animator { animator }
{
}

ThreadedAnimatorLoop::~ThreadedAnimatorLoop()
{
    stop();
}

void ThreadedAnimatorLoop::start()
{
    run_thread = std::thread([this]()
    { run(); });
}

void ThreadedAnimatorLoop::stop()
{
    if (!running)
        return;

    running = false;
    animator->get_cv().notify_one();
    run_thread.join();
}

void ThreadedAnimatorLoop::run()
{
    using clock = std::chrono::steady_clock;
    constexpr float target_fps = 120.f;
    constexpr float target_ms = 1000.f / target_fps;
    constexpr float dt = 1.f / target_fps;
    constexpr std::chrono::duration<float, std::milli> frame_duration(target_ms);
    running = true;

    while (running)
    {
        auto frame_start = clock::now();
        {
            std::unique_lock lock(animator->get_lock());
            if (!animator->has_animations())
            {
                animator->get_cv().wait(lock);
                frame_start = clock::now();

                if (!running)
                    return;
            }
        }

        animator->tick(dt);

        auto frame_end = clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start);

        if (elapsed < frame_duration)
        {
            std::this_thread::sleep_for(frame_duration - elapsed);
        }
    }
}

ServerActionQueueAnimatorLoop::ServerActionQueueAnimatorLoop(
    std::shared_ptr<Animator> const& animator,
    std::shared_ptr<mir::ServerActionQueue> const& server_action_queue) :
    animator { animator },
    server_action_queue { server_action_queue }
{
}

ServerActionQueueAnimatorLoop::~ServerActionQueueAnimatorLoop()
{
    ServerActionQueueAnimatorLoop::stop();
}

void ServerActionQueueAnimatorLoop::start()
{
    running = true;
    using clock = std::chrono::steady_clock;
    last_time = clock::now();
    run();
}

void ServerActionQueueAnimatorLoop::stop()
{
    if (!running)
        return;

    running = false;
}

void ServerActionQueueAnimatorLoop::run()
{
    if (!running)
        return;

    using clock = std::chrono::steady_clock;
    delta_time = clock::now() - last_time;
    last_time = clock::now();
    animator->tick(delta_time.count());

    server_action_queue->enqueue(this, [this]()
    {
        run();
    });
}