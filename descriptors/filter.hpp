#ifndef FILTER_HPP
#define FILTER_HPP

#include <cmath>

#include <opencv2/core/core.hpp>

const double PI = 3.14159265358979323846;

/*
 * image           image buffer to be filled
 * peakFreq        central or peak frequency of the filter response
 * deltaFreq       distance of the half-magnitude and peak frequency
 * orientAngle     orientation of the filter
 * deltaAngle      distance of the half-magnitude and peak frequency
 */
template <class image_t>
void generate_gabor_filter(image_t& image, double peakFreq, double deltaFreq, double orientAngle, double deltaAngle)
{
    const double C = std::sqrt(log(2.0) / PI);

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
    const size_t w = image.width();
    const size_t h = image.height();
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

            double value = std::exp(-PI * (U*U + V*V));

            image(x, y) = value;

            u += stepx;
        }

        v -= stepy;
    }
}

template <class image_t>
void generate_polargabor_filter(image_t& image, double peakFreq, double deltaFreq, double orientAngle, double deltaAngle)
{
    // sigma_omega = 1 / (kappa * omega)
    double kappa = (deltaFreq - 1) / ((deltaFreq + 1) * std::sqrt(2*std::log(2)));

//    double sigma_theta = std::sqrt(2*PI)*4.0*numorients/32.0; // torralba
    double sigma_theta = std::sqrt(std::log(2.0)) * 2.0 / deltaAngle;

    // generate filter
    const size_t w = image.width();
    const size_t h = image.height();
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

            if (Theta < -PI) Theta += 2*PI;
            if (Theta >  PI) Theta -= 2*PI;

            double value = std::exp(-1/(2*kappa*kappa) * Omega*Omega - sigma_theta*sigma_theta * Theta*Theta);


            image(x, y) = value;
//            image(xx, yy) = value;

            u += stepx;
        }

        v += stepy;
    }
}

template <class image_t>
void generate_gaussian_filter(image_t& image, double sigma)
{
    const int w = image.width();
    const int h = image.height();
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
            image(xx, yy) = std::exp(-(fx*fx + fy*fy) * s);
        }
    }
}

#endif // FILTER_HPP
