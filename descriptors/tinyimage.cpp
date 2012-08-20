/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "../util/types.hpp"

#include "tinyimage.hpp"

namespace imdb {

tinyimage_generator::tinyimage_generator(const ptree& params)
 : Generator(params, PropertyWriters().add<vec_f32_t>("features")),
 _width (parse<size_t>(_parameters, "generator.width" ,     16)), //width of thumbnail
 _height(parse<size_t>(_parameters, "generator.height",     16)), // height of thumbnail
 _colorspace(parse<string>  (_parameters, "generator.colorspace", "lab"))
{}

void tinyimage_generator::compute(anymap_t& data) const
{

    using namespace std;
    using namespace cv;

    // ------------------------------------------------------------------------
    // Required input:
    //
    // this generator expects the image to be a CV_8UC3 with BGR channel order.
    // ------------------------------------------------------------------------

    Mat img = get<mat_8uc3_t>(data, "image");

    // resize to thumbnail size
    Mat imgScaled;
    resize(img, imgScaled, Size(_width, _height), 0, 0, INTER_AREA);


    imgScaled.convertTo(imgScaled, CV_32FC3, 1.0/255.0);

    vec_f32_t features;
    if (_colorspace == "grey") features.resize(_width * _height);
    else                       features.resize(_width * _height * 3);

    // convert to user-selected colorspace
    cv::Mat imageColorspace;
    if (_colorspace == "lab")       cv::cvtColor(imgScaled, imageColorspace, CV_RGB2Lab);
    else if (_colorspace == "grey") cv::cvtColor(imgScaled, imageColorspace, CV_RGB2GRAY);
    else if (_colorspace == "rgb")  imageColorspace = imgScaled;


    // copy image data to output feature
    if (_colorspace == "grey")
    {
        vec_f32_t::iterator di = features.begin();
        for (cv::MatConstIterator_<float> si = imgScaled.begin<float>(); si != imgScaled.end<float>(); ++si)
        {
            *di = (*si);
            ++di;
        }
    }
    else
    {
        vec_f32_t::iterator di = features.begin();
        for (cv::MatConstIterator_<cv::Vec3f> si = imgScaled.begin<cv::Vec3f>(); si != imgScaled.end<cv::Vec3f>(); ++si)
        {
            *di = (*si)[0]; ++di;
            *di = (*si)[1]; ++di;
            *di = (*si)[2]; ++di;
        }
    }

    data["features"] = features;
}

bool tinyimage_registered = Generator::register_generator<tinyimage_generator>("tinyimage");

} // namespace imdb
