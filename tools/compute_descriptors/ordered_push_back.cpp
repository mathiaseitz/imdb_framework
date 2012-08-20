#include "ordered_push_back.hpp"

using namespace imdb;

OrderedPushBack::OrderedPushBack(shared_ptr<PropertyWriter> writer)
    : _writer(writer)
    , _numWrittenElements(0)
{}

bool OrderedPushBack::push_back(size_t index, const boost::any& element)
{
    boost::lock_guard<boost::mutex> locked(_mutex);

    // since the things we have written so far is just a linear
    // index of features, this condition must hold, i.e. all elements
    // we get must be at the end or behind of what has been written so far
    assert(index >= _numWrittenElements);

    _queue.push(queue_element(index, make_shared<boost::any>(element)));

    // *linearly* write stuff into the output vector
    while (!_queue.empty() && _queue.top().first == _numWrittenElements)
    {
        _writer->push_back(*_queue.top().second);
        _queue.pop();
        _numWrittenElements++;
    }

    return true;
}

bool OrderedPushBack::empty_buffer() const
{
    return _queue.empty();
}
