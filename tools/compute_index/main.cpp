/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include <iostream>
#include <algorithm>

#include <QTime>

#include <util/types.hpp>
#include <util/progress.hpp>
#include <util/quantizer.hpp>

#include <io/property_reader.hpp>
#include <io/cmdline.hpp>

#include <search/distance.hpp>
#include <search/inverted_index.hpp>
#include <search/tf_idf.hpp>



#include <util/kmeans.hpp>


using namespace imdb;

class command_compute : public Command
{
public:

    command_compute()
        : Command("compute_index [options]")
        , _co_histvwfile("histvw"            , "h", "filename to vector of histograms of visual words [required]")
        , _co_output("output"                , "o", "filename of the output index file [required]")
        , _co_tfidf("tfidf"                  , "t", "two strings specifying tf and idf function to be used [required]")
    {
        add(_co_histvwfile);
        add(_co_output);
        add(_co_tfidf);
    }


    bool run(const std::vector<std::string>& args)
    {

        warn_for_unknown_option(args);

        string in_histvw;
        string in_output;
        vector<string> in_tfidf;

        // check that the required options are available
        if (!_co_histvwfile.parse_single<string>(args, in_histvw) ||
            !_co_output.parse_single<string>(args, in_output) ||
            !_co_tfidf.parse_multiple<string>(args, in_tfidf) ||
            !in_tfidf.size() == 2)
        {
            print();
            return false;
        }


        std::cout << "compute_index: tf=" << in_tfidf[0] << ", idf=" << in_tfidf[1] << std::endl;

        shared_ptr<tf_function>  tf = make_tf(in_tfidf[0]);
        shared_ptr<idf_function> idf = make_idf(in_tfidf[1]);

        QTime total;
        total.start();



        try {
            PropertyReaderT<vec_f32_t> reader(in_histvw);

            std::cout << "compute_index: histvw file contains a total of " << reader.size() << " histograms." << std::endl;

            // what we expect is that histograms in the file have exactly this size!
            int vocabSize = reader[0].size();
            assert(vocabSize > 0);

            InvertedIndex index(vocabSize);

            progress_output progress;
            for (index_t i = 0; i < reader.size(); i++)
            {
                index.addHistogram(reader[i]);
                progress(i, reader.size(), "compute_index progress: ");
            }

            std::cout << "compute_index: finalizing" << std::endl;
            index.finalize(index, *tf, *idf);
            //index.apply_tfidf(index, *tf, *idf);
            std::cout << "compute_index: saving" << std::endl;
            index.save(in_output);
        }
        catch (const std::exception& e)
        {
            std::cerr << "compute_index: error: " << e.what() << std::endl;
            return false;
        }


        std::cout << "compute_index: done." << std::endl;
        int totalMSElapsed = total.elapsed();
        std::cout << "compute_index: total time: " << (totalMSElapsed / 1000) << "s" << std::endl;

        return true;
    }

private:

    CmdOption _co_histvwfile;
    CmdOption _co_output;
    CmdOption _co_tfidf;
};


int main(int argc, char *argv[])
{
    command_compute cmd;
    bool okay = cmd.run(argv_to_strings(argc-1, &argv[1]));
    return okay ? 0:1;
}
