/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include "image_sampler.hpp"

#include <ctime>
#include <algorithm>

#include <boost/random.hpp>

namespace imdb {


void grid_sampler::setParameters(ptree &params)
{
    _numSamples = parse<uint>(params, "num_samples", 625);
}

// the resulting samples contain (x,y) coordinates within the sampling area
void grid_sampler::sample(vec_vec_f32_t& samples, const cv::Mat& image) const
{
    cv::Rect samplingArea(0, 0, image.size().width, image.size().height);


    uint numSamples1D = std::ceil(std::sqrt(static_cast<float>(_numSamples)));
    float stepX = samplingArea.width / static_cast<float>(numSamples1D+1);
    float stepY = samplingArea.height / static_cast<float>(numSamples1D+1);

    for (uint x = 1; x <= numSamples1D; x++) {
        uint posX = x*stepX;
        for (uint y = 1; y <= numSamples1D; y++) {
            uint posY = y*stepY;
            vec_f32_t p(2);
            p[0] = posX;
            p[1] = posY;
            samples.push_back(p);
        }
    }
}

// -----------------------------------------------------------------------------------------------------------------------

void random_area_sampler::setParameters(ptree &params)
{
    _numSamples = parse<uint>(params, "num_samples", 500);
}

void random_area_sampler::sample(vec_vec_f32_t& samples, const cv::Mat& image) const
{
    // prevent logical error
    assert(samples.size() == 0);

    cv::Rect samplingArea(0, 0, image.size().width, image.size().height);

    int w = samplingArea.width;
    int h = samplingArea.height;

    // TODO: move rng and seed to constructor???
    boost::mt19937 rng;
    rng.seed(static_cast<unsigned int>(std::time(0)));


    boost::variate_generator<boost::mt19937&, boost::uniform_int<> > x_generator(rng, boost::uniform_int<>(0,w-1));
    boost::variate_generator<boost::mt19937&, boost::uniform_int<> > y_generator(rng, boost::uniform_int<>(0,h-1));

    vec_f32_t p(2);
    for (uint i = 0; i < _numSamples; i++)
    {
        p[0] = x_generator();
        p[1] = y_generator();
        samples.push_back(p);
    }
}

bool gridsampler_registered = ImageSampler::register_sampler<grid_sampler>("grid");
bool randomsampler_registered = ImageSampler::register_sampler<random_area_sampler>("random_area");

} // end namespace
