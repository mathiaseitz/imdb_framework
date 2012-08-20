/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef DESCRIPTORS__GIST_HELPER_HPP
#define DESCRIPTORS__GIST_HELPER_HPP

#include <complex>
#include <cstddef>
#include <cmath>
#include <algorithm>

#include <boost/static_assert.hpp>
#include <opencv2/core/core.hpp>

////////////////////////////////////////////////////////////////////////////////////////////////


template <class T>
void fftshift_even(const cv::Mat_<T>& src, cv::Mat_<T>& dst)
{
    assert(src.isContinuous() && dst.isContinuous());

    //assert that size is even!!!

    dst.create(src.size());

    const int w = src.size().width;
    const int h = src.size().height;
    const int hw = w / 2;
    const int hh = h / 2;

    for (int y = 0; y < hh; y++)
    {
        // src[i] gives the ith row with the T datatype
        std::copy(src[y], src[y] + hw, dst[y + hh] + hw); // copy tl quadrant to br quadrant
        std::copy(src[y] + hw, src[y] + w, dst[y + hh]);      // copy tr quadrant to bl quadrant
    }

    for (int y = hh; y < h; y++)
    {
        std::copy(src[y], src[y] + hw, dst[y - hh] + hw); // copy bl quadrant to tr quadrant
        std::copy(src[y] + hw, src[y] + w, dst[y - hh]);      // copy br quadrant to tl quadrant
    }
}





template <class T>
void generate_gaussian_filter(cv::Mat_<T>& image, double sigma)
{
    const int w = image.size().width;
    const int h = image.size().height;
    const int wh = w / 2;
    const int hh = h / 2;

    const double s = 1.0 / (sigma*sigma);

    for (int y = -hh; y < hh; y++)
    {
        size_t yy = (y + h) % h;
        for (int x = -wh; x < wh; x++)
        {
            size_t xx = (x + w) % w;
            double fx = x;
            double fy = y;
            image(yy, xx) = std::exp(-(fx*fx + fy*fy) * s);
        }
    }
}

class torralba_prefilter
{
    typedef std::complex<float> complex_t;

    double      _sigma;
    cv::Size    _size;
    cv::Mat_<complex_t> _filter;

    public:

    torralba_prefilter(std::size_t width, std::size_t height, double cycles = 4.0)
     : _sigma(cycles / std::sqrt(std::log(2.0)))
     , _size(width, height)
     , _filter(_size)
    {
        generate_gaussian_filter(_filter, _sigma);
    }

    void operator() (cv::Mat& img)
    {
        assert(img.type() == CV_8UC1 && img.size().width == _size.width && img.size().height == _size.height);

        // "whitening"
        cv::Mat_<float> logimg;
        img.convertTo(logimg, CV_32FC1);
        cv::log(1.0 + logimg, logimg);

        cv::Mat_<complex_t> spbuf(_size);
        std::copy(logimg.begin(), logimg.end(), spbuf.begin());

        cv::Mat_<complex_t> frbuf;
        cv::dft(spbuf, frbuf);

        cv::mulSpectrums(frbuf, 1.0 - _filter, frbuf, 0);

        cv::Mat_<complex_t> white;
        cv::idft(frbuf, white, cv::DFT_SCALE);

        // "local contrast normalization"
        cv::MatIterator_<complex_t> dit = spbuf.begin();
        for (cv::MatConstIterator_<complex_t> it = white.begin(); it != white.end(); ++it, ++dit)
        {
            const complex_t& v = *it;
            *dit = v.real() * v.real();
        }

        cv::dft(spbuf, frbuf);
        cv::mulSpectrums(frbuf, _filter, frbuf, 0);
        cv::idft(frbuf, spbuf, cv::DFT_SCALE);

        cv::MatIterator_<unsigned char> dst = img.begin<unsigned char>();
        cv::MatConstIterator_<complex_t> wit = white.begin();
        for (cv::MatConstIterator_<complex_t> it = spbuf.begin(); it != spbuf.end(); ++it, ++wit, ++dst)
        {
            float d = std::sqrt(std::abs((*it).real())) + 0.2;
            float v = std::min(255 * std::max((*wit).real(), 0.0f) / d, 255.0f);
            *dst = v;
        }
    }
};

template <class T>
void generate_gabor_filter(cv::Mat_<T>& image, double peakFreq, double deltaFreq, double orientAngle, double deltaAngle)
{
    const double C = std::sqrt(log(2.0) / M_PI);

    const double Ka = (deltaFreq - 1.0) / (deltaFreq + 1.0);
    const double Kb = std::tan(0.5 * deltaAngle);
    //const double lambda = Ka / Kb;

    // scaling factors of the gaussian envelope
    const double a = peakFreq * (Ka / C);
    //const double b = a / lambda;
    const double b = Kb * peakFreq/C * std::sqrt(1.0 - Ka*Ka);

    // spatial frequency in cartesian coordinates
    const double u0 = peakFreq * std::cos(orientAngle);
    const double v0 = peakFreq * std::sin(orientAngle);

    // default: set orientation of gaussian envelope (theta) equal to orientation of filter
    const double theta = orientAngle;

    // generate filter
    const size_t w = image.size().width;
    const size_t h = image.size().height;
    const double stepx = 1.0 / static_cast<double>(w);
    const double stepy = 1.0 / static_cast<double>(h);
    const double cos_theta = std::cos(theta);
    const double sin_theta = std::sin(theta);
    double v = 0.5 - v0;

    for (size_t yy = 0; yy < h; yy++)
    {
        size_t y = (yy + (h / 2)) % h;
        double u = -0.5 - u0;
        for (size_t xx = 0; xx < w; xx++)
        {
            size_t x = (xx + (w / 2)) % w;

            double ur = u * cos_theta + v * sin_theta;
            double vr = -u * sin_theta + v * cos_theta;

            double U = ur / a;
            double V = vr / b;

            double value = std::exp(-M_PI * (U*U + V*V));

            image(y, x) = value;

            u += stepx;
        }

        v -= stepy;
    }
}

