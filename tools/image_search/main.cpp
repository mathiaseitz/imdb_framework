/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include <iostream>

#include <boost/algorithm/string.hpp>

#include <opencv2/highgui/highgui.hpp>

#include <util/types.hpp>
#include <util/quantizer.hpp>
#include <io/property_reader.hpp>
#include <io/cmdline.hpp>
#include <io/filelist.hpp>
#include <descriptors/generator.hpp>
#include <search/linear_search.hpp>
#include <search/bof_search_manager.hpp>
#include <search/linear_search_manager.hpp>
#include <search/distance.hpp>

using namespace imdb;

template <class search_t>
void image_search(const anymap_t& data, const search_t& search, size_t num_results, vector<dist_idx_t>& results)
{
    typedef typename search_t::descr_t descr_t;
    descr_t descr = get<descr_t>(data, "features");
    search.query(descr, num_results, results);
}



class command_search : public Command
{
public:

    command_search()
        : Command("image_search [options]")
        , _co_query_image("queryimage"        , "q", "filename of image to be used as the query [required]")
        , _co_search_ptree("searchptree"      , "s", "filename of the JSON file containing parameters for the search manager [optional, if not provided, --searchparams must be given]")
        , _co_search_params("searchparams"    , "m", "parameters for the search manager [optional, if not provided, --searchptree must be given]")
        , _co_vocabulary("vocabulary"         , "v", "filename of vocabulary used for quantization [optional, only required with bag-of-features search]")
        , _co_filelist("filelist"             , "l", "filename of images filelist [required]")
        , _co_generator_name("generatorname"  , "g", "name of generator [optional, if given, we will use generator's default parameters and ignore --generatorptree]")
        , _co_generator_ptree("generatorptree", "p", "filename of the JSON file containing generator name and parameters [optional, if not provided, generator's default values are used']")
        , _co_num_results  ("numresults"      , "n", "number of results to search for [optional, if not provided all distances get computed]")

    {
        add(_co_query_image);
        add(_co_search_ptree);
        add(_co_search_params);
        add(_co_vocabulary);
        add(_co_filelist);
        add(_co_generator_ptree);
        add(_co_num_results);
        add(_co_generator_name);
    }


