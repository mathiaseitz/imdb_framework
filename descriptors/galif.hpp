/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef GALIF_HPP
#define GALIF_HPP

#include <opencv2/core/core.hpp>

#include "../util/types.hpp"
#include "generator.hpp"
#include "image_sampler.hpp"

namespace imdb
{

/**
 * @ingroup generators
 * @brief The galif_generator class
 */
class galif_generator : public Generator
{
    public:

    galif_generator(const ptree& params);

    void compute(anymap_t& data) const;

    double scale(const cv::Mat& image, cv::Mat& scaled) const;

    void detect(const cv::Mat& image, vec_vec_f32_t& keypoints) const;

    void extract(const cv::Mat& image, const vec_vec_f32_t &keypoints, vec_vec_f32_t& features, vector<index_t> &emptyFeatures) const;

    private:

    void assertImageSize(const cv::Mat& image) const;

    const uint         _width;
    const uint         _numOrients;
    const double       _peakFrequency;
    const double       _line_width;
    const double       _lambda;
    const double       _featureSize;
    const uint         _tiles;
    const bool         _smoothHist;
    const string       _normalizeHist;
    const string       _samplerName;

    cv::Size _filterSize;
    vector<cv::Mat_<std::complex<double> > > _gaborFilter;
    shared_ptr<ImageSampler> _sampler;

};


} // namespace imdb

#endif // GALIF_HPP
