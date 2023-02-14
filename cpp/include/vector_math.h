#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H

#include <cstdint>
#include <climits>
#include <string>
#include <cmath>
#include <vector>
#include <complex>
#include <type_traits>
#include <algorithm>
#include <numeric>

namespace bvec {

template <class T>
void cplx2ri(const std::vector<std::complex<T> >& in,
             std::vector<T>& treal, std::vector<T>& timag) {

    if (!in.size()) return;
    treal.resize(in.size());
    timag.resize(in.size());

    for (size_t i = 0; i < in.size(); ++i) {
        treal[i] = real(in[i]);
        timag[i] = imag(in[i]);
    }
}

template <class T>
void ri2cplx(const std::vector<T>& real, const std::vector<T>& imag,
             std::vector<std::complex<T> >& cplx) {

    if (real.size() != imag.size()) return;
    if (!real.size()) return;
    cplx.resize(real.size());

    for (size_t i = 0; i < real.size(); ++i) {
        cplx[i] = std::complex<T>(real, imag);
    }
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
    std::sort(idx.begin(), idx.end(), [&v](size_t i1, size_t i2) {return v[i1] < v[i2];});

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
std::vector<T> vector_from_indices(const std::vector<size_t>& idx, const std::vector<T> &v) {
    std::vector<T> out;
    if (!idx.size()) return out;
    if (idx.size() > v.size()) return out;
    out.reserve(v.size());
    for (auto i: idx) {
        out.emplace_back(v[i]);
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
    for (const auto &val : v) res[i++] = std::fabs(val);
    return res;
}

template<typename T>
T mean (const std::vector<T> &v) {

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
    if (w.size() % 2 == 0)       // array is even
        med = (w[w.size()/2] + w[w.size()/2 - 1])/half;
    else
        med = w[w.size()/2];  // integer divison should not round up
    return med;
}

template <typename T>
T median_range_mean(const std::vector<T> &v, const double &fraction_to_use) {

    T sum = T(0);
    if (fraction_to_use >= 1) return bvec::mean(v);
    if (fraction_to_use < 0.01) return bvec::mean(v);
    std::vector<T> w(v);
    std::sort(w.begin(), w.end());
    size_t use = size_t(double(w.size()) * fraction_to_use) / 2;
    auto b = w.cbegin();
    auto e = w.cbegin();
    std::advance(b, w.size()/2);
    std::advance(e, w.size()/2);
    if (w.size() % 2 == 0) {      // array is even
        std::advance(b, -use);
        std::advance(e, use);
    }
    else {
        std::advance(b, -use);
        std::advance(e, use+1);
    }
    if ((std::distance(w.cbegin(), b) >= 0) && (std::distance(e, w.cend()) >= 0) && (std::distance(b, e) > 0) ) {
        sum = std::accumulate(b, e, T(0));
        T divm = (T)(std::distance(b,e));
        sum /= T(divm);
    }
    return sum;

}

double median_range_mean_cplx(const std::vector<std::complex<double>> &v, const double &fraction_to_use) {

    double sum = 0.0;
    std::vector<double> w(bvec::absv(v));
    std::sort(w.begin(), w.end());
    size_t use = size_t((double(w.size()) * fraction_to_use) / 2.0);
    auto b = w.begin();
    auto e = w.begin();
    std::advance(b, w.size()/2);
    std::advance(e, w.size()/2);
    std::advance(b, -use);
    std::advance(e, use);
    if ((std::distance(w.begin(), b) >= 0) && (std::distance(e, w.end()) >= 0) && (std::distance(b, e) > 0) ) {
        sum = accumulate(b, e, 0.0);
        sum /= double(v.size());
    }
    return sum;

}





/*!
 * \brief swap_vec_vec  a) v[stacks][f], b) v[f][stacks]
 * \param in
 * \return swaped a) v[f][stacks], b) v[stacks][f]
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
    if (!in.size()) return std::vector<T>();
    if (  f_index > (in.at(0).size()-1) ) return std::vector<T>();
    std::vector<T> out(in.size()); // stack length
    size_t i = 0;
    for (const auto &v : in) {     // for all stacks
        out[i++] = v[f_index];     // get the requested indes
    }
    return out;
}



} // end namespace

///*!
// * \brief indices_wo_duplicates_sorted scans a sorted vector
// * \param v vector to scan
// * \return index vector of v referring to entries with no duplcate
// */
//template <typename T>
//std::vector<size_t> indices_wo_duplicates_sorted(const std::vector<T> &v) {

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
