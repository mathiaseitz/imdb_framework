/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include <algorithm>

#include "linear_search_manager.hpp"
#include "linear_search.hpp"
#include "../io/property_reader.hpp"

namespace imdb
{

LinearSearchManager::LinearSearchManager(const ptree& parameters)
{
    string filename = parameters.get<string>("descriptor_file");
    string distfn_str = parameters.get<string>("distfn");

    // try to create distance function object
    _distfn = distance_functions<vec_f32_t>().make(distfn_str);
    if (!_distfn) throw std::runtime_error("unknown distance function: " + distfn_str);

    // try to load features
    try {
        read_property(_features, filename);
    } catch(std::exception& e) {
        std::cerr << "LinearSearchManager: exception occured when trying to load features file: " + filename << std::endl;
        std::cerr << e.what() << std::endl;
    }
}


void LinearSearchManager::query(const vec_f32_t& descr, size_t num_results, vector<dist_idx_t>& result) const
{
    size_t max_num_results = std::min(num_results, _features.size());
    linear_search(descr, _features, result, max_num_results, _distfn);
}

} // namespace imdb
