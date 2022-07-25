#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H

#include <cstdint>
#include <climits>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>

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
