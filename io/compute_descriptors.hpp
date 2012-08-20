#ifndef COMPUTE_DESCRIPTORS_HPP
#define COMPUTE_DESCRIPTORS_HPP

#include <boost/thread.hpp>

#include <QDateTime>

#include <util/types.hpp>
#include <io/filelist.hpp>
#include <io/property_writer.hpp>
#include <descriptors/generator.hpp>

#include "ordered_push_back.hpp"

namespace imdb {

class ComputeDescriptors
{
    typedef std::pair<std::string, boost::shared_ptr<OrderedPushBack> > string_writer_pair;

    public:

    ComputeDescriptors(boost::shared_ptr<imdb::Generator> generator, const imdb::FileList& files);

    void add_writer(const std::string& name, boost::shared_ptr<imdb::PropertyWriter> writer);

    bool start(int num_threads);

    size_t current() const;
    bool finished() const;

    index_t num_files() const;
    int computation_time() const;

    private:

    void _thread(boost::shared_ptr<imdb::Generator> gen);

    boost::shared_ptr<imdb::Generator> _generator;
    std::vector<string_writer_pair>    _writers;
    imdb::FileList                     _files;

    volatile size_t _index;
    volatile bool _error;
    volatile bool _started;
    volatile bool _finished;

    QDateTime _datetime;
    int       _seconds;

    mutable boost::mutex _mutex;
};

} // namespace imdb

#endif // COMPUTE_DESCRIPTORS_HPP