    bool run(const std::vector<std::string>& args)
    {
        if (args.size() == 0)
        {
            print();
            return false;
        }

        warn_for_unknown_option(args);


        string in_queryimage;
        string in_searchptree;
        string in_filelist;
        string in_generatorptree;
        string in_generatorname;
        string in_vocabulary;

        // this default value will make the search managers search
        // for all images if the user does not provide a value
        size_t in_numresults = std::numeric_limits<size_t>::max();


        // check that the required options are available
        if (!_co_query_image.parse_single<string>(args, in_queryimage) || !_co_filelist.parse_single<string>(args, in_filelist))
        {
            print();
            return false;
        }


        // -----------------------------------------------------------------------------------
        // either searchptree or searchparams must be given
        ptree search_params;
        vector<string> in_searchparams;
        if (_co_search_ptree.parse_single<string>(args, in_searchptree))
        {
            boost::property_tree::read_json(in_searchptree, search_params);
        }
        else if (_co_search_params.parse_multiple<string>(args, in_searchparams))
        {
            for (size_t i = 0; i < in_searchparams.size(); i++)
            {
                vector<string> pv;
                boost::algorithm::split(pv, in_searchparams[i], boost::algorithm::is_any_of("="));

                if (pv.size() != 1 && pv.size() != 2)
                {
                    std::cerr << "image_search: cannot parse search manager parameter: " << in_searchparams[i] << std::endl;
                    return false;
                }
                search_params.put(pv[0], (pv.size() == 2) ? pv[1] : "");
            }

        }
        else
        {
            print();
            return false;
        }
        // -----------------------------------------------------------------------------------



        // try to parse the optional num_results parameter
        _co_num_results.parse_single<size_t>(args, in_numresults);

        // -----------------------------------------------------------------
        // create the generator; we have the following rule:
        // a) if the user provides a generator name, we use this and ignore an additional generator ptree
        // b) if no generator name but ptree is provided, we use this
        shared_ptr<Generator> gen;
        if (_co_generator_name.parse_single<string>(args, in_generatorname))
        {
            gen = Generator::from_default_parameters(in_generatorname);
        }
        else if (_co_generator_ptree.parse_single<string>(args, in_generatorptree))
        {
            gen = Generator::from_parameters_file(in_generatorptree);
        }
        else
        {
            std::cerr << "image_search: must provide either generator name or ptree" << std::endl;
            std::cerr << "received args: generatorname=" << in_generatorname << "; generatorptree=" << in_generatorptree << std::endl;

            print();
            return false;
        }
        // -----------------------------------------------------------------


        FileList imageFiles;
        imageFiles.load(in_filelist);




        mat_8uc3_t image = cv::imread(in_queryimage, 1);
        anymap_t data;
        data["image"] = image;

        gen->compute(data);

        vector<dist_idx_t> results;


        if (search_params.get<std::string>("search_type") == "BofSearch")
        {

            // parameter --vocabulary must be given
            if (!_co_vocabulary.parse_single<string>(args, in_vocabulary))
            {
                std::cerr << "image_search: when using bag-of-features search, you must also provide the --vocabulary commandline option" << std::endl;
                print();
                return false;
            }


            vec_vec_f32_t vocabulary;
            read_property(vocabulary, in_vocabulary);

            // quantize
            quantize_fn quantizer = quantize_hard<vec_f32_t, imdb::l2norm_squared<vec_f32_t> >();
            vec_vec_f32_t quantized_samples;

            const vec_vec_f32_t& samples = boost::any_cast<vec_vec_f32_t>(data["features"]);
            quantize_samples_parallel(samples, vocabulary, quantized_samples, quantizer);

            vec_f32_t histvw;
            build_histvw(quantized_samples, vocabulary.size(), histvw, false);

            // initialize search manager and run query
            BofSearchManager bofSearch(search_params);
            bofSearch.query(histvw, in_numresults, results);
        }

        else if (search_params.get<std::string>("search_type") == "LinearSearch")
        {
            // Tensor descriptor is a bit of a special case as we additionally
            // need to pass a 'mask' to the distance function, so we replicate
            // most of the functionality implemented in LinearSearchManager
            if (gen->parameters().get<string>("name") == "tensor")
            {
                string filename = search_params.get<string>("descriptor_file");
                vec_vec_f32_t features;
                read_property(features, filename);

                const vec_f32_t& descr = get<vec_f32_t>(data, "features");
                const vector<bool>& mask = get<vector<bool> >(data, "mask");
                std::cout << "mask size=" << mask.size() << std::endl;
                dist_frobenius<vec_f32_t> distfn;
                distfn.mask = &mask;
                linear_search(descr, features, results, in_numresults, distfn);

            }
            else
            {
                LinearSearchManager search(search_params);
                image_search(data, search, in_numresults, results);
            }
        }
        else
        {
            std::cerr << "unsupported search type" << std::endl;
        }



        // output results on the console
        // use piping to store in a text file (for now)
        for (size_t i = 0; i < results.size(); i++) {
            string filename = imageFiles.get_relative_filename(results[i].second);
            std::cout << i << " " << results[i].first << " " << filename  << std::endl;
        }


        return true;
    }

private:

    CmdOption _co_query_image;
    CmdOption _co_search_ptree;
    CmdOption _co_search_params;
    CmdOption _co_vocabulary;
    CmdOption _co_filelist;
    CmdOption _co_generator_name;
    CmdOption _co_generator_ptree;
    CmdOption _co_num_results;
};



int main(int argc, char *argv[])
{

    command_search cmd;
    bool okay = cmd.run(argv_to_strings(argc-1, &argv[1]));
    return okay ? 0:1;
}
