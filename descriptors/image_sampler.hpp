/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef IMAGE_SAMPLER_H
#define IMAGE_SAMPLER_H

#include <vector>
#include <opencv2/imgproc/imgproc.hpp>
#include <boost/bind.hpp>
#include "../util/types.hpp"
#include "../util/registry.hpp"

namespace imdb {


class ImageSampler
{

public:

    typedef map<string, function<shared_ptr<ImageSampler> ()> > samplers_t;

    template <class sampler_t>
    static inline bool register_sampler(const string& name)
    {
        // Note: the get() function adds a sampler_t object to the registry upon first call
        samplers_t& samplers_map = registry().get<samplers_t>("samplers");
        samplers_map[name] = boost::bind(create<sampler_t>);
        return true;
    }

    static inline shared_ptr<ImageSampler> create(const std::string& name) {

        samplers_t& samplers = registry().get<samplers_t>("samplers");

        // make sure that a sampler with the desired name exist in the
        // registry, i.e. the user has added it using ImageSampler::register_sampler
        samplers_t::const_iterator cit = samplers.find(name);
        if (cit == samplers.end())
        {
            throw std::runtime_error("imdb::ImageSampler: no sampler with name '" + name + "' registered.");
        }

        return registry().get<samplers_t>("samplers").at(name)();
    }

    // Note params is non-const on purpose: each sampler comes with a
    // set of default-parameters that are inserted into the ptree if the
    // corresponding parameters name is not yet existing in the tree
    virtual void setParameters(ptree& params)=0;

    virtual ~ImageSampler() {}

    virtual void sample(vec_vec_f32_t& samples, const cv::Mat& image) const=0;

private:

    template <class sampler_t>
    static shared_ptr<ImageSampler> create()
    {
        return boost::make_shared<sampler_t>();
    }
};


class grid_sampler : public ImageSampler
{
public:

    virtual ~grid_sampler() {}

    void setParameters(ptree &params);
    void sample(vec_vec_f32_t& samples, const cv::Mat &image) const;

private:

    uint _numSamples;
};


class random_area_sampler : public ImageSampler
{
public:

    virtual ~random_area_sampler() {}
    void setParameters(ptree &params);
    void sample(vec_vec_f32_t& samples, const cv::Mat &image) const;

private:

    unsigned int _numSamples;
};


} // end namespace

#endif // IMAGE_SAMPLER_H
