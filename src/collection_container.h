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

#ifndef MIRACLE_COLLECTION_CONTAINER_H
#define MIRACLE_COLLECTION_CONTAINER_H

#include "container.h"

namespace miracle
{
/// The collection container is a [Container] that contains multiple containers.
///
/// The [ParentContainer] and [WorkspaceInterface] are examples of collection containers.
///
/// By default, a collection container will provide iterators over its children.
class CollectionContainer : public Container
{
public:
    ~CollectionContainer() override = default;

    /// Return the list of children associated with this container.
    ///
    /// \returns reference to list of children
    virtual std::vector<std::shared_ptr<Container>> const& children() const = 0;

    /// Add a child to the container.
    ///
    /// \param container
    /// \param index
    /// \returns the new container
    virtual void add_child(std::shared_ptr<Container> const& container, size_t index) = 0;

    /// Remove a child container.
    ///
    /// \param container
    virtual void remove_child(std::shared_ptr<Container> const& container) = 0;

    /// Iterate over the windows in this collection.
    void for_each_window(std::function<void(std::shared_ptr<WindowContainer>)> const&) const;

    /// Returns the number of children.
    ///
    /// \returns number of children
    size_t num_children() const;

    /// Returns the container at \p i.
    ///
    /// \param i the index
    /// \returns container, or nullptr if none exist
    std::shared_ptr<Container> at(size_t i) const;
};
}

#endif