/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <algorithm>
#include <utility>
#include <set>

#include "../util/types.hpp"


/**
 * @ingroup search
 * @brief Linear search implementation given a distance metric.
 *
 * Note that the distance function must actually measure *distance*, i.e. better matches
 * produce smaller values. The result will be sorted ascending, i.e. best matches come first.
 *
 * Typical example for template paramters:
 * - storage_t: std::vector<std::vector<float>>
 * - result_t:  std::vector<dist_idx_t>>
 * - distfn_t: l2dist
 *
 * The result is always a container class containing at each index
 * a std::pair(distance, index), where index points into the features
 * collection.
 */
template <class storage_t, class result_t, class distfn_t>
void linear_search(const typename storage_t::value_type& query_feature, const storage_t& features, result_t& result, size_t num_results, const distfn_t& distfn)
{

    using namespace std;

    // if result is not empty, then its content get involved by the search algorithm
    // i.e. result will be updated
    if (result.size() > 0) make_heap(result.begin(), result.end());

    for (size_t i = 0; i < features.size(); i++)
    {
        typename distfn_t::result_type dist = distfn(query_feature, features[i]);

        // if the number of wanted elements is not reached, every element is taken
        // else if the current element has smaller distance than the element with
        // the greatest distance in the queue, then it is inserted into the queue
        if (result.size() < num_results)
        {
            result.push_back(make_pair(dist, i));
            push_heap(result.begin(), result.end());
        }
        else if (!result.empty() && result.front().first > dist)
        {
            pop_heap(result.begin(), result.end());
            result.back() = make_pair(dist, i);
            push_heap(result.begin(), result.end());
        }
    }

    // make an ascending sorted list out of the heap
    sort_heap(result.begin(), result.end());
}

#endif // SEARCH_HPP
