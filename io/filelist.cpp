/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include "filelist.hpp"

#include <QDir>
#include <QDirIterator>

#include <boost/random.hpp>
#include <boost/assert.hpp>

#include "property_reader.hpp"
#include "property_writer.hpp"


namespace imdb {

FileList::FileList(const std::string& image_dir)
{
    set_root_dir(image_dir);
}

const std::string& FileList::root_dir() const
{
    return _rootdir;
}

void FileList::set_root_dir(const std::string& image_dir)
{
    // make sure our precondition holds
    QDir dir(QString::fromStdString(image_dir));
    if (!dir.exists())
    {
        throw std::runtime_error("FileList rootdir <" + image_dir + "> does not exist.");
    }
    _rootdir = image_dir;
}

size_t FileList::size() const
{
    return _files.size();
}

std::string FileList::get_filename(size_t index) const
{

    return _rootdir + "/" + get_relative_filename(index);
}

const string &FileList::get_relative_filename(size_t index) const
{
    BOOST_ASSERT(index < _files.size());
    return _files[index];
}

const vector<string>& FileList::filenames() const
{
    return _files;
}

void FileList::lookup_dir(const vector<string>& namefilters, callback_fn callback)
{
    QStringList qnamefilters;
    for (size_t i = 0; i < namefilters.size(); i++)
    {
        qnamefilters.push_back(QString::fromStdString(namefilters[i]));
    }

    std::vector<std::string> files;

    QDir root(_rootdir.c_str());

    // NOTE: we pass an absolut path to the QDirIterator;
    //       without that the relative path resolution doesn't work
    QDirIterator it(root.absolutePath(),
                    qnamefilters,
                    QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);

    for (size_t i = 0; it.hasNext(); i++)
    {
        QString p = it.next();

        // NOTE: relative path resolution works only with absolut paths as input argument
        QString r = root.relativeFilePath(p);
        files.push_back(r.toStdString());
        if (callback) callback(i, "");
    }

    _files = files;
}

void FileList::load(const std::string& filename)
{
    // as the call below may fail, we pass in a temporary and
    // copy over the results only when no excpetion occurred
    std::vector<std::string> files;

    // possibly throws an exception that needs to be caught
    // in the application using this function
    read_property(files, filename);

    _files = files;
}

void FileList::store(const std::string& filename) const
{
   // possibly throws an exception that needs to be caught
   // in the application using this function
    write_property(_files, filename);
}

void FileList::random_sample(size_t new_size, size_t seed)
{
    if (new_size >= _files.size()) return;

    std::vector<size_t> indices(_files.size());
    for (size_t i = 0; i < indices.size(); i++) indices[i] = i;

    typedef boost::mt19937 rng_t;
    typedef boost::uniform_int<size_t> uniform_t;
    rng_t rng(seed);
    boost::variate_generator<rng_t&, uniform_t> random(rng, uniform_t(0, indices.size() - 1));

    std::random_shuffle(indices.begin(), indices.end(), random);
    indices.resize(new_size);
    std::sort(indices.begin(), indices.end());

    std::vector<std::string> new_files(new_size);
    for (size_t i = 0; i < new_files.size(); i++) new_files[i] = _files[indices[i]];
    _files = new_files;
}
}

