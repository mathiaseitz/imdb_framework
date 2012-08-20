/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef DESCRIPTORS__GIST_HPP
#define DESCRIPTORS__GIST_HPP

#include <string>
#include <complex>

#include <boost/function.hpp>

#include "../util/types.hpp"
#include "../descriptors/generator.hpp"

namespace imdb
{

/**
 * @ingroup generators
 * @brief The gist_generator class
 */
class gist_generator : public Generator
{
    typedef float                 float_t;
    typedef std::complex<float_t> complex_t;

    public:

    gist_generator(const ptree& params);

    void compute(anymap_t& data) const;

    private:

    void init_filter();

    const size_t _padding;

    const size_t _realwidth;
    const size_t _realheight;

    const size_t _num_x_tiles;
    const size_t _num_y_tiles;

    const size_t _num_freqs;
    const size_t _num_orients;

    const double _max_peak_freq;
    const double _delta_freq_oct;
    const double _bandwidth_oct;
    const double _angle_factor;

    const bool   _polar;

    const std::string _prefilter_str;

    const size_t _width;
    const size_t _height;

    boost::function<void (cv::Mat&)> _prefilter_ocv;
    std::vector<cv::Mat_<complex_t> > _filters;
};

} // namespace imdb

#endif // DESCRIPTORS__GIST_HPP
