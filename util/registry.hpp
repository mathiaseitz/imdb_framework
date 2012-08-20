/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef REGISTRY_HPP
#define REGISTRY_HPP

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include "types.hpp"

namespace imdb {

/**
 * @ingroup util
 * @brief General singleton registry implementation that can hold
 * elements of arbitrary type accessible by a string name
 */
class Registry
{
    public:

    template <class T>
    T& get(const std::string& name)
    {
        // in case the entry does not exist yet, create it. Very often, our entries are
        // themselves containers (typically maps) and thus this avoids that we first have
        // to manually add an empty container to the registry before we can use it.
        if (!_entries.count(name)) _entries.insert(std::make_pair(name, boost::make_shared<T>()));
        return *boost::any_cast<boost::shared_ptr<T> >(_entries.find(name)->second);
    }

    private:

    anymap_t _entries;

    // this gives the one and only instance of Registry
    static Registry& instance() { static Registry singleton; return singleton; }
    friend Registry& registry();

    // protect construction from outside
    Registry() {}
    Registry(const Registry&) {}
    ~Registry() {}
};

inline Registry& registry() { return Registry::instance(); }

} // namespace imdb

#endif // REGISTRY_HPP
