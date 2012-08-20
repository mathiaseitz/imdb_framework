/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef GENERATOR_HPP
#define GENERATOR_HPP

#include <boost/bind.hpp>

#include <opencv2/core/core.hpp>

#include "util/types.hpp"
#include "util/registry.hpp"
#include "io/property_writer.hpp"


namespace imdb {

/**
 * @ingroup generators
 * @brief Interface for all generators (a Generator is a class, that given an image
 * generates a set of features out of this image.
 */
class Generator
{

    public:

    typedef map<string, function<shared_ptr<Generator> (const ptree&)> > generators_t;

    // -----------------------------------
    // static functions
    // -----------------------------------


    /**
     * @brief Register a Generator of type generator_t in the Registry.
     *
     * Note that in our implementation of all provided Generators, each Generator
     * registers itself (see e.g. the end of the tinyimage_generator implementation).
     * This means that by simply adding the tinyimage.cpp file to your project, the
     * tinyimage_generator is automatically registered and ready to be used.
     *
     * @param name The name to be used for this Generator. Typically you want to use something that identifies
     * the Generator, e.g. "gist" for the GIST Generator.
     */
    template <class generator_t>
    static inline bool register_generator(const string& name)
    {
        generators_t& generators_map = registry().get<generators_t>("generators");
        generators_map[name] = boost::bind(create<generator_t>, name, boost::arg<1>());
        return true;
    }

    /**
     * @brief Creates a Generator from a parameters file (in JSON format).
     *
     * The created Generator has exactly the same parameters as
     * stored in the file. Note that the parameters file needs to
     * contain at least a "name" property that is a valid name of a
     * Generator that has been registered using register_generator().
     * All other valid parameters options depend on the
     * specific Generator and are listed in its constructor. For all invalid/
     * unspecified parameter options, the Generator uses its default values.
     *
     * @param filename Filename of the JSON file with Generator parameters
     * @throws boost::property_tree::json_parser_error If the JSON file at filename cannot be opened or parsed
     * @throws std::runtime_error If no Generator with the name in the parameters file is registered
     * @return Generator using the specified parameters
     */
    static shared_ptr<Generator> from_parameters_file(const string& filename);


    /**
     * @brief Creates a Generator using all its default parameters.
     * @param name Name of the Generator, must be a valid registered Generator name
     * @throws std::runtime_error If no Generator with the given name is registered
     * @return Generator using default parameters as specified in its constructor
     */
    static shared_ptr<Generator> from_default_parameters(const string& name);

    /**
     * @brief Creates a Generator from a given boost::property_tree file
     *
     * Behaviour/Requirements are exactly the same as for from_parameters_file().
     *
     * @param params boost::property_tree containing Generator parameters
     * @throws std::runtime_error If the JSON file cannot be parsed or if no
     * Generator is registered under the name given in the property tree
     * @return Generator using the specified parameters
     */
    static shared_ptr<Generator> from_parameters(const ptree &params);



    static const generators_t& generators();


    // -------------------------------------------
    // class members
    // -------------------------------------------

    /**
     * @brief Construct a Generator given a set of parameters and PropertyWriters (both defined in the derived class).
     *
     * Note that since Generator is a pure abstract base class, this constructor can never be explicitly called. The
     * parameters are just passed through from derived classes and Generator stores them as members.
     *
     * @param parameters A boost::property_tree holding the key/value pairs that identify the parameters for this
     * Generator, see the derived Generator classes for valid options
     * @param propertyWriters A list of PropertyWriters that are used to serialized the data/features extracted by the Generator to harddisk.
     */
    Generator(const ptree& parameters, const PropertyWriters& propertyWriters);


    virtual ~Generator();


    /**
     * @brief Extract features from what is contained in data.
     *
     * This member function performs the actual feature extraction and needs to be implemented
     * in derived classes. While we kept the parameter data very general (a map from std::string
     * to a boost::any type) our internal convention used in compute_descriptors is that data
     * contains the key/value pair
     * "image" -> OpenCV 3 channel 8-bit image (mat_8uc3_t).
     *
     * @param data A map from std::string to a boost::any type containing the actual data a Generator
     * works on (typically an OpenCV image, although any other datatype that fits into a boost::any is
     * possible as well).
     */
    virtual void compute(anymap_t& data) const = 0;


    // Note: we cannot return as const as a common usecase is
    // to open() the writers for actually writing to a file
    PropertyWriters& propertyWriters();

    /**
     * @brief The actual parameters as used by the Generator.
     *
     * Note that this might be different from the parameters originally passed in. All valid
     * parameters passed in when creating the Generator are used, all invalid or unspecified
     * parameter values are set to their default values.
     *
     * @return boost::property_tree containing key/value pairs of paramter name/parameter as actually
     * used by the Generator for extracting features.
     */
    const ptree& parameters() const;

    protected:

    // must be protected instead of private since specific generators
    // make use of the parse() function which must be able to
    // replace/add entries to the property tree
    ptree             _parameters;
    PropertyWriters   _propertyWriters;
    
    private:

    template <class generator_t>
    static shared_ptr<Generator> create(const string& name, const ptree& parameters)
    {
        ptree extendedParams(parameters);
        extendedParams.put("generator.name", name);
        return boost::make_shared<generator_t>(extendedParams);
    }
};

} // namespace imdb

#endif // GENERATOR_HPP
