#ifndef MINI_MATH_H
#define MINI_MATH_H

#include <cmath>
#include <utility>
#include <vector>


/*!
 * \brief The inner_outer class collects the maximum range as well as the intersection
 */
template<class T>
class inner_outer
{
public:
    inner_outer() {

    }
    ~inner_outer() {

    }

    void set_lower(const T &val) {
        if (!this->is_init_l) {
            this->inner.first = val;
            this->outer.first = val;
            this->is_init_l = true;
        }
        else {
            if (val < outer.first) outer.first = val;
            if (val > inner.first) inner.first = val;
        }
    }

    void set_higher(const T &val) {
        if (!this->is_init_h) {
            this->inner.second = val;
            this->outer.second = val;
            this->is_init_h = true;
        }
        else {
            if (val > outer.second) outer.second = val;
            if (val < inner.second) inner.second = val;
        }
    }

    void set_low_high(const std::pair<T, T> &min_max) {
        this->set_lower(min_max.first);
        this->set_higher(min_max.second);
    }

    void clear() {
        this->is_init_h = false;
        this->is_init_l = false;
    }

    std::pair<T, T> get_inner() const {
        return this->inner;
    }
    std::pair<T, T> get_outer() const {
        return this->outer;

    }

private:
    bool is_init_l = false;
    bool is_init_h = false;
    std::pair<T, T> outer = std::pair<T, T> (0, 0);  // only for better debugging
    std::pair<T, T> inner = std::pair<T, T> (0, 0);
};

template<class T>
std::vector<T> trim_by_index(const std::vector<T> &v, const std::pair<std::size_t, std::size_t> &idx_min_max ) {
    auto first = v.begin() + idx_min_max.first;
    auto last = v.begin() + idx_min_max.second;
    return std::vector<T>(first, last);
}

template<class T>
double percentage_difference (const T &a, const T &b) {

    return std::abs((a - b) / ((a + b) / 2.0)) * 100.0;
}

#endif // MINI_MATH_H
