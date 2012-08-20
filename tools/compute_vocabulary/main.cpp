/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include <iostream>
#include <string>
#include <vector>

#include <util/types.hpp>
#include <util/kmeans.hpp>
#include <io/property_reader.hpp>
#include <io/property_writer.hpp>
#include <io/cmdline.hpp>
#include <search/distance.hpp>


using namespace imdb;

// We assume that we have a descriptor file that contains (in each element) a vec_vec_f32_t, i.e.
// a vector of local descriptors with each local descriptor being a vec_f32_t. Note that the
// descriptorfile does not give us an easy way to determine how many local features each element
// contains, we would need to read in each element to determine this. Therefore, we require to
// pass in an additional "size file", that holds the exact number of local features stored for each entry
// in the descriptor file.
void sampleWords(const std::string& descriptorFile, const std::string& sizeFile, size_t numSamples, imdb::vec_vec_f32_t& data)
{
    std::vector<int32_t> sizes;

    read_property(sizes, sizeFile);

    size_t num_local_features = std::accumulate(sizes.begin(), sizes.end(), 0);
    std::cout << "compute_vocabulary: descriptor file contains " << num_local_features << " words" << std::endl;

    // limit the number of samples to the number of words in the file
    // in order to be sure not read over the end of the file
    numSamples = std::min(numSamples, num_local_features);

    std::cout << "compute_vocabulary: creating samples" << std::endl;

    // create as many random samples as desired by the user
    vector<pair<int, int> > samples(num_local_features);
    size_t linear_index = 0;
    for (size_t i = 0; i < sizes.size(); i++) {
        for (int j = 0; j < sizes[i]; j++) {
            samples[linear_index] = make_pair(j, i);
            linear_index++;
        }
    }

    // randomize and cut off at the desired size
    srand(time(0));
    std::random_shuffle(samples.begin(), samples.end());
    samples.resize(numSamples);


    map<int, vector<int> > map_featureid_sampleids;
    for (size_t i = 0; i < samples.size(); i++) {
        int sample_id = samples[i].first;
        int feature_id = samples[i].second;
        map_featureid_sampleids[feature_id].push_back(sample_id);
    }


    std::cout << "compute_vocabulary: extracting samples from descriptor file" << std::endl;


    PropertyReaderT<vec_vec_f32_t> reader(descriptorFile);

    std::cout << "compute_vocabulary: reading " << (map_featureid_sampleids.size() / static_cast<float>(reader.size()))*100 << "% of all features to gather desired number of samples."  << std::endl;

    map<int, vector<int> >::const_iterator cit;

    for (cit = map_featureid_sampleids.begin(); cit != map_featureid_sampleids.end(); ++cit) {

        int feature_id = cit->first;
        vec_vec_f32_t feature;
        reader.get(feature, feature_id);

        const vector<int>& sample_ids = cit->second;
        for (size_t i = 0; i < sample_ids.size(); i++) {
            int sample_id = sample_ids[i];
            data.push_back(feature[sample_id]);
        }
    }

    assert(data.size() == numSamples);

    std::cout << "compute_vocabulary: done, data contains " << data.size() << " samples." << std::endl;
}

void readAllWords(const std::string& descriptorFile, imdb::vec_vec_f32_t& data)
{
    std::cout << "compute_vocabulary: extracting samples from descriptor file..." << std::endl;

    PropertyReaderT<vec_vec_f32_t> reader(descriptorFile);

    // now extract exactly only those "words" from the feature vector
    // that correspond to the sample indices
    int currWordIndex = 0;
    for (index_t i = 0; i < reader.size(); i++)
    {
        vec_vec_f32_t feature = reader[i];
        for (size_t j = 0; j < feature.size(); j++)
        {
            data.push_back(feature[j]);
            currWordIndex++;
        }
    }

    std::cout << "compute_vocabulary: done, data contains " << data.size() << " samples." << std::endl;
}

class command_compute : public Command
{
public:

    command_compute()
        : Command("compute_vocabulary [options]")
        , _co_descfile  ("descfile"         , "d", "descriptors file [required]")
        , _co_numclusters("numclusters"     , "c", "number of clusters/visual words to generate [required]")
        , _co_outputfile("outputfile"       , "o", "output file [required]")
        , _co_sizefile  ("sizefile"         , "s", "file that contains number of words per descriptor [optional]")
        , _co_numsamples("numsamples"       , "n", "number of words randomly extracted from descriptor file [optional, but sizefile must also be specified]")
        , _co_numthreads("numthreads"       , "t", "number of threads for parallel computation (default: number of processors) [optional]")
        , _co_maxiter   ("maxiter"          , "i", "kmeans stopping criterion: maximum number of iterations (default: 20) [optional]")
        , _co_minchangesfraction("minchangesfraction" , "m", "kmeans stopping criterion: number of changes (fraction of total samples) (default: 0.01) [optional]")
    {
        add(_co_descfile);
        add(_co_sizefile);
        add(_co_numsamples);
        add(_co_numclusters);
        add(_co_outputfile);
        add(_co_numthreads);
        add(_co_maxiter);
        add(_co_minchangesfraction);
    }


    bool run(const std::vector<std::string>& args)
    {
        if (args.size() == 0)
        {
            print();
            return false;
        }

        warn_for_unknown_option(args);

        string in_descfile;
        string in_sizefile;
        string in_outputfile;
        int in_numsamples;
        int in_numclusters;

        // number of threads to be used: by default we use as many
        // as we have processors on the machine, otherwise use
        // user-provided parameter.
        int in_numthreads = boost::thread::hardware_concurrency();
        if (_co_numthreads.parse_single<int>(args, in_numthreads))
        {
            if (in_numthreads < 1)
            {
                std::cout << "compute_vocabulary: number of threads should be > 0, using default" << std::endl;
                in_numthreads = boost::thread::hardware_concurrency();
            }
        }

        std::cout << "compute_vocabulary: using " << in_numthreads << " threads" << std::endl;

        if (!_co_descfile.parse_single<string>(args, in_descfile)
                || !_co_outputfile.parse_single<string>(args, in_outputfile)
                || !_co_numclusters.parse_single<int>(args, in_numclusters))
        {
            print();
            return false;
        }

        int in_maxiter = 20;
        double in_minchangesfraction = 0.01;

        _co_maxiter.parse_single<int>(args, in_maxiter);
        _co_minchangesfraction.parse_single<double>(args, in_minchangesfraction);

        bool has_sizefile = _co_sizefile.parse_single<string>(args, in_sizefile);
        bool has_numsamples = _co_numsamples.parse_single<int>(args, in_numsamples);

        // incorrect usage by definition: user have to specify both parameters or none of them
        // this might be slightly inconvenient for some usecases but typically the
        // required size-file is automatically created with all local feature generators
        if (has_sizefile != has_numsamples)
        {
            print();
            return false;
        }

        vec_vec_f32_t samples;
        if (has_sizefile && has_numsamples)
        {
            try { sampleWords(in_descfile, in_sizefile, in_numsamples, samples); }
            catch (const std::exception& e)
            {
                std::cerr << "compute_vocabulary: failed to read samples: " << e.what() << std::endl;
                return false;
            }
        }
        else
        {
            try { readAllWords(in_descfile, samples); }
            catch (const std::exception& e)
            {
                std::cerr << "compute_vocabulary: failed to read samples: " << e.what() << std::endl;
                return false;
            }
        }

        std::cout << "compute_vocabulary: clustering" << std::endl;

        // cluster the data
        vec_vec_f32_t centers;
        typedef kmeans<vec_vec_f32_t, l2norm_squared<vec_f32_t> > cluster_fn;
        cluster_fn clusterfn(samples, in_numclusters);
        clusterfn.run(in_maxiter, in_minchangesfraction);
        centers = clusterfn.centers();

        // write the resulting cluster centers
        try { write_property(centers, in_outputfile); }
        catch (const std::exception& e)
        {
            std::cerr << "compute_vocabulary: failed to write vocabulary: " << e.what() << std::endl;
            return false;
        }

        std::cout << "compute_vocabulary: writing resulting centers to output file " << in_outputfile << std::endl;

        return true;
    }

private:

    CmdOption _co_descfile;
    CmdOption _co_numclusters;
    CmdOption _co_outputfile;
    CmdOption _co_sizefile;
    CmdOption _co_numsamples;
    CmdOption _co_numthreads;
    CmdOption _co_maxiter;
    CmdOption _co_minchangesfraction;
};

int main(int argc, char **argv)
{
    command_compute cmd;

    bool okay = cmd.run(argv_to_strings(argc-1, &argv[1]));

    return okay ? 0:1;
}