template <class T>
void generate_polargabor_filter(cv::Mat_<T>& image, double peakFreq, double deltaFreq, double orientAngle, double deltaAngle)
{
    // sigma_omega = 1 / (kappa * omega)
    double kappa = (deltaFreq - 1) / ((deltaFreq + 1) * std::sqrt(2*std::log(2.0)));

//    double sigma_theta = std::sqrt(2*PI)*4.0*numorients/32.0; // torralba
    double sigma_theta = std::sqrt(std::log(2.0)) * 2.0 / deltaAngle;

    // generate filter
    const size_t w = image.size().width;
    const size_t h = image.size().height;
    const double stepx = 1.0 / static_cast<double>(w);
    const double stepy = 1.0 / static_cast<double>(h);

    double v = -0.5;
    for (size_t yy = 0; yy < h; yy++)
    {
        size_t y = (yy + (h / 2)) % h;

        double u = -0.5;
        for (size_t xx = 0; xx < w; xx++)
        {
            size_t x = (xx + (w / 2)) % w;

            double omega = std::sqrt(u*u + v*v);
            double theta = std::atan2(v, u);

            double Omega = omega/peakFreq - 1;
            double Theta = theta + orientAngle;

            if (Theta < -M_PI) Theta += 2*M_PI;
            if (Theta >  M_PI) Theta -= 2*M_PI;

            double value = std::exp(-1/(2*kappa*kappa) * Omega*Omega - sigma_theta*sigma_theta * Theta*Theta);

            image(y, x) = value;

            u += stepx;
        }

        v += stepy;
    }
}

template <class T>
void symmetric_pad(const cv::Mat_<T>& src, cv::Mat_<T>& dst)
{
    cv::Mat_<T> tmp;

    if (src.cols < dst.cols)
    {
        int width = dst.cols;
        int height = std::min(src.rows, dst.rows);

        int pad = dst.cols - src.cols;
        int border = src.cols + pad/2;

        tmp.create(height, width);

        cv::Mat_<T> flipped;
        cv::flip(src, flipped, 1);

        for (int p = 0, k = 0; p < border; p += src.cols, k++)
        {
            int w = std::min(src.cols, border - p);
            int h = height;

            cv::Mat_<T> r = tmp(cv::Rect(p, 0, w, h));

            if (k % 2)
            {
                flipped(cv::Rect(0, 0, w, h)).copyTo(r);
            }
            else
            {
                src(cv::Rect(0, 0, w, h)).copyTo(r);
            }
        }

        for (int p = width, k = 1; p >= border; p -= src.cols, k++)
        {
            int w = std::min(src.cols, p - border);
            int h = height;

            cv::Mat_<T> r = tmp(cv::Rect(p - w, 0, w, h));

            if (k % 2)
            {
                flipped(cv::Rect(src.cols - w, 0, w, h)).copyTo(r);
            }
            else
            {
                src(cv::Rect(src.cols - w, 0, w, h)).copyTo(r);
            }
        }
    }
    else
    {
        tmp = src;
    }

    if (src.rows < dst.rows)
    {
        int width = dst.cols;
        int height = dst.rows;

        int pad = dst.rows - src.rows;
        int border = src.rows + pad/2;

        cv::Mat_<T> flipped;
        cv::flip(tmp, flipped, 0);

        for (int p = 0, k = 0; p < border; p += src.rows, k++)
        {
            int w = width;
            int h = std::min(src.rows, border - p);

            cv::Mat_<T> r = dst(cv::Rect(0, p, w, h));

            if (k % 2)
            {
                flipped(cv::Rect(0, 0, w, h)).copyTo(r);
            }
            else
            {
                tmp(cv::Rect(0, 0, w, h)).copyTo(r);
            }
        }

        for (int p = height, k = 1; p >= border; p -= src.rows, k++)
        {
            int w = width;
            int h = std::min(src.rows, p - border);

            cv::Mat_<T> r = dst(cv::Rect(0, p - h, w, h));

            if (k % 2)
            {
                flipped(cv::Rect(0, src.rows - h, w, h)).copyTo(r);
            }
            else
            {
                tmp(cv::Rect(0, src.rows - h, w, h)).copyTo(r);
            }
        }
    }
    else
    {
        tmp.copyTo(dst);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////


#endif // DESCRIPTORS__GIST_HELPER_HPP
