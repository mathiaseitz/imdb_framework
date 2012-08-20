/*
Copyright (C) 2012 Mathias Eitz and Ronald Richter.
All rights reserved.

This file is part of the imdb library and is made available under
the terms of the BSD license (see the LICENSE file).
*/

#include <QDateTime>
#include <QTime>

#include <iostream>
#include <queue>
#include <stdexcept>

#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>

#include <io/property_writer.hpp>
#include <io/cmdline.hpp>
#include <io/filelist.hpp>
#include <io/compute_descriptors.hpp>
#include <util/progress.hpp>
#include <util/types.hpp>
#include <descriptors/generator.hpp>



using namespace imdb;


void progress_observer(const ComputeDescriptors& cd)
{
    size_t lastindex = 0;
    int running_sum_time = 0;
    int running_sum_processed = 0;

    QTime lasttime;
    std::queue<pair<int,int> > queue;

    // make the number of running samples we
    // compute the average from reasonably large
    // such that outliers do not strongly influence
    // the average (better use the median)
    std::size_t q_maxsize = 100;

    bool firstrun = true;
    while (!cd.finished())
    {
        size_t index = cd.current();
        int d_processed = index - lastindex;

        if (!firstrun && d_processed > 0)
        {
            int d_time = lasttime.elapsed();

            queue.push(std::make_pair(d_time, d_processed));
            running_sum_time += d_time;
            running_sum_processed += d_processed;

            if (queue.size() > q_maxsize)
            {
                running_sum_time -= queue.front().first;
                running_sum_processed -= queue.front().second;
                queue.pop();
            }

            assert(queue.size() > 0);
            assert(running_sum_processed > 0);
        }

        std::cout << "compute: " << index << "/" << cd.num_files();
        if (queue.size() > 0)
        {
            int msecdescr = running_sum_time / static_cast<float>(running_sum_processed);
            int etaseconds = (cd.num_files() - index) * msecdescr / 1000;
            int fmth = etaseconds / 3600;
            int fmtm = etaseconds / 60 % 60;
            int fmts = etaseconds % 60;
            std::cout << ", ms/descriptor: " << msecdescr << ", eta: " << fmth << ":" << fmtm << ":" << fmts;
        }

        std::cout << "            \r" << std::flush;

        lasttime.start();
        firstrun = false;
        lastindex = index;

        boost::this_thread::sleep(boost::posix_time::seconds(3));
    }

    std::cout << std::endl;
}

void print_available_generators()
{
    std::cout << "available generators:" << std::endl;

    Generator::generators_t::const_iterator it;
    for (it = Generator::generators().begin(); it != Generator::generators().end(); ++it)
    {
        std::cout << "* " << it->first << std::endl;
    }
}

class command_compute : public Command
{
public:

    command_compute()
        : Command("compute <generator> [options]")
        , _co_rootdir   ("rootdir"          , "r", "root directory of data descriptors are computed from [required]")
        , _co_filelist  ("filelist"         , "f", "file that contains filenames of data (images/models) [required]")
        , _co_output    ("output"           , "o", "output prefix [required]")
        , _co_params    ("parameters"       , "p", "parameters for generator construction [optional] (default: params defined in generator)")
        , _co_numthreads("numthreads"       , "t", "number of threads for parallel computation [optional] (default: number of processors)")

    {
        add(_co_rootdir);
        add(_co_filelist);
        add(_co_output);
        add(_co_params);
        add(_co_numthreads);
    }


