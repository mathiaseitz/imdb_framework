/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include "quantizer.hpp"

namespace imdb {

void quantize_samples_parallel(const vec_vec_f32_t& samples, const vec_vec_f32_t& vocabulary, vec_vec_f32_t& quantized_samples, quantize_fn& quantizer)
{
    quantized_samples.resize(samples.size());

    // for each word compute distances to each entry in the vocabulary ...
    #pragma omp parallel for
    for (size_t i = 0; i < samples.size(); i++)
    {
        quantizer(samples[i], vocabulary, quantized_samples[i]);
    }
}

void build_histvw(const vec_vec_f32_t& quantized_features, size_t vocabularySize, vec_f32_t& histvw, bool normalize, const vec_vec_f32_t& positions, int res)
{

    // sanity checking of the input arguments
    assert(res > 0);
    assert(vocabularySize > 0);
    //assert(quantized_features.size() > 0);
    if (res > 1) assert(positions.size() == quantized_features.size());


    //size_t vocabularySize = quantized_features[0].size();

    // length of the vector is number of cells x histogram length
    // i.e. it actually stores one histogram per cell
    histvw.resize(res*res*vocabularySize, 0);


    // Note that if quantize_features.size() == 0, the whole for
    // loop does not get executed and we end up with an all-zero
    // histogram of visual words, as expected
    for (size_t i = 0; i < quantized_features.size(); i++)
    {
        // we assume that each feature has the same length as we
        // expect them all to have been quantized against the same vocabulary
        assert(quantized_features[i].size() == vocabularySize);


        // in the case of res = 1, offset will be zero and
        // we only have a single histogram (no pyramid) and
        // thus the offset into this overall histogram will be zero
        int offset = 0;

        // ----------------------------------------------------------------
        // Special path for building a spatial pyramid
        //
        // If the user has chosen res = 1 we do not care about the content
        // of the positions vector as they are only accessed for res > 1
        if (res > 1)
        {
            int x = static_cast<int>(positions[i][0] * res);
            int y = static_cast<int>(positions[i][1] * res);
            if (x == res) x--; // handles the case positions[i][0] = 1.0
            if (y == res) y--; // handles the case positions[i][1] = 1.0

            // generate a linear index from 2D (x,y) index
            int idx = y*res + x;
            assert(idx >= 0 && idx < res*res);

            // identify the spatial histogram we want to add to
            offset = vocabularySize*idx;
        }
        // -----------------------------------------------------------------


        // Build up histogram by adding the quantized feature to the
        // intermediate histogram. Offset defines the spatial bin we add into
        for (size_t j = 0; j < vocabularySize; j++) {
            histvw[offset+j] += quantized_features[i][j];
        }
    }


    // for the soft features we should normalize by the number of samples
    // but we do not really want that for the hard quantized features...
    // follow the approach by Chatterfield et al.
    // The second check is to avoid division by zero. In case an empty quantized_features
    // vector is passed in, the result will be an all zero histogram
    if (normalize && quantized_features.size() > 0)
    {
        size_t numSamples = quantized_features.size();
        for (size_t i = 0; i < histvw.size(); i++)
            histvw[i] /= numSamples;
    }
}


} // end namespace


