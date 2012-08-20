/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#ifndef FILELIST_H
#define FILELIST_H

#include "../util/types.hpp"

namespace imdb {

/**
 * @ingroup IO
 * \brief Holds a vector of filenames relative to a given root directory.
 *
 * A FileList is a vector of filenames and thus determines a certain order
 * of the files that is now completely independent of the operating system and
 * filesystem. Other tools in our framework (e.g. compute_descriptors) rely on
 * this order and store the feature vectors of the corresponding images in exactly
 * the same order. This lets you determine the image filename from its feature id.
 *
 * This class has two main use cases:
 *
 * -# Listing all files with a given file-ending in and below rootdir.
 * Note: all subdirectories below root directory are parsed recursively.
 * -# Loading/saving/accessing//subsampling a previously generated list of filenames.
 */
class FileList
{
    public:

    typedef boost::function<void (int, const string&)> callback_fn;

    /// Constructor, pass in the desired root directory.
    /// The directory passed in must be a valid, existing directory
    FileList(const string& root_dir = ".");

    /// Alternative way to set/change the root directory
    void set_root_dir(const string& root_dir);

    /// List all files found in and below rootdir that have one of the file-endings specified
    /// in 'namefilter'. Each entry of the vector contains a string such as "*.png" or "*.jpg"
    void lookup_dir(const vector<string>& namefilters, callback_fn callback = callback_fn());

    /// Save a FileList. Note that the root directory is not stored, only
    /// the list of filenames relative to the root directory.
    /// @throw std::runtime_error thrown when filename can not be opened for writing
    void store(const string& filename) const;

    /// Load a FileList from harddisk.
    /// @throw std::runtime_error thrown when the given file does not exist/could not be opened
    void load(const string& filename);


    /// Subsample given filelist randomly
    void random_sample(size_t new_size, size_t seed);

    /// Returns the current root directory
    const string& root_dir() const;

    /// Number of files
    size_t size() const;

    /// Access relative filename of file i
    /// Note that we do not perform range checking on the index. Index must
    /// be in range [0, size()-1], otherwise an access error will occur
    const string& get_relative_filename(size_t index) const;

    /// Access 'absolute' filename of file i, i.e. root_dir + '/' + get_relative_filename(i)
    /// Note that we do not perform range checking on the index. Index must
    /// be in range [0, size()-1], otherwise an access error will occur.
    string get_filename(size_t index) const;

    /// Vector of all filenames
    const vector<string>& filenames() const;

    private:

    string         _rootdir;
    vector<string> _files;
};

} // end namespace

#endif // FILELIST_H
