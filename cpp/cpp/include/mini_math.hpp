#ifndef MINI_MATH_HPP
#define MINI_MATH_HPP

#include <cmath>
#include <list>
#include <utility>
#include <vector>

/*!
 * \brief The inner_outer class collects the maximum range as well as the intersection
 */
template <class T>
class inner_outer {
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
    } else {
      if (val < outer.first)
        outer.first = val;
      if (val > inner.first)
        inner.first = val;
    }
  }

  void set_higher(const T &val) {
    if (!this->is_init_h) {
      this->inner.second = val;
      this->outer.second = val;
      this->is_init_h = true;
    } else {
      if (val > outer.second)
        outer.second = val;
      if (val < inner.second)
        inner.second = val;
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
  std::pair<T, T> outer = std::pair<T, T>(0, 0); // only for better debugging
  std::pair<T, T> inner = std::pair<T, T>(0, 0);
};

template <class T>
std::vector<T> trim_by_index(const std::vector<T> &v, const std::pair<std::size_t, std::size_t> &idx_min_max) {
  auto first = v.begin() + idx_min_max.first;
  auto last = v.begin() + idx_min_max.second;
  return std::vector<T>(first, last);
}

template <class T>
double percentage_difference(const T &a, const T &b) {

  return std::abs((a - b) / ((a + b) / 2.0)) * 100.0;
}
void remove_spectral_lines(const std::vector<double> &f, const std::vector<double> &s, std::vector<double> &f_out,
                           std::vector<double> &s_out, const double base_freq, const size_t n_lines) {
  std::vector<double> f_tmp(f);
  std::vector<double> s_tmp(s);
  // swap the vectors
  f_out.reserve(f.size());
  s_out.reserve(s.size());
  size_t i = 0;

  // copy until  [i + n_line] is smaller base_freq
  for (size_t i = 0; i < f.size() - ((2 * n_lines) + 1); ++i) {
    double result = f[i + n_lines] / base_freq;
    // result is not a multiple of base_freq  without remainder
    if (result != std::floor(result)) {
      f_out.push_back(f[i]);
      s_out.push_back(s[i]);
    } else {
      i += (2 * n_lines);
    }
    // else {
    //   int odd = int(result);
    //   // check if odd is odd, so divide by 2 with no remainder (modulo)
    //   if (odd % 2 == 0) {
    //     f_out.push_back(f[i]);
    //     s_out.push_back(s[i]);
    //   } else {
    //     // we have to skip the next (2 * n_lines) + 1 element which is incremented at the end of the loop
    //     i += (2 * n_lines);
    //   }
    // }
  }
}

/*!
 * @brief  takes a sorted vector of pairs
 * @param f frequencies
 * @param s signal such as amplitude or phase
 * @param f_out
 * @param s_out
 * @param ranges such as std::vector<std::pair<double, double>> power_lines_ranges = {{12, 20}, {46, 54}, {146, 154}}; SORTED!!
 */
void remove_spectral_range(const std::vector<double> &f, const std::vector<double> &s, std::vector<double> &f_out,
                           std::vector<double> &s_out, const std::vector<std::pair<double, double>> &ranges) {

  auto r_iter = ranges.cbegin();
  double last_f = ranges.back().second;
  f_out.reserve(f.size());
  s_out.reserve(s.size());

  for (size_t i = 0; i < f.size(); ++i) {
    // if f[i] is smaller than the first range, copy it
    if (f[i] < r_iter->first) {
      f_out.push_back(f[i]);
      s_out.push_back(s[i]);
      // if f[i] is greater copy it AND increment the range iterator
    } else if (f[i] > r_iter->second) {
      if (r_iter->second == last_f) {
        // copy the rest
        for (; i < f.size(); ++i) {
          f_out.push_back(f[i]);
          s_out.push_back(s[i]);
        }
        break; // break the loop, and we can not increment the range iterator
      }
      ++r_iter;
      --i; // decrement i to check the same f[i] again
    }
  }
}

void running_average(const std::vector<double> &in_f, const std::vector<double> &in_data, std::vector<double> &f_out, std::vector<double> &out_data, const size_t radius) {
  if (in_f.size() != in_data.size()) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << " :: input data size mismatch " << in_f.size() << " " << in_data.size();
    throw std::runtime_error(err_str.str());
  }
  if (radius == 0) {
    f_out = in_f;
    out_data = in_data;
    return;
  }
  f_out.reserve(in_f.size());
  out_data.reserve(in_data.size());
  for (size_t i = 0; i < in_f.size(); ++i) {
    double sum = 0;
    size_t count = 0;
    for (size_t j = i > radius ? i - radius : 0; j < (i + radius) && j < in_f.size(); ++j) {
      sum += in_data[j];
      ++count;
    }
    f_out.push_back(in_f[i]);
    out_data.push_back(sum / count);
  }
}

#endif // MINI_MATH_HPP
