/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef QUANTIZER_HPP
#define QUANTIZER_HPP

#include "types.hpp"

namespace imdb {

/**
 * @addtogroup util
 * @{
 */


/**
 * @brief Functor performing hard quantization of a sample against a given codebook of samples
 */
template <typename sample_t, typename dist_fn>
struct quantize_hard {


    /**
     * @brief Performs hard quantization of \p sample against the passed \p vocabulary.
     *
     * Computes the index of the sample in the vocabulary that has the smallest
     * distance (under the distance functor passed in via the second template parameter) to
     * the sample to be quantized. Note that we actually return a vector that contains a single 1
     * at the corresponding index. This is not really optimal performance-wise, but makes
     * the 'interface' similar to that of quantize_fuzzy such that both functors can be easily
     * exchanged for each other.
     *
     * @param sample Sample to be quantized
     * @param vocabulary Vocabulary
     * @param quantized_sample
     */
    void operator()(const sample_t& sample, const vector<sample_t>& vocabulary, vec_f32_t& quantized_sample)
    {

        // this should be very efficient in case the
        // result vector already has the correct size
        // TODO: we need to make sure that all entries
        // are zero!
        quantized_sample.resize(vocabulary.size());

        size_t closest = 0;
        float minDistance = std::numeric_limits<float>::max();

        dist_fn dist;

        for (size_t i = 0; i < vocabulary.size(); i++)
        {
            float distance = dist(sample, vocabulary[i]);
            if (distance <= minDistance)
            {
                closest = i;
                minDistance = distance;
            }
        }
        quantized_sample[closest] = 1;
    }
};


/**
 * @brief Functor performing soft quantization of a sample against a given codebook of samples
 */
template <typename sample_t, typename dist_fn>
struct quantize_fuzzy
{

    /**
     * @brief quantize_fuzzy
     * @param Standard deviation of the Gaussian used for weighting a sample
     */
    quantize_fuzzy(float sigma) : _sigma(sigma)
    {
        assert(_sigma > 0);
    }

    void operator()(const sample_t& sample, const vector<sample_t>& vocabulary, vec_f32_t& quantized_sample)
    {
        // this should be very efficient in case the
        // result vector already has the correct size
        quantized_sample.resize(vocabulary.size());

        dist_fn dist;

        float sigma2 = 2*_sigma*_sigma;
        float sum = 0;

        for (size_t i = 0; i < vocabulary.size(); i++)
        {
            float d = dist(sample, vocabulary[i]);
            float e = exp(-d*d / sigma2);
            sum += e;
            quantized_sample[i] = e;
        }

        // Normalize such that sum(result) = 1 (L1 norm)
        // The reason is that each local feature contributes the same amount of energy (=1) to the
        // resulting histogram, If we wouldn't normalize, some features (that are close to several
        // entries in the vocabulary) would contribute more energy than others.
        // This is exactly the approach taken by Chatterfield et al. -- The devil is in the details
        for (size_t i = 0; i < quantized_sample.size(); i++)
            quantized_sample[i] /= sum;
    }


    float _sigma;
};



/**
 * @brief 'Base-class' for a quantization function.
 *
 * Instead of creating a common base class we use a boost::function
 * that achieves the same effect, i.e. both quantize_hard
 * and quantize_fuzzy can be assigned to this function type
 */
typedef boost::function<void (const vec_f32_t&, const vec_vec_f32_t&, vec_f32_t&)> quantize_fn;




/**
 * @brief Convenience function that quantizes a vector of samples in parallel
 * @param samples Vector of samples to be quantized, each sample is of vector<float>
 * @param vocabulary Vocabulary to quantize the samples against
 * @param quantized_samples A vector of the same size as the \p samples vector with each
 * entry being a vector the size of the \p vocabulary.
 * @param quantizer quantization function to be used
 */
void quantize_samples_parallel(const vec_vec_f32_t& samples, const vec_vec_f32_t& vocabulary, vec_vec_f32_t& quantized_samples, quantize_fn& quantizer);



// Given a list of quantized samples and corresponding coordinates
// compute the (spatialized) histogram of visual words out of that.
// normalize=true normalizes the resulting histogram by the number
// of samples, this is typically only used in case of a fuzzy histogram!
//
// Note a):
// If you don't want to add any spatial information only pass in the
// first three parameters, this gives a standard BoF histogram
//
// Note b):
// we assume that the positions lie in [0,1]x[0,1]
void build_histvw(const vec_vec_f32_t& quantized_features, size_t vocabulary_size, vec_f32_t& histvw, bool normalize, const vec_vec_f32_t& positions = vec_vec_f32_t(), int res = 1);



/** @} */

} // end namespace

#endif // QUANTIZER_HPP
