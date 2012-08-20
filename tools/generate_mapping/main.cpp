/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>

#include <util/types.hpp>
#include <util/progress.hpp>

#include <io/cmdline.hpp>
#include <io/filelist.hpp>
#include <io/property_writer.hpp>



// ------------------------------------------------------------
// General usage
//
//    generate mapping in case each "object" is represented
//    by multiple images, e.g one .off file is represented as
//    many view images. The mapping file associates each
//    "view" with a "model".
//
//    generate_mapping -m modelsRootDir modelsFilelist -v viewsRootDir viewsFilelist -o modelsViewsMapping
//
// The mapping relies on the filesystem structure you must use for both the folder
// containing the 'models' as well as the folder containing the 'views':
//
// ------------------------------------------------------------

using namespace imdb;

class command_files : public Command
{
public:

    command_files()
        : Command("files [options]")
        , _co_models       ("models"       , "m", "filelist of models [required]")
        , _co_views        ("views"        , "v", "filelist of views [required]")
        , _co_outputfile   ("outputfile"   , "o", "output mapping filename [optional, if not provided, output is console]")
    {
        add(_co_models);
        add(_co_views);
        add(_co_outputfile);
    }

    bool run(const std::vector<std::string>& args)
    {
        string in_modellist;
        string in_viewlist;
        string in_outputfile;

        warn_for_unknown_option(args);

        if (!_co_models.parse_single<string>(args, in_modellist))
        {
            std::cerr << "generate_mapping: no models filelist provided." << std::endl;
            return false;
        }

        if (!_co_views.parse_single<string>(args, in_viewlist))
        {
            std::cerr << "generate_mapping: no views filelist provided." << std::endl;
            return false;
        }

        bool output_to_file = _co_outputfile.parse_single<string>(args, in_outputfile);



        FileList modelfilelist;
        FileList viewfilelist;

        try
        {
            modelfilelist.load(in_modellist);
            viewfilelist.load(in_viewlist);
        }
        catch (const std::exception& e)
        {
            std::cerr << "generate_mapping: failed to load filelist: " << e.what() << std::endl;
            return false;
        }

        progress_output progress;


        // build a map from model subdirectory (relative to the model
        // rootdirectory) to index in the model filelist
        std::map<string, int> map_model_index;
        for (size_t i = 0; i < modelfilelist.size(); i++)
        {
            boost::filesystem::path p(modelfilelist.get_relative_filename(i));
            string directory = p.parent_path().string();

            // make sure that a subdirectory contains exactly one model file, i.e. check
            // if the map already contains the subdirectory and fail if yes. Otherwise,
            // pu the subdirectory and the associated modellist index into the map
            std::map<string, int>::const_iterator cit = map_model_index.find(directory);
            if (cit == map_model_index.end())
            {
                map_model_index[directory] = i;
                progress(i, modelfilelist.size(), "generate_mapping: parsing model file list: ");
            }
            else
            {
                std::cerr << "generate_mapping: error, duplicate entry in model file list. Each sub directory must only contain exactly one model file." << std::endl;
                return false;
            }
        }



        // now go over each entry of the view filelist, extract the sub directory and associate
        // the view with the model lying in the same subdirectory in the models rootdir
        std::vector<index_t> map_view_model;
        for (size_t i = 0; i < viewfilelist.size(); i++)
        {
            boost::filesystem::path p(viewfilelist.get_relative_filename(i));
            string directory = p.parent_path().string();

            // for each directory in the views rootdir, we must have an associated
            // directory in the models rootdir, this  defines the mapping. So make
            // sure this is actually true and fail otherwise
            std::map<string, int>::const_iterator cit = map_model_index.find(directory);
            if (cit == map_model_index.end())
            {
                std::cerr << "generate_mapping: error, no corresponding model directory for this view: " << p << std::endl;
                return false;
            }

            int model_index = cit->second;
            map_view_model.push_back(model_index);
            progress(i, viewfilelist.size(), "generate_mapping: creating mapping: ");
        }

        if (output_to_file)
        {
            try { write_property(map_view_model, in_outputfile); }
            catch (const std::exception& e)
            {
                std::cerr << "generate_mapping: failed to write mapping file: " << e.what() << std::endl;
                return false;
            }
        }
        else
        {
            for (size_t i = 0; i < map_view_model.size(); i++)
            {
                std::cout << map_view_model[i] << std::endl;
            }
        }

        return true;
    }

private:

    CmdOption _co_models;
    CmdOption _co_views;
    CmdOption _co_outputfile;
};



int main(int argc, char *argv[])
{
    if (argc <= 1)
    {
        command_files().print();
        return 1;
    }

    return command_files().run(argv_to_strings(argc - 1, &argv[1])) ? 0 : 2;
}

