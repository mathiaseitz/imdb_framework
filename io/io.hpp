/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef IO_HPP
#define IO_HPP

#include <istream>
#include <ostream>
#include <vector>
#include <map>
#include <set>

#include <boost/cstdint.hpp>
#include <boost/array.hpp>
#include <boost/type_traits/is_arithmetic.hpp>

namespace imdb {

/**
 * @ingroup IO
 * @brief Handles all input/output operations we use for serializing data using PropertyWriterT and PropertyReaderT.
 *
 * Currently supports reading/writing of the most commonly used STL containers such as vector<T>, set<T>, map<S,T> and pair<T1,T2>
 * as well as arbitrary nestings of those. In our use-case, we usually read/write huge amounts of data stored as vector<T> and
 * therefore this operation is especially efficient.
 */
namespace io
{
    // ----------------------------------------------------------------------------
    // Signatures -- we need them to allow arbitrary nestings in the following
    // implementation part. Otherwise the implementation for e.g. std::pair<T1,T2>
    // would need to be defined before the one of std::vector<T> when trying to
    // read/write a vector<pair<T1,T2>>
    // ----------------------------------------------------------------------------

    template <class T>
    size_t write(std::ostream& os, const T& v);

    template <class T>
    size_t read(std::istream& is, T& v);

    template <class T, std::size_t N>
    size_t write(std::ostream& os, const boost::array<T, N>& v);

    template <class T, std::size_t N>
    size_t read(std::istream& is, boost::array<T, N>& v);

    template <class T1, class T2>
    size_t write(std::ostream& os, const std::pair<T1, T2>& v);

    template <class T1, class T2>
    size_t read(std::istream& is, std::pair<T1, T2>& v);

    template <class T>
    size_t write(std::ostream& os, const std::vector<T>& v);

    template <class T>
    size_t read(std::istream& is, std::vector<T>& v);

    template <class T1, class T2>
    size_t write(std::ostream& os, const std::map<T1, T2>& v);

    template <class T1, class T2>
    size_t read(std::istream& is, std::map<T1, T2>& v);

    template <class T>
    size_t write(std::ostream& os, const std::set<T>& v);

    template <class T>
    size_t read(std::istream& is, std::set<T>& v);


    // ----------------------------------------------------------------------------
    // Implementations
    // ----------------------------------------------------------------------------

    template <class T>
    size_t _write(std::ostream& os, T v)
    {
        os.write(reinterpret_cast<char*>(&v), sizeof(T));
        return sizeof(v);
    }

    template <class T>
    size_t _read(std::istream& is, T& v)
    {
        is.read(reinterpret_cast<char*>(&v), sizeof(T));
        return sizeof(v);
    }




   inline size_t write(std::ostream& os, int8_t v)   { return _write(os, v); }
   inline size_t write(std::ostream& os, int16_t v)  { return _write(os, v); }
   inline size_t write(std::ostream& os, int32_t v)  { return _write(os, v); }
   inline size_t write(std::ostream& os, int64_t v)  { return _write(os, v); }
   inline size_t write(std::ostream& os, uint8_t v)  { return _write(os, v); }
   inline size_t write(std::ostream& os, uint16_t v) { return _write(os, v); }
   inline size_t write(std::ostream& os, uint32_t v) { return _write(os, v); }
   inline size_t write(std::ostream& os, uint64_t v) { return _write(os, v); }
   inline size_t write(std::ostream& os, float v)    { return _write(os, v); }
   inline size_t write(std::ostream& os, double v)   { return _write(os, v); }
   inline size_t write(std::ostream& os, const std::string& v)
    {
        size_t t = 0;
        t += write(os, static_cast<int32_t>(v.length()));
        for (size_t i = 0; i < v.length(); i++) t += write(os, reinterpret_cast<const int8_t&>(v[i]));
        return t;
    }

   inline  size_t read(std::istream& is, int8_t& v)   { return _read(is, v); }
    inline size_t read(std::istream& is, int16_t& v)  { return _read(is, v); }
    inline size_t read(std::istream& is, int32_t& v)  { return _read(is, v); }
    inline size_t read(std::istream& is, int64_t& v)  { return _read(is, v); }
    inline size_t read(std::istream& is, uint8_t& v)  { return _read(is, v); }
    inline size_t read(std::istream& is, uint16_t& v) { return _read(is, v); }
    inline size_t read(std::istream& is, uint32_t& v) { return _read(is, v); }
    inline size_t read(std::istream& is, uint64_t& v) { return _read(is, v); }
    inline size_t read(std::istream& is, float& v)    { return _read(is, v); }
    inline size_t read(std::istream& is, double& v)   { return _read(is, v); }
    inline size_t read(std::istream& is, std::string& v)
    {
        int32_t s;
        size_t t = 0;
        t += read(is, s);
        v.resize(s);
        for (size_t i = 0; i < v.length(); i++) t += read(is, reinterpret_cast<int8_t&>(v[i]));
        return t;
    }



