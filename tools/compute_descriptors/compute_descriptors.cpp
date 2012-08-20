
#include <opencv2/highgui/highgui.hpp>

#include "compute_descriptors.hpp"

using namespace imdb;

ComputeDescriptors::ComputeDescriptors(boost::shared_ptr<Generator> generator, const FileList& files)
    : _generator(generator)
    , _files(files)
    , _index(0)
    , _error(false)
    , _started(false)
    , _finished(false)
{}

void ComputeDescriptors::add_writer(const std::string& name, boost::shared_ptr<PropertyWriter> writer)
{
    _writers.push_back(std::make_pair(name, boost::make_shared<OrderedPushBack>(writer)));
}

bool ComputeDescriptors::start(int num_threads)
{
    assert(num_threads > 0);
    using namespace boost;

    if (_started) return false;

    _started = true;
    _datetime = QDateTime::currentDateTime();

    thread_group pool;
    for (int i = 0; i < num_threads; i++)
    {
        pool.add_thread(new thread(std::mem_fun(&ComputeDescriptors::_thread), this, _generator));
    }

    pool.join_all();

    _finished = true;
    _seconds = _datetime.secsTo(QDateTime::currentDateTime());

    for (size_t i = 0; i < _writers.size(); i++)
    {
        _error |= !_writers[i].second->empty_buffer();
    }

    return (!_error);
}

size_t ComputeDescriptors::current() const
{
    boost::lock_guard<boost::mutex> lock(_mutex);
    return _index;
}

bool ComputeDescriptors::finished() const
{
    return _finished;
}

index_t ComputeDescriptors::num_files() const
{
    return _files.size();
}

int ComputeDescriptors::computation_time() const
{
    // precondition: finished() == true
    return _seconds;
}

void ComputeDescriptors::_thread(boost::shared_ptr<Generator> gen)
{
    while (!_error)
    {
        size_t current;
        anymap_t data;

        {
            boost::lock_guard<boost::mutex> lock(_mutex);
            if (_index == _files.size()) break;
            current = _index;
            _index++;

        }

        string filename = _files.get_filename(current);

        try
        {
            // second parmeter: flags >0 means that the loaded image is forced to be a 3-channel color image
            //
            // Although this is not explicitly stated in the OpenCV docs, the channel
            // order is BGR, this has been tested by Mathias 08.June.2011 for both png
            // and jpg images. I.e. the following code
            // cv::Mat img = cv::imread("/Users/admin/tmp/blue.jpg");
            // cv::Vec3b v = img.at<cv::Vec3b>(0,0);
            // will result in the blue information at v[0], green at v[1] and red at v[2]
            mat_8uc3_t image = cv::imread(filename, 1);
            data["image"] = image;
            data["image_filename"] = filename;
            gen->compute(data);
        }

        catch(cv::Exception& e)
        {
            // the original opencv exception message is very poor -- make it more clear
            // and more importantly give the problematic filename
            throw std::runtime_error("compute_descriptors: cv::imread failed for file: " + filename);
        }

        catch (std::exception& e)
        {
            std::cerr << e.what() << std::endl;
            _error = true;
            return;
        }

        for (std::vector<string_writer_pair>::const_iterator wi = _writers.begin(); wi != _writers.end(); ++wi)
        {
            anymap_t::const_iterator ri = data.find(wi->first);
            if (ri != data.end()) wi->second->push_back(current, ri->second);
        }
    }
}
