/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef BOF_H
#define BOF_H

#include "inverted_index.hpp"
#include "../util/types.hpp"
#include "../io/filelist.hpp"

namespace imdb {


    /**
     * @ingroup search
     * @brief Standard bag-of features (Bof) search as used for searching images.
     *
     * Encapsulates loading of the underlying InvertedIndex and instantiates the
     * tf_idf function to be used for weighting of the query histogram.
     */
    class BofSearchManager
    {

    public:


        /// Datatype of descriptor (histogram of visual words) used by this class.
        typedef vec_f32_t descr_t;

        /**
         * @brief Constructs the BofSearchManager, loads all required datastructures such that a query() can be performed
         * @param parameters A boost::property_tree holding three key/value pairs:
         * - "index_file": path to the filename of the InvertedIndex to load, e.g. "/tmp/index.data"
         * - "tf": name of the tf_function used to weigh the query histogram, e.g. "video_google", you probably
         * want to use the same function you used when constructing the InvertedIndex
         * - "idf": name of the idf_function used to weigh the query histogram, e.g. "video_google", you probably
         * want to use the same function you used when constructing the InvertedIndex
         */
        BofSearchManager(const ptree& parameters);

        /**
         * @brief Perform a query for the most similar 'documents' on the inverted index.
         * @param histvw Histogram of visual words encoding the query 'document' (image)
         * @param num_results Desired number of results
         * @param results A vector of dist_idx_t that holds the result indices in descending order of
         * similarity (i.e. best matches are first in the vector). Any potentially existing contents
         * of this vector are cleared before the new results are added.
         */
        void query(const vec_f32_t& histvw, size_t num_results, vector<dist_idx_t>& results) const;

        const InvertedIndex& index() const {return _index;}

    private:

        InvertedIndex                   _index;

        // tf*idf weighting functions
        shared_ptr<tf_function>  _tf;
        shared_ptr<idf_function> _idf;
    };


} // end namespace

#endif // BOF_H