    bool run(const std::vector<std::string>& args)
    {
        if (args.size() == 0)
        {
            print();
            return false;
        }

        warn_for_unknown_option(args);

        std::string in_generator(args[0]);

        if (!Generator::generators().count(in_generator))
        {
            std::cerr << "compute_descriptors: no generator named " << in_generator << std::endl;
            print_available_generators();
            return false;
        }


        std::string in_rootdir;
        std::string in_filelist;
        std::string in_output;
        std::vector<std::string> in_params;


        // ----------------------------------------------------------------------------------
        // number of threads to be used: by default we use as many
        // as we have processors on the machine, otherwise use
        // user-provided parameter.
        int in_numthreads = boost::thread::hardware_concurrency();
        if (_co_numthreads.parse_single<int>(args, in_numthreads)) {
            if (in_numthreads < 1) {
                std::cout << "compute_descriptors: number of threads should be > 0, using default" << std::endl;
                in_numthreads = boost::thread::hardware_concurrency();
            }
        }
        std::cout << "compute_descriptors: using " << in_numthreads << " threads" << std::endl;
        // ------------------------------------------------------------------------------------

        if (!_co_rootdir.parse_single<std::string>(args, in_rootdir)
                || !_co_output.parse_single<std::string>(args, in_output))
        {
            print();
            return false;
        }

        _co_params.parse_multiple<std::string>(args, in_params);


        ptree params;
        params.put("generator.name", in_generator);
        for (size_t i = 0; i < in_params.size(); i++)
        {
            std::vector<std::string> pv;
            boost::algorithm::split(pv, in_params[i], boost::algorithm::is_any_of("="));

            if (pv.size() != 1 && pv.size() != 2)
            {
                std::cerr << "compute_descriptors: cannot parse descriptor parameter: " << in_params[i] << std::endl;
                return false;
            }
            params.put(pv[0], (pv.size() == 2) ? pv[1] : "");
        }

        FileList files;
        try { files.set_root_dir(in_rootdir); }
        catch (const std::exception& e)
        {
            std::cerr << "Exception in compute_descriptors: " << e.what() << std::endl;
            return false;
        }

        if (!_co_filelist.parse_single<std::string>(args, in_filelist))
        {
            print();
            return false;
        }
        else
        {
            try { files.load(in_filelist); }
            catch(std::exception& e)
            {
                std::cerr << "compute_descriptors: failed to load filelist from file " << in_filelist << ": " << e.what() << std::endl;
                return false;
            }
        }

        // Create generator. Typical failure case for this
        // is that the JSON parameters contains an unregistered
        // name for a Generator/ImageSampler
        boost::shared_ptr<Generator> generator;
        try
        {
            generator = Generator::from_parameters(params);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            return false;
        }

        // initialize a computing object
        ComputeDescriptors cd(generator, files);

        // add writers for properties offered by the generator
        PropertyWriters::properties_t& propertyWriters = generator->propertyWriters().get();
        PropertyWriters::properties_t::const_iterator cit = propertyWriters.begin();

        for(; cit != propertyWriters.end(); ++cit)
        {
            const std::string& name = cit->first;
            string filename = in_output + name;

            try { cit->second->open(filename); }
            catch (const std::exception& e)
            {
                std::cerr << "compute_descriptors: failed to open property writer on file " << filename << ": " << e.what() << std::endl;

                // We could continue here, but it seems better to fail and end the program
                // instead doing a long computation for which we cannot store the result
                return false;
            }

            cd.add_writer(name, cit->second);
        }

        // start computing descriptors
        QDateTime time = QDateTime::currentDateTime();

        boost::thread obs(progress_observer, boost::ref(cd));

        bool okay = cd.start(in_numthreads);

        int seconds = time.secsTo(QDateTime::currentDateTime());
        obs.join();

        int fmth = seconds / 3600;
        int fmtm = seconds / 60 % 60;
        int fmts = seconds % 60;
        std::cout << "finished." << std::endl;
        std::cout << "duration: " << fmth << "h " << fmtm << "m " << fmts << "s" << " (" << seconds << " s)" << std::endl;

        string filename = in_output + "parameters";
        boost::property_tree::write_json(filename, generator->parameters());


        if (!okay)
        {
            std::cerr << "compute_descriptors: error during computation occured" << std::endl;
        }

        return okay;
    }

private:

    CmdOption _co_rootdir;
    CmdOption _co_filelist;
    CmdOption _co_output;
    CmdOption _co_params;
    CmdOption _co_numthreads;
};

class command_info : public Command
{
public:

    command_info()
        : Command("info <generator>")
    {}

    bool run(const std::vector<std::string>& args)
    {
        if (args.size() == 0)
        {
            print();
            return false;
        }

        warn_for_unknown_option(args);

        std::string in_generator(args[0]);

        if (!Generator::generators().count(in_generator))
        {
            std::cerr << "no generator named " << in_generator << std::endl;
            print_available_generators();

            return false;
        }

        // create generator
        shared_ptr<Generator> generator(Generator::generators().at(in_generator)(ptree()));

        // print properties, parameters

        // since we have just created a "new" generator (passing an empty parameters object
        // the parameters contained in it describe its default values
        const ptree& params = generator->parameters();

        const int c0 = 20;
        std::cout << " parameter          | default" << std::endl;
        std::cout << "                    +        " << std::endl;

        // we assume that all parameter related entries we want
        // to print here are stored in the subtree "params".
        // if this subtree does not exist, we print nothing
        const boost::optional<const ptree&> subtree(params.get_child_optional("params"));
        if (subtree)
        {
            for (ptree::const_iterator it = subtree->begin(); it != subtree->end(); ++it)
            {
                const std::string& param_name = it->first;
                const std::string& param_default = it->second.data();
                std::cout << param_name;
                for (int i = 0; i < c0 - int(param_name.length()); i++) std::cout << ' ';
                std::cout << param_default;
                std::cout << std::endl;
            }
        }

        return true;
    }
};

class command_list : public Command
{
public:

    bool run(const std::vector<std::string>& /*args*/)
    {
        print_available_generators();
        return true;
    }
};


int main(int argc, char *argv[])
{
    typedef std::map<std::string, std::pair<boost::shared_ptr<Command>, std::string> > cmd_map_t;
    cmd_map_t cmd_desc;
    cmd_desc["compute"]    = std::make_pair(boost::make_shared<command_compute>()   , "compute descriptors");
    cmd_desc["info"]       = std::make_pair(boost::make_shared<command_info>()      , "print informations of specific generator");
    cmd_desc["list"]       = std::make_pair(boost::make_shared<command_list>()      , "print list of available generators");

    if (argc <= 1 || !cmd_desc.count(argv[1]))
    {
        std::cout << "usage: " << (argc > 0 ? argv[0]:"compute_descriptors") << " <command> ..." << std::endl;
        std::cout << " commands:" << std::endl;

        const int c0 = 20;
        cmd_map_t::const_iterator it;
        for (it = cmd_desc.begin(); it != cmd_desc.end(); ++it)
        {
            std::cout << " * " << it->first;
            for (int k = 0; k < c0 - (int)it->first.length(); k++) std::cout << ' ';
            std::cout << " : " << it->second.second << std::endl;
        }

        return 1;
    }
    return cmd_desc[argv[1]].first->run(argv_to_strings(argc-2, &argv[2])) ? 0:1;
}
