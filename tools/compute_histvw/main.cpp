/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include <QTime>

#include <iostream>
#include <algorithm>

#include <util/types.hpp>
#include <util/progress.hpp>
#include <util/quantizer.hpp>

#include <io/property_reader.hpp>
#include <io/property_writer.hpp>
#include <io/cmdline.hpp>

#include <search/distance.hpp>



using namespace imdb;

class command_compute : public Command
{
public:

    command_compute()
        : Command("image_search [options]")
        , _co_vocabulary("vocabulary"        , "v", "filename of the vocabulary to be used for quantization [required]")
        , _co_descriptors("descriptors"      , "d", "filename of the descriptors to convert into histograms of visual words [required]")
        , _co_positions("positions"          , "p", "positions data for features [required]")
        , _co_quantization("quantization"    , "q", "quantization method {hard,fuzzy} [required]")
        , _co_sigma("sigma"                  , "s", "sigma for gaussian weighting in fuzzy quantization [required (with 'fuzzy' quantization only)]")
        , _co_output("output"                , "o", "filename of the output file of histograms of visual words [required]")
        , _co_pyramidlevels("pyramidlevels"  , "l", "number of spatial pyramid levels [optional, default 1]")
    {
        add(_co_vocabulary);
        add(_co_descriptors);
        add(_co_positions);
        add(_co_output);
        add(_co_quantization);
        add(_co_sigma);
        add(_co_pyramidlevels);
    }


    bool run(const std::vector<std::string>& args)
    {
        if (args.size() == 0)
        {
            print();
            return false;
        }

        warn_for_unknown_option(args);


        string in_vocabulary;
        string in_descriptors;
        string in_positions;
        string in_quantization;
        string in_output;
        float  in_sigma;

        // Default values if no command line parameters are provided
        // Those default parameters essentially mean no spatial
        // subdivision of the histogram of visual words (i.e. creation of
        // an original bag-of-features histogram
        size_t in_pyramidlevels = 1;

        // check that the required options are available
        if (!_co_vocabulary.parse_single<string>(args, in_vocabulary)
                || !_co_descriptors.parse_single<string>(args, in_descriptors)
                || !_co_positions.parse_single<string>(args, in_positions)
                || !_co_quantization.parse_single<string>(args, in_quantization)
                || !_co_output.parse_single<string>(args, in_output))
        {
            print();
            return false;
        }


        // make sure that the qunatization option provided is either
        // "fuzzy" or "hard"; provide a warning otherwise
        if ((in_quantization != "fuzzy") && (in_quantization != "hard"))
        {
            std::cerr << "compute_histvw: quantization method can only be {'fuzzy', 'hard'}. You provided: '" << in_quantization << "'. Exiting." << std::endl;
            return false;
        }

        // we require that sigma is also provided when
        // fuzzy clustering has been selected
        if (in_quantization == "fuzzy")
        {
            if (!_co_sigma.parse_single<float>(args, in_sigma))
            {
                std::cerr << "compute_histvw: you must provide a value for 'sigma' when selecting 'fuzzy' quantization" << std::endl;
                print();
                return false;
            }
        }


        // check for optional arguments
        _co_pyramidlevels.parse_single<size_t>(args, in_pyramidlevels);

        // ----------------------------------------------
        // we now have parse all relevant commandline
        // parameters and are ready to compute....
        // ----------------------------------------------

        vec_vec_f32_t vocabulary;

        try
        {
            read_property(vocabulary, in_vocabulary);
        }
        catch (const std::exception& e)
        {
            std::cerr << "compute_histvw: failed to read data: " << e.what() << std::endl;
            return false;
        }

        typedef l2norm_squared<vec_f32_t> dist_fn_t;



        quantize_fn quantizer; // boost::function, see quantizer.hpp for typedef
        bool normalizeHistvw;

        if (in_quantization == "fuzzy")
        {
            std::cout << "compute_histvw: using fuzzy clustering, sigma=" << in_sigma << std::endl;
            quantizer = quantize_fuzzy<vec_f32_t, dist_fn_t>(in_sigma);
            normalizeHistvw = true;
        }
        else if (in_quantization == "hard")
        {
            std::cout << "compute_histvw: using hard clustering" << std::endl;
            quantizer = quantize_hard<vec_f32_t, dist_fn_t>();
            normalizeHistvw = false;
        }



        try {
            PropertyWriterT<vec_f32_t> writer(in_output);
            PropertyReaderT<vec_vec_f32_t> reader_desc(in_descriptors);
            PropertyReaderT<vec_vec_f32_t> reader_pos(in_positions);

            assert(reader_desc.size() == reader_pos.size());
            std::cout << "compute_histvw: reader #entries=" << reader_desc.size() << std::endl;

            progress_output progress(10);
            for (index_t i = 0; i < reader_desc.size(); i++)
            {
                vec_vec_f32_t samples(reader_desc[i]);
                vec_vec_f32_t positions(reader_pos[i]);

                // quantize all samples contained in the current vec_vec_f32_t in parallel, the
                // result is again a vec_vec_f32_t which has the same size as the samples vector,
                // i.e. one quantized sample for each original sample.
                vec_vec_f32_t quantized_samples;
                quantize_samples_parallel(samples, vocabulary, quantized_samples, quantizer);

                vec_f32_t hist;

                for (size_t j = 0; j < in_pyramidlevels; j++)
                {
                    vec_f32_t tmp;
                    int res = 1 << j; // 2^j
                    build_histvw(quantized_samples, vocabulary.size(), tmp, normalizeHistvw, positions, res);

                    // append the current pyramid level histograms to
                    // the overall histogram
                    hist.insert(hist.end(), tmp.begin(), tmp.end());
                }

                progress(i, reader_desc.size(), "compute_histvw progress: ");

                writer.push_back(hist);
            }
        }

        // catch (most probably) i/o exceptions that occur when the user messed up a path/filename
        catch (const std::exception& e)
        {
            std::cerr << "compute_histw: failed to read/write data: " << e.what() << std::endl;
            return false;
        }

        std::cout << "compute_histvw: done" << std::endl;

        return true;
    }

private:

    CmdOption _co_vocabulary;
    CmdOption _co_descriptors;
    CmdOption _co_positions;
    CmdOption _co_quantization;
    CmdOption _co_sigma;
    CmdOption _co_output;
    CmdOption _co_pyramidlevels;
};


int main(int argc, char *argv[])
{
    command_compute cmd;
    bool okay = cmd.run(argv_to_strings(argc-1, &argv[1]));
    return okay ? 0:1;
}
