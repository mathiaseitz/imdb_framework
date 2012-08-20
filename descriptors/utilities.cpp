/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include "utilities.hpp"

#include <numeric>
#include <opencv2/imgproc/imgproc.hpp>

namespace imdb {

void filterEmptyFeatures(const vec_vec_f32_t& features, const vec_vec_f32_t& keypoints, const vector<index_t>& emptyFeatures, vec_vec_f32_t& featuresFiltered, vec_vec_f32_t& keypointsFiltered)
{
    assert(features.size() == keypoints.size());
    assert(features.size() == emptyFeatures.size());

    for (size_t i = 0; i < emptyFeatures.size(); i++) {

        if (!emptyFeatures[i]) {
            featuresFiltered.push_back(features[i]);
            keypointsFiltered.push_back(keypoints[i]);
        }
    }
}

void normalizePositions(const vec_vec_f32_t& keypoints, const cv::Size& imageSize, vec_vec_f32_t& keypointsNormalized) {

    vec_f32_t p(2);
    for (size_t i = 0; i < keypoints.size(); i++) {
        p[0] = keypoints[i][0] / imageSize.width;
        p[1] = keypoints[i][1] / imageSize.height;
        keypointsNormalized.push_back(p);
    }
}


double scaleToSideLength(const cv::Mat& image, int maxSideLength, cv::Mat& scaled)
{
    double scaling_factor = (image.size().width > image.size().height)
            ? static_cast<double>(maxSideLength) / image.size().width
            : static_cast<double>(maxSideLength) / image.size().height;

    // as of OpenCV version 2.2 we need to user INTER_AREA for downscaling as all
    // other options lead to severe aliasing!
    cv::resize(image, scaled, cv::Size(0, 0), scaling_factor, scaling_factor, cv::INTER_AREA);
    return scaling_factor;
}

}

