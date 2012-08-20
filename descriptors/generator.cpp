/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include "generator.hpp"

namespace imdb {


shared_ptr<Generator> Generator::from_parameters_file(const string& filename)
{
    ptree params;
    boost::property_tree::read_json(filename, params);
    return Generator::from_parameters(params);
}


// returns a generator initialized with its default parameters
shared_ptr<Generator> Generator::from_default_parameters(const string& name)
{
    ptree emptyPtree;
    emptyPtree.put("generator.name", name);
    return Generator::from_parameters(emptyPtree);
}

// returns a generator using the parameters provided in the ptree. The remaining
// parameters use their default values. Note that params must at least contain
// the "name" property to identify the generator.
// params is non-const as the generator adds its additional default-values
// to this ptree
shared_ptr<Generator> Generator::from_parameters(const ptree& params)
{

    // Required parameter: name of the generator
    // if not available, we cannot lookup the generator instance and thus
    // cannot provide a Generator instance.
    string generator_name = "unitialized_name";

    try {
        generator_name = params.get<string>("generator.name");
    } catch (boost::property_tree::ptree_bad_path& e) {
        std::cerr << "Exception when trying to read the 'generator.name' field from the generator property tree: " << e.what() << std::endl;
        // in case this exception is caught, the next few lines will fail and raise a new exception
    }

    generators_t::const_iterator cit = generators().find(generator_name);
    if (cit == generators().end())
    {
        throw std::runtime_error("generator " + generator_name + " not registered -- probably need to include the corresponding cpp file.");
    }
    return generators().at(generator_name)(params);
}


const Generator::generators_t& Generator::generators() { return registry().get<generators_t>("generators");}


// class members
Generator::Generator(const ptree& parameters, const PropertyWriters& propertyWriters)
    : _parameters(parameters)
    , _propertyWriters(propertyWriters)
{}

Generator::~Generator() {}


// Note can't return as const as a common usecase is
// to open() the writers for actually writing to a file
PropertyWriters& Generator::propertyWriters()
{
    return _propertyWriters;
}

const ptree& Generator::parameters() const
{
    return _parameters;
}
} // namespace imdb

