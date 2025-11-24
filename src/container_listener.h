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

#ifndef MIRACLE_CONTAINER_LISTENER_H
#define MIRACLE_CONTAINER_LISTENER_H

namespace miracle
{
class Container;

class ContainerListener
{
public:
    virtual ~ContainerListener() = default;

    virtual void on_container_fullscreen(Container const&) = 0;
    virtual void on_container_moved(Container const&) = 0;
    virtual void on_container_float(Container const&) = 0;
    virtual void on_container_mark(Container const&) = 0;
};

class NullContainerListener : public ContainerListener
{
    void on_container_fullscreen(Container const&) override { }
    void on_container_moved(Container const&) override { }
    void on_container_float(Container const&) override { }
    void on_container_mark(Container const&) override { }
};

} // miracle

#endif // MIRACLE_CONTAINER_LISTENER_H
