#ifndef PARZEN_VECTOR_H
#define PARZEN_VECTOR_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <numbers>
#include <numeric>
#include <sstream>
#include <utility>
#include <vector>

#include "base_constants.h"

template <typename T, typename S>
void parzen(const std::vector<T> &data, const std::vector<S> &selected_freqs, const std::vector<std::vector<S>> &parzendists, std::vector<T> &result) {

  result.reserve(selected_freqs.size());
  for (const auto &przd : parzendists) {
    result.push_back(std::inner_product(data.cbegin(), data.cend(), przd.cbegin(), T(0)));
  }

  if (result.size() != selected_freqs.size()) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::result size != selected frequencies size";
    throw std::runtime_error(err_str.str());
  }
  // for (auto &res: result) res *= 0.7;

  return;
}
template <typename T, typename S>
void parzen_t(const std::shared_ptr<std::vector<T>> &data, const std::vector<S> &selected_freqs, const std::vector<std::vector<S>> &parzendists, std::shared_ptr<std::vector<T>> &result) {
  parzen(*data, selected_freqs, parzendists, *result);
}

static size_t parzen_vector(const std::vector<double> &freqs, const std::vector<double> &target_freqs, const double &prz_radius,
                            std::vector<double> &selected_freqs, std::vector<std::vector<double>> &parzendists) {

  if (!freqs.size() || !target_freqs.size()) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no frequencies or target frequencies given";
    throw std::runtime_error(err_str.str());
  }
  const double max_parzen = 0.31;

  if (prz_radius <= 0.0) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::parzen radius is < 0: " << prz_radius;
    throw std::runtime_error(err_str.str());
  }

  if (prz_radius > max_parzen) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::parzen radius " << prz_radius << " is > " << max_parzen;
    throw std::runtime_error(err_str.str());
  }

  double prz_rad = prz_radius;
  double step_change = 0.005;

  std::vector<std::vector<double>::const_iterator> iter_lwr_bnds;
  std::vector<std::vector<double>::const_iterator> iter_upr_bnds;
  std::vector<double>::const_iterator low;
  std::vector<double>::const_iterator high;

  int max_tries = 40;
  int tries = 0;
  do {
    if (prz_rad < max_parzen) {
      selected_freqs.clear();
      iter_lwr_bnds.clear();
      iter_upr_bnds.clear();
      std::vector<double>::const_iterator min_freq = freqs.begin();
      std::vector<double>::const_iterator max_freq = freqs.end() - 1;

      // eg. the zero element contains the DC part f = 0 Hz
      if (*min_freq == 0.0)
        ++min_freq;

      // copy the list
      std::vector<double> lower_prz_bounds = target_freqs;
      std::vector<double> upper_prz_bounds = target_freqs;

      // set the range of frequencies to be included in pazening
      //  so if target f was 100 and prz_rad = 0.1 we have 90 .. 100 .. 110
      for (size_t i = 0; i < lower_prz_bounds.size(); ++i) {
        lower_prz_bounds[i] = lower_prz_bounds[i] - (lower_prz_bounds[i] * prz_rad);
        upper_prz_bounds[i] = upper_prz_bounds[i] + (upper_prz_bounds[i] * prz_rad);
      }

      // now find the itrators pointing to the valid areas of the frequency vector containing the parzen freqs
      // *iter_lwr_bnds[i] is value, iter_lwr_bnds - f.begin() is pos
      for (size_t i = 0; i < lower_prz_bounds.size(); ++i) {
        if ((lower_prz_bounds[i] >= *min_freq) && (upper_prz_bounds[i] <= *max_freq)) {
          low = std::lower_bound(min_freq, max_freq, lower_prz_bounds[i]);
          high = std::upper_bound(min_freq, max_freq, upper_prz_bounds[i]);

          if (high - low > 1) {
            // all lower start points
            iter_lwr_bnds.push_back(low);
            // all upper end points
            iter_upr_bnds.push_back(high);
            // all freqs belong to that
            selected_freqs.push_back(target_freqs[i]);
            // if (target_freqs[i] > 1.0)
            //   std::cout << *high << " < " << target_freqs[i] << " > " << *low << " :target " << std::endl;
            // else
            //   std::cout << 1.0 / (*high) << " < " << 1.0 / target_freqs[i] << " > " << 1.0 / (*low) << " :target " << std::endl;

          } else {
            std::cout << "slot_parzen_vector -> no bounds " << lower_prz_bounds[i] << " " << *min_freq << " " << upper_prz_bounds[i] << " " << *max_freq << std::endl;
            std::cout << "slot_parzen_vector -> no bounds " << 1.0 / lower_prz_bounds[i] << " " << 1.0 / (*min_freq) << " " << 1.0 / upper_prz_bounds[i] << " " << 1.0 / (*max_freq) << std::endl;
          }
        }
      }

      ++tries;
      if (selected_freqs.size() < 1) {
        prz_rad += step_change;
        std::cerr << " math_vector::slot_parzen_vector ---> changed PARZEN! " << prz_rad << std::endl;
      }
    }

  } while ((selected_freqs.size() < 1) && (tries < max_tries));

  if (!selected_freqs.size()) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::no frequencies selected ";
    throw std::runtime_error(err_str.str());
  }

  parzendists.clear();
  parzendists.resize(selected_freqs.size());
  for (auto &prz : parzendists) {
    prz.resize(freqs.size(), 0.0);
  }

  double dist = 0;
  double distsum = 0;
  size_t k;

  // for each of the frequencies found we make a parzen vector (with length of the incoming fft spectra freqs
  for (size_t i = 0; i < selected_freqs.size(); ++i) {
    // no realloc here; reset to 0
    // parzendists.assign(freqs.size(), 0.0);
    distsum = 0;
    dist = 0;
    low = iter_lwr_bnds[i]; // save the original iterpos
    high = iter_upr_bnds[i];
    while (low < high) {
      k = (low - freqs.begin()); // index of frequency vector is naturally the same as freq
      dist = (fabs(selected_freqs[i] - *low) * M_PI) / (selected_freqs[i] * prz_rad);
      // k is the index of freqs
      if (!dist) {
        distsum += 1.0;
        parzendists[i][k] = 1.0;
      } else {
        dist = pow(((sin(dist)) / dist), 4.);
        // dist = ((sin(dist))/dist);
        parzendists[i][k] = dist;
      }
      distsum += dist;
      ++low;
      // if (dist > 0.0001) this->mtd->avgfs[i] += 1;
    }
    // this->mtd->bws[i] =  (this->mtd->avgfs[i] * this->mcd->get_key_dvalue("sample_freq")) / ( this->mcd->get_key_uivalue("read_samples") * M_PI);
    //       std::cout << this->mtd->avgfs[i] << this->mcd->get_key_dvalue("sample_freq") << this->mcd->get_key_uivalue("read_samples");
    //       std::cout << distsum << "at" << this->mtd->przfreq_sel.at(i) << *iter_lwr_bnds[i] << *iter_upr_bnds[i] << *iter_upr_bnds[i] - *iter_lwr_bnds[i] << j << "lines";

    if (distsum < treat_as_null) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::distsum too small - can't avarage < " << treat_as_null;
      throw std::runtime_error(err_str.str());
    }

    for (auto &val : parzendists[i])
      val /= distsum;
  }

  return 1;
}

#endif // PARZEN_VECTOR_H
