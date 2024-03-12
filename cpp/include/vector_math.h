#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H

#include <algorithm>
#include <climits>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <string>
#include <type_traits>
#include <vector>

#include "iterator_templates.h"

namespace bvec {

template <class T>
void cplx2ri(const std::vector<std::complex<T>> &in,
             std::vector<T> &treal, std::vector<T> &timag) {

  if (!in.size())
    return;
  treal.resize(in.size());
  timag.resize(in.size());

  for (size_t i = 0; i < in.size(); ++i) {
    treal[i] = real(in[i]);
    timag[i] = imag(in[i]);
  }
}

template <class T>
void ri2cplx(const std::vector<T> &real, const std::vector<T> &imag,
             std::vector<std::complex<T>> &cplx) {

  if (real.size() != imag.size())
    return;
  if (!real.size())
    return;
  cplx.resize(real.size());

  for (size_t i = 0; i < real.size(); ++i) {
    cplx[i] = std::complex<T>(real, imag);
  }
}

/*!
 * @brief
 * @param cplx vecotr of complex numbers
 * @param ampl amplitude vector retuned
 * @param phz phase vector returned
 * @param deg if true, phase is returned in degrees (not radians)
 */
inline void cplx2ap(const std::vector<std::complex<double>> &cplx, std::vector<double> &ampl, std::vector<double> &phz, const bool deg = false) {
  if (!cplx.size())
    return;
  ampl.resize(cplx.size());
  phz.resize(cplx.size());
  auto iter_a = ampl.begin();
  auto iter_p = phz.begin();
  if (deg) {
    for (const auto &c : cplx) {
      *(iter_a++) = std::abs(c);
      *(iter_p++) = std::arg(c) * (180.0 / M_PI);
    }
  } else {
    for (const auto &c : cplx) {
      *(iter_a++) = std::abs(c);
      *(iter_p++) = std::arg(c);
    }
  }
}

inline void cplx2_vap(const std::vector<std::complex<double>> &cplx, std::vector<std::vector<double>> &ampls, std::vector<std::vector<double>> &phzs, const bool deg = false) {
  if (!cplx.size())
    return;
  ampls.emplace_back(std::vector<double>(cplx.size()));
  phzs.emplace_back(std::vector<double>(cplx.size()));
  cplx2ap(cplx, ampls.back(), phzs.back(), deg);
}

/*!
 * \brief sort_indices simulates a sort of v; assuming freq, ampl and phase,
 * you sort freq by index and use the return vector index for finally sorting freq, ampl and phase
 * keeping the triple (freq, ampl, phase) together, orderd by f
 * \param v vector to sort in future
 * \return vector of indices of v, presenting the sorted vector
 */
template <typename T>
std::vector<size_t> sort_indices(const std::vector<T> &v) {

  // initialize original index locations
  std::vector<size_t> idx(v.size());
  std::iota(idx.begin(), idx.end(), 0);

  // sort indices based on comparing values in v
  std::sort(idx.begin(), idx.end(), [&v](size_t i1, size_t i2) { return v[i1] < v[i2]; });

  return idx;
}

/*!
 * \brief vector_from_indices - assume you sort freq and want to apply the result to ampl
 * keeping the triple (freq, ampl) together
 * \param idx index for re-arrange the vector
 * \param v
 * \return vector sorted by idx
 */
template <typename T>
std::vector<T> vector_from_indices(const std::vector<size_t> &idx, const std::vector<T> &v) {
  std::vector<T> out;
  if (!idx.size())
    return out;
  if (idx.size() > v.size())
    return out;
  out.reserve(v.size());
  for (auto i : idx) {
    out.emplace_back(v[i]);
  }

  return out;
}

template <typename T>
std::vector<T> join_overlap(const std::vector<T> &lower, const std::vector<T> &higher, const size_t overlap) {
  std::vector<T> out;
  if (!lower.size() || !higher.size())
    return out;
  if (lower.size() < overlap || higher.size() < overlap)
    return out;
  out.reserve(lower.size() + higher.size() - overlap);
  for (size_t i = 0; i < lower.size() - overlap; ++i) {
    out.emplace_back(lower[i]);
  }
  size_t j = 0;
  // add the overlap from both vectors and divide by 2
  for (size_t i = lower.size() - overlap; i < lower.size(); ++i) {
    out.emplace_back((lower[i] + higher[j++]) / T(2));
  }
  for (size_t i = overlap; i < higher.size(); ++i) {
    out.emplace_back(higher[i]);
  }
  return out;
}

/*!
 * \brief absv
 * \param v vector<std::complex<double>>
 * \return vector<double> of absolute values
 */
template <typename T>
std::vector<T> absv(const std::vector<std::complex<T>> &v) {
  std::vector<double> res(v.size());
  size_t i = 0;
  for (const auto &val : v)
    res[i++] = std::abs(val);
  return res;
}

template <typename T>
T mean(const std::vector<T> &v) {

  T sum = T(0);
  sum = accumulate(v.cbegin(), v.cend(), T(0));
  sum /= T(v.size());
  return sum;
}

template <typename T>
T median(const std::vector<T> &v) {

  T med;
  T half = T(2);
  std::vector<T> w(v);
  std::sort(w.begin(), w.end());
  if (w.size() % 2 == 0) // array is even
    med = (w[w.size() / 2] + w[w.size() / 2 - 1]) / half;
  else
    med = w[w.size() / 2]; // integer divison should not round up
  return med;
}

template <typename T>
T median_range_mean(const std::vector<T> &v, const double &fraction_to_use) {

  T sum = T(0);
  if (fraction_to_use >= 1)
    return bvec::mean(v);
  if (fraction_to_use < 0.01)
    return bvec::mean(v);
  std::vector<T> w(v);
  std::sort(w.begin(), w.end());
  size_t use = size_t(double(w.size()) * fraction_to_use) / 2;
  auto b = w.cbegin();
  auto e = w.cbegin();
  std::advance(b, w.size() / 2);
  std::advance(e, w.size() / 2);
  if (w.size() % 2 == 0) { // array is even
    std::advance(b, -use);
    std::advance(e, use);
  } else {
    std::advance(b, -use);
    std::advance(e, use + 1);
  }
  if ((std::distance(w.cbegin(), b) >= 0) && (std::distance(e, w.cend()) >= 0) && (std::distance(b, e) > 0)) {
    sum = std::accumulate(b, e, T(0));
    T divm = (T)(std::distance(b, e));
    sum /= T(divm);
  }
  return sum;
}

inline double median_range_mean_cplx(const std::vector<std::complex<double>> &v, const double &fraction_to_use) {

  double sum = 0.0;
  std::vector<double> w(bvec::absv(v));
  std::sort(w.begin(), w.end());
  size_t use = size_t((double(w.size()) * fraction_to_use) / 2.0);
  auto b = w.begin();
  auto e = w.begin();
  std::advance(b, w.size() / 2);
  std::advance(e, w.size() / 2);
  std::advance(b, -use);
  std::advance(e, use);
  if ((std::distance(w.begin(), b) >= 0) && (std::distance(e, w.end()) >= 0) && (std::distance(b, e) > 0)) {
    sum = accumulate(b, e, 0.0);
    sum /= double(v.size());
  }
  return sum;
}

/*!
 * \brief swap_vec_vec  a) v[stacks][f], b) v[f][stacks]
 * \param in
 * \return swapped a) v[f][stacks], b) v[stacks][f]
 */
template <typename T>
std::vector<std::vector<T>> swap_vec_vec(const std::vector<std::vector<T>> &in) {

  std::vector<std::vector<T>> out(in.at(0).size());
  for (auto &v : out) {
    v.resize(in.size());
  }
  size_t j = 0;
  for (const auto &v : in) {
    size_t i = 0;
    for (const auto &val : v) {
      out[i++][j] = val;
    }
    ++j;
  }

  return out;
}
/*!
 * \brief get_fslice
 * \param in vector[stacks][f_index]
 * \param f_index
 * \return vector[f] of stack length
 */
template <typename T>
std::vector<T> get_fslice(const std::vector<std::vector<T>> &in, const size_t f_index) {
  if (!in.size())
    return std::vector<T>();
  if (f_index > (in.at(0).size() - 1))
    return std::vector<T>();
  std::vector<T> out(in.size()); // stack length
  size_t i = 0;
  for (const auto &v : in) { // for all stacks
    out[i++] = v[f_index];   // get the requested indes
  }
  return out;
}

/*!
 * @brief interpolates a x,y data set with Akima spline to new given x-axis (e.g. frequencies) and gives back the interpolated y data
 * @param x_in
 * @param y_in
 * @param new_xin
 * @param y_out
 * @return
 */
inline size_t akima_vector_double(const std::vector<double> &x_in, const std::vector<double> &y_in,
                                  const std::vector<double> &new_x_in, std::vector<double> &y_out) {

  // new_x_in                        // the new x-axis or frequencies
  y_out.resize(new_x_in.size()); // the result - the new y axis or amplitude / phase

  size_t j;

  // nodes
  std::vector<double> b(x_in.size());
  std::vector<double> c(x_in.size());
  std::vector<double> d(x_in.size());

  std::vector<double>::const_iterator cfb, cfe, cyrb, cyre, cnfb, cnfe;
  std::vector<double>::iterator bb, be, cb, ce, db, de;

  size_t old_cols;

  // that must be the row size of the calibration matrix

  old_cols = x_in.size();

  b.resize(old_cols);
  c.resize(old_cols);
  d.resize(old_cols);

  // f = x_in;

  cfb = x_in.begin();
  cfe = x_in.end();
  cyrb = y_in.begin();
  cyre = y_in.end();

  bb = b.begin();
  be = b.end();
  cb = c.begin();
  ce = c.end();
  db = d.begin();
  de = d.end();
  cnfb = new_x_in.begin();
  cnfe = new_x_in.end();

  // qDebug()  << "akima run" << rows << cols << old_cols;
  // take the frequencies from calibration files, the real or imag from calibration and calculate the set of b,c,d coefficients
  //  real part
  miter::aspline<double, std::vector<double>::const_iterator, std::vector<double>::iterator>(cfb, cfe, cyrb, cyre, bb, be, cb, ce, db, de);

  cfb = x_in.begin();
  cfe = x_in.end();
  cyrb = y_in.begin();

  bb = b.begin();
  cb = c.begin();
  db = d.begin();
  cnfb = new_x_in.begin();

  for (j = 0; j < new_x_in.size(); ++j) {
    y_out[j] = miter::seval<double, std::vector<double>::const_iterator>(cnfb, cfb, cfe, cyrb, bb, cb, db);
    // qDebug() << j << new_x_in.at(j) << *cnfb << y_out.at(j);
    ++cnfb;
  }

  return y_out.size();
}

inline double fold(const std::vector<double> &v, const std::vector<double> &w) {
  double sum = 0.0;
  if (!v.size())
    throw std::runtime_error("fold: data vector size zero");
  if (v.size() != w.size())
    throw std::runtime_error("fold: vector size mismatch");

  for (size_t i = 0; i < v.size(); ++i) {
    sum += v[i] * w[i];
  }
  return sum;
}

} // namespace bvec

///*!
// * \brief indices_wo_duplicates_sorted scans a sorted vector
// * \param v vector to scan
// * \return index vector of v referring to entries with no duplcate
// */
// template <typename T>
// std::vector<size_t> indices_wo_duplicates_sorted(const std::vector<T> &v) {

//        std::vector<size_t> idx;
//        if (!v.size()) return idx;
//        idx.reserve(v.size());
//        idx.emplace_back(0);
//        for (size_t i = 1; i < v.size(); ++i) {
//            if (v[i - j] != v[i] ) {
//                idx.emplace_back(i);
//            }
//        }

//        return idx;
//}

#endif
