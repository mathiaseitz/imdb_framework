/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef TYPES_HPP
#define TYPES_HPP

#include <vector>
#include <map>
#include <string>
#include <utility>

#include <boost/cstdint.hpp>
#include <boost/any.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/function.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <opencv2/core/core.hpp>


namespace imdb {

// namespace includes
using boost::shared_ptr;
using boost::make_shared;
using boost::any_cast;
using boost::scoped_ptr;
using boost::dynamic_pointer_cast;
using boost::static_pointer_cast;
using boost::const_pointer_cast;
using boost::property_tree::ptree;
using boost::function;

using std::string;
using std::vector;
using std::pair;
using std::make_pair;
using std::map;

typedef unsigned int uint;
typedef int64_t      index_t;

/// Datatype used to represent the result of a search, dist_idx_t.first
/// contains the distance, dist_idx_t.second the index into the search
/// datastructure pointing to the element being compared.
typedef std::pair<double, index_t> dist_idx_t;

typedef cv::Mat_<cv::Vec3b> mat_8uc3_t;
typedef cv::Mat_<unsigned char> mat_8uc1_t;

typedef boost::any feature_t;

typedef std::vector<float>     vec_f32_t;
typedef std::vector<int32_t>   vec_i32_t;
typedef std::vector<int8_t>    vec_i8_t;

// Mathias 31.12.2011
// u_int_32_t and u_int8_t seem to be BSD specific
// and do not work on the Mac. uint32_t and uint8_t
// seem to be more standard and work on both Windows an Mac
//typedef std::vector<u_int32_t> vec_u32_t;
//typedef std::vector<u_int8_t>  vec_u8_t;
typedef std::vector<uint32_t> vec_u32_t;
typedef std::vector<uint8_t>  vec_u8_t;


typedef std::vector<vec_f32_t> vec_vec_f32_t;
typedef std::vector<vec_i32_t> vec_vec_i32_t;

typedef std::map<std::string, std::string> strmap_t;
typedef std::map<std::string, boost::any>  anymap_t;

template <class T> inline
bool less_second(const T& a, const T& b)
{
    return (a.second < b.second);
}

template <class T> inline
T get(const strmap_t& map, const std::string& key, const T& defaultvalue = T())
{
    strmap_t::const_iterator it = map.find(key);
    return (it != map.end()) ? boost::lexical_cast<T>(it->second) : defaultvalue;
}

template <class T> inline
T get(const anymap_t& map, const std::string& key, const T& defaultvalue = T())
{
    anymap_t::const_iterator it = map.find(key);
    return (it != map.end()) ? boost::any_cast<T>(it->second) : defaultvalue;
}

// Returns the value that is stored in the property_tree under path.
// If path does not exist, the default value is inserted into the tree
// and returned.
template <class T> inline
T parse(ptree& p, const string& path, const T& default_value)
{
    T value = p.get(path, default_value);
    p.put(path, value);
    return value;
}

} // namespace imdb

#endif // TYPES_HPP
