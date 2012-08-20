/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef PROPERTY_WRITER_HPP
#define PROPERTY_WRITER_HPP

#include "../util/types.hpp"
#include "io.hpp"
#include "type_names.hpp"


namespace imdb {

/// \addtogroup io
/// @{


/**
  * @brief Interface for templated PropertyWriterT.
  *
  * In order for the derived templated class PropertyWriterT to have the same interface, we
  * use boost::any to pass in the data to be written. This works transparently for users as
  * most datatypes get automatically packaged into a boost:any. Thus, users can still
  * simply call push_back() passing e.g. a vector<float>.
  */
struct PropertyWriter
{
    virtual ~PropertyWriter() {}
    virtual void open(const string& filename) = 0;
    virtual bool push_back(const boost::any&) = 0;
    virtual bool insert(const boost::any& element, size_t pos) = 0;
};


/**
 * @brief Writes a binary vector-like file ('property file') to harddisk that contains elements of type T
 *
 * In our usage-scenario, T usually is a descriptor that encodes an image/sketch. A typical example is
 * T = std::vector<float> in the case of a global descriptor. In a bag-of-features approach, typically
 * T = vector<vector<float> >, i.e. all local features of a single image. PropertyWriterT supports all
 * element types T that are implemented in imdb::io as well as arbitrary nestings of those.
 *
 * PropertyWriterT supports huge files without any performance loss, typically limited only by your
 * free harddisk space. We have sucessfully generated files close to a Terabyte in size. The files are
 * directly portable between 64-bit and 32-bit machines but not portable between machines of differing endianness.
 *
 * Use PropertyReaderT to read in a property file that has been written with PropertyWriterT.
 *
 * Note: instances are noncopyable since they internally open files.
 *
 */
template <class T>
class PropertyWriterT : public PropertyWriter, boost::noncopyable
{
public:


    // if you change the internal format, be sure to also adapt the reader
    static int version()
    {
        return 2;
    }

    PropertyWriterT() {}

    /// Open the passed filename for writing, if the file aready exists, its content will be overwritten
    PropertyWriterT(const string& filename)
    {
        this->open(filename);
    }

    /// Open the passed filename for writing, if the file aready exists, its content will be overwritten
    void open(const string& filename)
    {
        _ofs.open(filename.c_str(), std::ofstream::binary|std::ofstream::trunc);
        if (!_ofs.is_open()) throw std::runtime_error("could not open file " + filename);
        _map["__version"] = boost::lexical_cast<std::string>(version());
        string type_name = imdb::nameof<T>();
        _map["__typeinfo"] = type_name;
    }


    ~PropertyWriterT()
    {
        if (!_ofs.is_open()) return;

        int64_t p_features = 0;
        _map["__features"] = boost::lexical_cast<std::string>(p_features);

        int64_t p_offsets = _ofs.tellp();
        _map["__offsets"] = boost::lexical_cast<std::string>(p_offsets);
        io::write(_ofs, _offset);

        int64_t p_map = _ofs.tellp();
        io::write(_ofs, _map);
        io::write(_ofs, p_map);

        _ofs.close();
    }


    /// Append an element to the end of the file
    bool push_back(const boost::any& element)
    {
        assert(_ofs.is_open());
        _offset.push_back(_ofs.tellp());
        io::write(_ofs, boost::any_cast<T>(element));
        assert(_ofs.good());
        return true;
    }

    /// Insert an element at an arbitrary position in the file
    bool insert(const boost::any& element, size_t pos)
    {
        assert(_ofs.is_open());
        if (_offset.size() <= pos) _offset.resize(pos + 1, -1);
        _offset[pos] = _ofs.tellp();
        io::write(_ofs, boost::any_cast<T>(element));
        assert(_ofs.good());
        return true;
    }


// Mathias 22.03.2012: seems to be unused, delete at some point
//    bool insert_map_entry(const std::string& key, const std::string& value)
//    {
//        return _map.insert(typename strmap_t::value_type(key, value)).second;
//    }

private:

    std::ofstream        _ofs;
    std::vector<int64_t> _offset;
    strmap_t             _map;
};



/**
 * @brief Convenience function for getting a boost::shared_ptr to a PropertyWriter for elements of type T.
 *
 * Creates a PropertyWriterT that writes to the file with \p filename. PropertyWriterT overwrites existing
 * content in case the file already exists, otherwise it will be created.
 */
template <class T>
boost::shared_ptr<PropertyWriter> create_writer(const std::string& filename)
{
    PropertyWriterT<T>* pw = new PropertyWriterT<T>();
    pw->open(filename);
    return boost::shared_ptr<PropertyWriter>(pw);
}

/**
 * @brief Convenience function to write a complete vector<T> to filename.
 *
 * Writes to the file specified by filename. PropertyWriterT overwrites existing content in case
 * the file already exists, otherwise it will be created.
 */
template <class T>
void write_property(const std::vector<T>& v, const std::string& filename)
{
    PropertyWriterT<T> wr;
    wr.open(filename);
    for (size_t i = 0; i < v.size(); i++) wr.push_back(v[i]);
}



/**
 * @brief Encapsulates a set of writers each identified by a unique name
 *
 * The purpose of this class simply is to hold a set of PropertyWriterT working on
 * potentially different datatypes, i.e. T can be different for all PropertyWriterT
 * in this class. At some later point, they can be opened for writing using
 * the base class interface PropertyWriter.
 */
class PropertyWriters
{
public:

    /// associates a writer with a name
    typedef map<string, shared_ptr<PropertyWriter> > properties_t;


    /**
     * @brief Add a PropertyWriterT (working in elements of type T) to the current map of writers.
     *
     * You can add PropertyWriters of arbitrary type T as we internally store a pointer
     * to the base class. Note that we return a reference to this such than we can add
     * multiple writers in a single line.
     * @param name name of the PropertyWriterT to be added
     */
    template <class T>
    PropertyWriters& add(const std::string& name)
    {
        _properties[name] = make_shared<PropertyWriterT<T> >();
        return *this;
    }

    // Note: can't make this function const as a common usecase is that
    // we actually want to open() the writers for writing data to a file
    properties_t& get()
    {
        return _properties;
    }

private:

    properties_t _properties;
};

/// @} // end addtogroup

} // end namespace




#endif // PROPERTY_WRITER_HPP
