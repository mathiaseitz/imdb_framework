/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef DESCRIPTORS__TINYLAB_H
#define DESCRIPTORS__TINYLAB_H

#include <cstdlib>

#include "../util/types.hpp"
#include "../descriptors/generator.hpp"

/**
  * One of the most simple image descripors possible,
  * this one only store a downscaled version of the
  * original image it represents.
  * The data is stored as pixels in L*a*b colorspace
  * since Euclidean distances in that colorspace
  * match human perception of color differences quite well.
  */

namespace imdb {


/**
 * @ingroup generators
 * @brief The tinyimage_generator class
 */
class tinyimage_generator : public Generator
{
    public:

    tinyimage_generator(const ptree& params);

    void compute(anymap_t& data) const;

    private:

    const std::size_t _width;
    const std::size_t _height;
    const std::string _colorspace;
};

} // namespace imdb

#endif // DESCRIPTORS__TINYLAB_H
