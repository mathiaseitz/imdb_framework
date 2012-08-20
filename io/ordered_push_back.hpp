#ifndef ORDERED_PUSH_BACK_HPP
#define ORDERED_PUSH_BACK_HPP

#include <queue>

#include <boost/thread.hpp>

#include <util/types.hpp>
#include <io/property_writer.hpp>

namespace imdb {

class OrderedPushBack
{
    public:

    OrderedPushBack(shared_ptr<PropertyWriter> writer);

    bool push_back(size_t index, const boost::any& element);

    bool empty_buffer() const;

    private:

    boost::shared_ptr<PropertyWriter> _writer;
    std::size_t _numWrittenElements;
    boost::mutex _mutex;

    typedef std::pair<size_t, boost::shared_ptr<boost::any> > queue_element;
    typedef std::greater<queue_element>                       queue_compare;
    typedef std::priority_queue<queue_element, std::vector<queue_element>, queue_compare> queue_t;

    queue_t _queue;
};

} // namespace imdb

#endif // ORDERED_PUSH_BACK_HPP
