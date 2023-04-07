#ifndef RING_BUF_MICRO_H
#define RING_BUF_MICRO_H

#include <vector>
#include <numeric>



template<class  T>
/*!
 * \brief The mini_ring_buf class : put values in a ring buffer and in case immedeately make a (running) average; the buffer is NOT for consuming
 */
class mini_ring_buf_avg {
public:

    mini_ring_buf_avg(const std::size_t &size) {
        buf.resize(size);
    }
    void push_back(const T &val) {
        buf[p++] = val;
        if (p >= buf.size()) p = 0;
        ++count;
    }

    double push_back_avg(const T &val) {
        this->push_back(val);
        if (count < buf.size()) {
            auto end = buf.begin();
            std::advance(end, p);
            return double(std::accumulate(buf.begin(), end, 0));
        }
        return double(std::accumulate(buf.begin(), buf.end(), 0));
    }

    std::size_t size() const {
        return buf.size();
    }

    T last() {
        if (p == 0) return buf.back();
        return buf[p-1];
    }

private:

    std::size_t p = 0;
    std::vector<T> buf;
    std::size_t count = 0;

};

#endif // RING_BUF_MICRO_H
