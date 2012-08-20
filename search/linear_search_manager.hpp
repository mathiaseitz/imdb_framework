/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef LINEAR_SEARCH_HPP
#define LINEAR_SEARCH_HPP

#include "../util/types.hpp"
#include "distance.hpp"

namespace imdb
{

/**
 * @ingroup search
 * @brief Linear search over data in a property file using a given distance metric.
 *
 * Encapsulates loading of the required datastructures and distance metric
 * such that a search can be performed once the instance has been constructed.
 * Note that this class loads \b all features into main memory, make sure
 * that you have enough memory to do so.
 */
class LinearSearchManager
{
    public:

    typedef vec_f32_t descr_t;

    /**
     * @brief Constructs the LinearSearchManager, loads all required datastructures such that a query() can be performed.
     * @param parameters boost::property_tree that must contain the following key/value pairs:
     * - "descriptor_file": filename of the features file over which you want to perform linear search, e.g. "/tmp/tinyimage.features". The
     * features file must have been created using a PropertyWriterT with T=vec_f32_t.
     * - "distfn": distance function, can be "l1norm", "l2norm", "l2norm_squared" or any other sensible distance metric available
     * via distance_functions<T>.make()
     */
    LinearSearchManager(const ptree& parameters);

    /**
     * @brief Perform a linear search with data as the query feature.
     *
     * The input feature is compared to all features from the property file specified in the constructor using
     * the provided distance metric.
     *
     * @param data Query data, must have the same size as the features in the property file that has been loaded in the constructor
     * @param num_results Number of results to be returned
     * @param result A vector of imdb::dist_idx_t that holds the result indices in descending order of
     * similarity (i.e. best matches are first in the vector). Any potentially existing contents
     * of this vector are cleared before the new results are added. The result indices point at positions
     * in the property file that has been searched.
     */
    void query(const vec_f32_t& data, size_t num_results, vector<dist_idx_t>& result) const;
    const vec_vec_f32_t& features() {return _features;}

    private:

    vec_vec_f32_t _features;
    distance_functions<vec_f32_t>::distfn_t _distfn;
};

} // namespace imdb

#endif // LINEAR_SEARCH_HPP
