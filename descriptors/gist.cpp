/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include <algorithm>

#define _USE_MATH_DEFINES		// Visual Studio compiler needs this to find M_PI
#include <cmath>

#include <QImage>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "gist.hpp"
#include "gist_helper.hpp"

namespace imdb {

gist_generator::gist_generator(const ptree& params)
 : Generator(params,
     PropertyWriters()
//     .add<vec_f32_t>("features_mean")
//     .add<vec_f32_t>("features_variance")
     .add<vec_f32_t>("features")
   )

 , _padding       (parse<size_t>     (_parameters, "generator.padding"       , 64             )) // padding (adds to width and height)
 , _realwidth     (parse<size_t>     (_parameters, "generator.width"         , 256            )) // image width
 , _realheight    (parse<size_t>     (_parameters, "generator.height"        , 256            )) // image height
 , _num_x_tiles   (parse<size_t>     (_parameters, "generator.num_x_tiles"   , 4              )) // number of tiles in x-direction
 , _num_y_tiles   (parse<size_t>     (_parameters, "generator.num_y_tiles"   , 4              )) // number of tiles in y-direction
 , _num_freqs     (parse<size_t>     (_parameters, "generator.num_freqs"     , 4              )) // number of different frequencies
 , _num_orients   (parse<size_t>     (_parameters, "generator.num_orients"   , 6              )) // number of orientations (distributed over the half-circle)
 , _max_peak_freq (parse<double>     (_parameters, "generator.max_peak_freq" , 0.3            )) // highest frequency
 , _delta_freq_oct(parse<double>     (_parameters, "generator.delta_freq_oct", 0.88752527     )) // frequency step size in octaves
 , _bandwidth_oct (parse<double>     (_parameters, "generator.bandwidth_oct" , _delta_freq_oct)) // bandwidth in octaves
 , _angle_factor  (parse<double>     (_parameters, "generator.angle_factor"  , 1.0            )) // circular width factor
 , _polar         (parse<bool>       (_parameters, "generator.polar"         , true           )) // use polar gabor filter construction
 , _prefilter_str (parse<string>     (_parameters, "generator.prefilter"     , "torralba"     )) // use prefilter (none, torralba)

 , _width(_realwidth + _padding)
 , _height(_realheight + _padding)
{
    if (_prefilter_str == "torralba") _prefilter_ocv = torralba_prefilter(_width, _height, 4.0 * _width / _realwidth);

    init_filter();
}

void gist_generator::compute(anymap_t& data) const
{

    // ------------------------------------------------------------------------
    // Required input:
    //
    // this generator expects the image to be a CV_8UC3 with BGR channel order.
    // ------------------------------------------------------------------------

    cv::Mat imgColor = get<mat_8uc3_t>(data, "image");

    cv::Mat image;
    cv::cvtColor(imgColor, image, CV_BGR2GRAY);

    // uniformly scale the image such that it has no side that is larger than the filter's size
    double scaling_factor = (image.size().width > image.size().height)
                          ? static_cast<double>(_realwidth) / image.size().width
                          : static_cast<double>(_realheight) / image.size().height;


    // need to use INTER_AREA for downscaling as only this performs correct antialiasing
    cv::Mat scaled;
    cv::resize(image, scaled, cv::Size(), scaling_factor, scaling_factor, cv::INTER_AREA);

    cv::Mat_<unsigned char> padded(_height, _width);
    symmetric_pad(cv::Mat_<unsigned char>(scaled), padded);

    // if exists, apply prefilter on image buffer
    // for example the torralba prefilter
    if (_prefilter_ocv) _prefilter_ocv(padded);

    cv::Mat_<complex_t> src(_height, _width);
    cv::MatConstIterator_<unsigned char> sit = padded.begin();
    cv::MatIterator_<complex_t> dit = src.begin();
    while (sit != padded.end())
    {
        *dit++ = (float_t) *sit++ * (1.0/255.0);
    }

    // transform image into fourier space
    cv::Mat_<complex_t> fts;
    cv::dft(src, fts);

//    vec_f32_t means;
//    vec_f32_t vars;
    vec_f32_t means_vars;

    // shouldn't we better use scaled.width and scaled.height?
    int tilewidth = scaling_factor * image.size().width / _num_x_tiles;
    int tileheight = scaling_factor * image.size().height / _num_y_tiles;

    // convolve image with each filter in fourier space
    for (size_t i = 0; i < _filters.size(); i++)
    {
        // multiply sprectrums == convolve
        cv::Mat_<complex_t> ftd;
        cv::mulSpectrums(fts, _filters[i], ftd, 0);

        // transform back to image space
        cv::Mat_<complex_t> dst;
        cv::idft(ftd, dst, cv::DFT_SCALE);

        // compute the response magnitude
        cv::Mat_<float_t> mag(_height, _width);
        for (int r = 0; r < dst.rows; r++)
        for (int c = 0; c < dst.cols; c++)
        {
            float_t real = dst(r, c).real();
            float_t imag = dst(r, c).imag();
            mag(r, c) = std::sqrt(real*real + imag*imag);
        }

        // get mean and standard deviation of tile contents
        for (size_t y = 0; y < _num_y_tiles; y++)
        for (size_t x = 0; x < _num_x_tiles; x++)
        {
            cv::Mat tile = mag(cv::Rect(x * tilewidth, y * tileheight, tilewidth, tileheight));
            cv::Scalar m, d;
            cv::meanStdDev(tile, m, d);

//            means.push_back(m[0]);

            // TODO: is it better to use the standard deviation or the variance?
            // (w.r.t. optional normalization or distance measures in general)
//            vars.push_back(d[0]*d[0]);

            means_vars.push_back(m[0]);
            means_vars.push_back(d[0]*d[0]);
        }
    }


//    data["features_mean"] = means;
//    data["features_variance"] = vars;
    data["features"] = means_vars;
}

void gist_generator::init_filter()
{
    const double delta_freq = std::pow(2.0, _delta_freq_oct);
    const double bandwidth = std::pow(2.0, _bandwidth_oct);
    const double delta_omega = (M_PI / static_cast<double>(_num_orients));
    const double max_extend = std::max(_width, _height);
    const double pad_max_peak_freq = max_extend * _max_peak_freq / (max_extend + static_cast<double>(_padding));

    // compute gabor filter in regular formation
    for (size_t i = 0; i < _num_freqs; i++)
    {
        for (size_t k = 0; k < _num_orients; k++)
        {
            // compute params
            const double curPeak = pad_max_peak_freq / std::pow(delta_freq, static_cast<double>(i));
            const double curOmega = k * delta_omega;

            // generate filter
            cv::Mat_<complex_t> filter(_width, _height);
            if (_polar)
            {
                generate_polargabor_filter(filter, curPeak, bandwidth, curOmega, delta_omega * _angle_factor);
            }
            else
            {
                generate_gabor_filter(filter, curPeak, bandwidth, curOmega, delta_omega * _angle_factor);
            }

            // kill dc
            filter(0, 0) = 0;

            // add
            _filters.push_back(filter);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////

bool gist_new_registered = Generator::register_generator<gist_generator>("gist");

} // namespace imdb
