/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include "bof_search_manager.hpp"

#include "../util/types.hpp"

namespace imdb {

BofSearchManager::BofSearchManager(const ptree& parameters)
{
    string index_file = parameters.get<string>("index_file");

    // get the tf/idf weighting functions from the config,
    // use the most basic one (constant) as default if none supplied

    // TODO: better throw an exception when the user passes in an
    // unsupported/misspelled function name
    string tf = parameters.get<string>("tf", "constant");
    string idf = parameters.get<string>("idf", "constant");

    //std::cout << "BofSearchManager: tf=" << tf << ", idf=" << idf << std::endl;

    _tf  = make_tf(tf);
    _idf = make_idf(idf);

    _index.load(index_file);
}


void BofSearchManager::query(const vec_f32_t& histvw, size_t num_results, vector<dist_idx_t>& results) const
{
    _index.query(histvw, *_tf, *_idf, num_results, results);
}

} // end namespace imdb