    template <class T, std::size_t N>
    size_t write(std::ostream& os, const boost::array<T, N>& v)
    {
        size_t t = 0;
        for (size_t i = 0; i < N; i++) t += write(os, v[i]);
        return t;
    }

    template <class T, std::size_t N>
    size_t read(std::istream& is, boost::array<T, N>& v)
    {
        size_t t = 0;
        for (size_t i = 0; i < N; i++) t += read(is, v[i]);
        return t;
    }


    // writing a vector of data is the most common operation in our use-case
    // and we usually write large vectors (gigabytes to terabytes in size).
    // so we made sure to make this case as efficient as reasonably possible
    template <class T>
    size_t write(std::ostream& os, const std::vector<T>& v)
    {

        size_t t = write(os, static_cast<int64_t>(v.size()));

        // Arithmetic types are all floating points and integral types, see
        // http://www.boost.org/doc/libs/1_48_0/libs/type_traits/doc/html/boost_typetraits/reference/is_arithmetic.html
        //
        // In case the vector contains arithmetic types we use a more
        // efficient implementation and write its whole content at once.
        if (boost::is_arithmetic<T>::value)
        {
            size_t num_bytes = v.size()*sizeof(T);
            os.write(reinterpret_cast<const char*>(&v[0]), num_bytes);
            t+=num_bytes;
        }

        // in case the vector is nested, e.g. a vector<vector<float> > or
        // a vector<map<string, int> > we need to call write for all
        // elements separately
        else
        {
            for (size_t i = 0; i < v.size(); i++) t += write(os, v[i]);
        }

        return t;
    }

    template <class T>
    size_t read(std::istream& is, std::vector<T>& v)
    {
        int64_t size = 0;
        size_t t = read(is, size);
        v.resize(size);

        // Specialized function to read in a complete vector<float/double/int>
        // etc. with a single read. This gives us about 4x performance over
        // calling read for all entries separately (the more general case).
        if (boost::is_arithmetic<T>::value)
        {
            size_t num_bytes = size*sizeof(T);
            is.read(reinterpret_cast<char*>(&v[0]), num_bytes);
            t+=num_bytes;
        }
        else
        {
            for (int64_t i = 0; i < size; i++) t += read(is, v[i]);
        }
        return t;
    }

    template <class T1, class T2>
    size_t write(std::ostream& os, const std::pair<T1, T2>& v)
    {
        size_t s = 0;
        s += write(os, v.first);
        s += write(os, v.second);
        return s;
    }

    template <class T1, class T2>
    size_t read(std::istream& is, std::pair<T1, T2>& v)
    {
        size_t s = 0;
        s += read(is, v.first);
        s += read(is, v.second);
        return s;
    }


    template <class T>
    size_t write(std::ostream& os, const std::set<T>& v)
    {
        size_t s = 0;
        s += write(os, static_cast<int64_t>(v.size()));
        for (typename std::set<T>::const_iterator it = v.begin(); it != v.end(); ++it)
        {
            s += write(os, *it);
        }
        return s;
    }

    template <class T>
    size_t read(std::istream& is, std::set<T>& v)
    {
        size_t s = 0;
        v.clear();
        int64_t size = 0;
        s += read(is, size);
        for (int64_t i = 0; i < size; i++)
        {
            T x;
            s += read(is, x);
            v.insert(x);
        }
        return s;
    }


    template <class T1, class T2>
    size_t write(std::ostream& os, const std::map<T1, T2>& v)
    {
        size_t s = 0;
        s += write(os, static_cast<int64_t>(v.size()));
        for (typename std::map<T1, T2>::const_iterator it = v.begin(); it != v.end(); ++it)
        {
            s += write(os, *it);
        }
        return s;
    }

    template <class T1, class T2>
    size_t read(std::istream& is, std::map<T1, T2>& v)
    {
        size_t s = 0;
        v.clear();
        int64_t size = 0;
        s += read(is, size);
        for (int64_t i = 0; i < size; i++)
        {
            std::pair<T1, T2> x;
            s += read(is, x);
            v.insert(x);
        }
        return s;
    }
}

} // namespace imdb

#endif // IO_HPP
