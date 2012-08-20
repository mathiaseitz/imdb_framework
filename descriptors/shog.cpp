/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include <vector>

// Mathias 31.12.2011
// Need to have this defined on Windows to have the macro
// M_PI available which we use later in this code
#define _USE_MATH_DEFINES

// Mathias 31.12.2011
// Avoids generation of the min/max macros on Windows which
// interfere with our more standard use of std::min/std::max
// template functions
#define NOMINMAX
#include <cmath>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "../util/types.hpp"
#include "shog.hpp"
#include "utilities.hpp"

namespace imdb {

shog_generator::shog_generator(const ptree& params)
    : Generator(params,
                PropertyWriters()
                .add<vec_vec_f32_t>("features")
                .add<vec_vec_f32_t>("positions")
                .add<int32_t>("numfeatures") // number of visual words/image
                )


    // _width determines that maximum desired sidelength of the image, i.e. the
    // incoming image is scaled such that no side is longer than this. Named
    // image_width for compatibility reasons with the sift_sketch generator
    , _width              (parse<uint>  (_parameters, "generator.image_width", 256))
    , _numOrients         (parse<uint>  (_parameters, "generator.num_orients", 4))
    , _featureSize        (parse<double>(_parameters, "generator.feature_size", 0.125))
    , _tiles              (parse<uint>  (_parameters, "generator.tiles", 4))
    , _smoothHist         (parse<bool>  (_parameters, "generator.smooth_hist", true))
    , _samplerName        (parse<string>(_parameters, "generator.sampler.name", "grid"))
    , _sampler            (ImageSampler::create(_samplerName))
{

    _sampler->setParameters(_parameters.get_child("generator.sampler"));

    // TODO: iterate over property tree instead
    std::cout << "shog config:" << std::endl;
    std::cout << " generator.image_width=" << _width << std::endl;
    std::cout << " generator.num_orients=" << _numOrients << std::endl;
    std::cout << " generator.feature_size=" << _featureSize << std::endl;
    std::cout << " generator.tiles=" << _tiles << std::endl;
    std::cout << " generator.smooth_hist=" << _smoothHist << std::endl;
    std::cout << " generator.sampler.name=" << _samplerName << std::endl;
}


void shog_generator::compute(anymap_t& data) const
{
    // --------------------------------------------------------------
    // prerequisites:
    //
    // this generator expects a 3-channel image, with
    // each channel containing exactly the same pixel values
    //
    // the image must have a white background with black sketch lines
    // --------------------------------------------------------------

    using namespace std;
    using namespace cv;

    Mat img = get<mat_8uc3_t>(data, "image");
    Mat imgGray;

    cvtColor(img, imgGray, CV_RGB2GRAY);

    assert(imgGray.type() == CV_8UC1);

    // scale image to desired size
    Mat scaled;
    scale(imgGray, scaled);

    // detect keypoints on the scaled image
    vec_vec_f32_t keypoints;
    detect(scaled, keypoints);

    // extract local features at the given keypoints
    vec_vec_f32_t features;
    vector<index_t> emptyFeatures;
    extract(scaled, keypoints, features, emptyFeatures);
    assert(features.size() == keypoints.size());
    assert(emptyFeatures.size() == keypoints.size());

    // normalize keypoints to range [0,1]x[0,1] so they are
    // independent of image size
    vec_vec_f32_t keypointsNormalized;
    normalizePositions(keypoints, scaled.size(), keypointsNormalized);

    // remove features that are empty, i.e. that contain
    // no sketch stroke within their area
    vec_vec_f32_t featuresFiltered;
    vec_vec_f32_t keypointsNormalizedFiltered;
    filterEmptyFeatures(features, keypointsNormalized, emptyFeatures, featuresFiltered, keypointsNormalizedFiltered);
    assert(featuresFiltered.size() == keypointsNormalizedFiltered.size());

    // store
    data["features"] = featuresFiltered;
    data["positions"] = keypointsNormalizedFiltered;
    data["numfeatures"] = static_cast<int32_t>(featuresFiltered.size());
}

void shog_generator::detect(const cv::Mat& image, vec_vec_f32_t& keypoints) const
{
    assert(image.type() == CV_8UC1);
    _sampler->sample(keypoints, image);
}

double shog_generator::scale(const cv::Mat& image, cv::Mat& scaled) const
{
    return scaleToSideLength(image, _width, scaled);
}


void shog_generator::extract(const cv::Mat& image, const vec_vec_f32_t& keypoints, vec_vec_f32_t& features, vector<index_t>& emptyFeatures) const
{
    using namespace cv;

    assert(image.type() == CV_8UC1);

    //cv::imwrite("input.png", image);

    // Smooth sketch slightly to be able to compute smoother orientations
    // OpenCV computes the appropriate sigma for the given kernel size
    Size kernelSize(7,7);
    double sigma = 2;
    Mat imageBlurred = image.clone();
    cv::GaussianBlur(image, imageBlurred, kernelSize, sigma);

    assert(image.size() == imageBlurred.size());

    // compute gradients
    cv::Mat gx, gy;
    cv::Sobel(imageBlurred, gx, CV_32FC1, 1, 0);
    cv::Sobel(imageBlurred, gy, CV_32FC1, 0, 1);

    // compute orientations per pixel. Additionally, we store the magnitude
    // of the gradient which we use as a weighting factor in the histogram.
    // This helps to better localize the edges which have been a bit blurred
    // in order to be able to compute smooth orientations.
    Mat orient(gx.size(), CV_32FC2);
    for (int r = 0; r < gx.rows; r++) {
        for (int c = 0; c < gx.cols; c++) {
            float gxx = gx.at<float>(r,c);
            float gyy = gy.at<float>(r,c);
            float len = std::sqrt(gxx*gxx + gyy*gyy);

            float o = gyy / (len + std::numeric_limits<float>::epsilon());
            if (gxx < 0) o = -o;
            orient.at<Vec2f>(r,c)[0] = acos(o);
            orient.at<Vec2f>(r,c)[1] = len;
        }
    }

    // allocate orientation response matrices, filled with zeros
    std::vector<Mat> orientations;
    for (uint i = 0; i < _numOrients; i++) {
        orientations.push_back(Mat::zeros(image.size(), CV_32FC1));
    }

    // split the orientation image into _numOrientation response images and
    // perform cyclic smoothing along the orientation
    for (int r = 0; r < gx.rows; r++) {
        for (int c = 0; c < gx.cols; c++) {

            // Note: we work with a bin spacing of 1, i.e. if we want to quantize into
            // 4 orientation bins, the bins have the range [0,4] each with a width of 1
            float halfBin = 0.5;

            float orientation = orient.at<Vec2f>(r,c)[0];
            float magnitude = orient.at<Vec2f>(r,c)[1];

            // if the gradient magnitude is zero, we are currently looking
            // at a blank area in the sketch and we therefore cannot add
            // anything meaningful into the descriptor, so skip this pixel
            if (magnitude == 0) {
                continue;
            }

            // normalize orientations into range [0, numOrientations]
            float val = (orientation / M_PI)*_numOrients;
            val = fmod(val, static_cast<float>(_numOrients));

            // so far, val is in range [0,numOrients] where the value numOrients only
            // occurs if the orientation is exactly pi. This is in fact equal to the
            // orientation = 0, so force this to the same number by applying a modulo operation.
            //int binIdx = static_cast<int>(val) % _numOrients;
            int binIdx = static_cast<int>(val);

            // distance of val to center of right bin determines
            // fraction of energy that goes into right bin
            float rCenter = binIdx + 1 + halfBin;
            float rDiff = rCenter - val;
            float rVal = std::max(1-rDiff,0.0f);
            assert(rVal >= 0 && rVal <= 0.5);

            // distance of val to center of left bin determines
            // fraction of energy that goes into left bin
            float lCenter = binIdx - 1 + halfBin;
            float lDiff = val - lCenter;
            float lVal = std::max(1-lDiff,0.0f);
            assert(lVal >= 0 && lVal <= 0.5);


            orientations[binIdx].at<float>(r,c) += (1 - lVal - rVal)*magnitude;
            orientations[(binIdx+1) % _numOrients].at<float>(r,c) += rVal*magnitude;
            // the additional +_numOrients is to avoid working on negative
            // number with the modulo operator
            orientations[(binIdx+_numOrients-1) % _numOrients].at<float>(r,c) += lVal*magnitude;
        }
    }

    // debug output of orientational response images
    //    for (size_t i = 0; i < orientations.size(); i++) {
    //        Mat o = orientations[i];
    //        double vmin,vmax;
    //        minMaxLoc(o, &vmin, &vmax);
    //        o = (o-vmin) / (vmax-vmin);
    //        o*=255;
    //        cv::imwrite("orientation_" + boost::lexical_cast<string>(i) + ".png", o);
    //    }

    // local region size is defined to be relative to image area
    int featureSize = std::sqrt(image.size().area() * _featureSize);

    // if not multiple of _tiles then round up
    if (featureSize % _tiles)
    {
        featureSize += _tiles - (featureSize % _tiles);
    }

    int tileSize = featureSize / _tiles;
    float halfTileSize = tileSize / 2.0f;


    // spatial smooting
    for (size_t i = 0; i < _numOrients; i++)
    {
        // copy response image centered into a new, larger image that contains an empty border
        // of size tileSize around all sides. This additional  border is essential to be able to
        // later compute values outside of the original image bounds
        cv::Mat framed = cv::Mat::zeros(imageBlurred.rows + 2*tileSize, imageBlurred.cols + 2*tileSize, CV_32FC1);
        cv::Mat image_rect_in_frame = framed(cv::Rect(tileSize, tileSize, imageBlurred.cols, imageBlurred.rows));
        orientations[i].copyTo(image_rect_in_frame);

        if (_smoothHist)
        {
            int kernelSize = 2 * tileSize + 1;
            float gaussBlurSigma = tileSize / 3.0;
            cv::GaussianBlur(framed, framed, cv::Size(kernelSize, kernelSize), gaussBlurSigma, gaussBlurSigma); // TODO: border type?
        }
        else
        {
            int kernelSize = tileSize;
            cv::boxFilter(framed, framed, CV_32F, cv::Size(kernelSize, kernelSize), cv::Point(-1, -1), false); // TODO: border type?
        }

        // response have now size of image + 2*tileSize in each dimension
        orientations[i] = framed;
    }


    // integral image to be able to easily check whether a region that is
    // overlapped by a feature actually contains a sketch stroke
    // We invert the image such that the backgound is black (0) and strokes
    // are white (255). So if the sum in certain region is 0, we know that
    // no stroke goes through this region.
    cv::Mat_<unsigned char> inverted = 255 - image;
    cv::Mat_<int> integral;
    cv::integral(inverted, integral, CV_32S);


    // will contain a 1 at each index where the underlying patch in the
    // sketch is completely empty, i.e. contains no stroke, 0 at all other
    // indices. Therefore it is essentail that this vector has the same size
    // as the keypoints and features vector
    emptyFeatures.resize(keypoints.size(), 0);

    // collect orientational responses for each keypoint/region
    for (size_t i = 0; i < keypoints.size(); i++)
    {
        //const cv::Point2f& keypoint = keypoints[i];
        const vec_f32_t& keypoint = keypoints[i];

        // create histogram: row <-> tile, column <-> histogram of directional responses
        vec_f32_t histogram(_tiles * _tiles * _numOrients, 0.0f);

        // define region of the feature, intersect feature region with original image
        // to determine the overlapping region, we can now make use of the integral image
        // to quickly check if this region contains any sketch strokes
        //cv::Rect rect(keypoint.x - featureSize/2, keypoint.y - featureSize/2, featureSize, featureSize);
        cv::Rect rect(keypoint[0] - featureSize/2, keypoint[1] - featureSize/2, featureSize, featureSize);
        cv::Rect isec = rect & cv::Rect(0, 0, image.cols, image.rows);

        // check if patch contains any strokes of the sketch
        int patchsum = integral(isec.tl())
                + integral(isec.br())
                - integral(isec.y, isec.x + isec.width)
                - integral(isec.y + isec.height, isec.x);

        if (patchsum == 0)
        {
            // skip this patch. It contains no strokes.
            // add empty histogram, filled with zeros,
            // will be (optionally) filtered in a later descriptor computation step
            features.push_back(histogram);
            emptyFeatures[i] = 1;
            continue;
        }


        // adjust rect position by frame width
        rect.x += tileSize;
        rect.y += tileSize;


        const int ndims[3] = { _tiles, _tiles, _numOrients };
        cv::Mat_<float> hist(3, ndims, 0.0f);

        for (size_t k = 0; k < orientations.size(); k++)
        {
            for (int y = rect.y + halfTileSize; y < rect.br().y; y += tileSize)
            {
                for (int x = rect.x + halfTileSize; x < rect.br().x; x += tileSize)
                {
                    // check for out of bounds condition
                    // NOTE: we have added a frame with the size of a tile
                    if (y < 0 || x < 0 || y >= orientations[k].rows || x >= orientations[k].cols)
                    {
                        continue;
                    }

                    // get relative coordinates in current patch
                    int ry = y - rect.y;
                    int rx = x - rect.x;

                    // get tile indices
                    int tx = rx / tileSize;
                    int ty = ry / tileSize;

                    assert(tx >= 0 && ty >= 0);
                    assert(tx < static_cast<int>(_tiles) && ty  < static_cast<int>(_tiles));

                    hist(ty, tx, k) = orientations[k].at<float>(y, x);
                }
            }
        }

        std::copy(hist.begin(), hist.end(), histogram.begin());

        // l2 normalization
        float sum = 0;
        for (size_t i = 0; i < histogram.size(); i++) sum += histogram[i]*histogram[i];
        sum = std::sqrt(sum)  + std::numeric_limits<float>::epsilon(); // + eps avoids div by zero
        for (size_t i = 0; i < histogram.size(); i++) histogram[i] /= sum;


        // add histogram to the set of local features for that image
        features.push_back(histogram);
    }
}

bool shog_registered = Generator::register_generator<shog_generator>("shog");

} // namespace imdb

