/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include "../util/types.hpp"

namespace imdb {

// Removes all empty features, i.e. those that only contains zeros
void filterEmptyFeatures(const vec_vec_f32_t& features, const vec_vec_f32_t& keypoints, const vector<index_t>& emptyFeatures, vec_vec_f32_t& featuresFiltered, vec_vec_f32_t& keypointsFiltered);


// Normalizes keypoint coordinates into range [0, 1] x [0, 1] to have them stored independently of image size
void normalizePositions(const vec_vec_f32_t &keypoints, const cv::Size& imageSize, vec_vec_f32_t& keypointsNormalized);

// Uniformly scale the image such that the longer of its sides is
// scaled to exactly maxSideLength, the other one <= maxSideLength
double scaleToSideLength(const cv::Mat& image, int maxSideLength, cv::Mat& scaled);

} // namespace imdb

#endif // UTILITIES_HPP
