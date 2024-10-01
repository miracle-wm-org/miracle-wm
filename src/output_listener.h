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


#ifndef MIRACLE_WM_OUTPUT_LISTENER_H
#define MIRACLE_WM_OUTPUT_LISTENER_H

#include <vector>

namespace miral
{
class Output;
}

namespace miracle
{

class OutputListener
{
public:
    virtual void output_created(miral::Output const&) = 0;
    virtual void output_updated(miral::Output const& updated, miral::Output const& original) = 0;
    virtual void output_deleted(miral::Output const&) = 0;
};

class OutputListenerMultiplexer : public OutputListener
{
public:
    void output_created(miral::Output const&) override;
    void output_updated(miral::Output const& updated, miral::Output const& original) override;
    void output_deleted(miral::Output const&) override;

    void register_listener(OutputListener*);
    void unregister_listener(OutputListener*);

private:
    std::vector<OutputListener*> listeners;
};

}

#endif //MIRACLE_WM_OUTPUT_LISTENER_H
