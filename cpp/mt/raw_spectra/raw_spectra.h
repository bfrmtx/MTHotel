#ifndef RAW_SPECTRA_H
#define RAW_SPECTRA_H

#include <algorithm>
#include <complex>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "BS_thread_pool.h"
#include "atss.h"
#include "freqs.h"
#include "prz_vector.h"
#include "vector_math.h"

// for beeing accessable as jthreads core math stays outside if possible

void simple_ampl_stack(const std::vector<std::vector<std::complex<double>>> &in, std::vector<double> &out) {
  size_t n = in.at(0).size(); // n = f size
  out.resize(n, 0.0);         // f size

  for (size_t i = 0; i < n; ++i) {                 //   for frequencies
    auto ff = bvec::absv(bvec::get_fslice(in, i)); // get all stacks
    out[i] = bvec::mean(ff);
  }
}

void simple_ampl_stack_div(const std::vector<std::vector<std::complex<double>>> &in_nom, const std::vector<std::vector<std::complex<double>>> &in_denom, std::vector<double> &out) {

  if (in_nom.size() != in_denom.size()) {
    throw std::runtime_error("simple_ampl_stack: in_nom.size() != in_denom.size()");
  }

  size_t n = in_nom.at(0).size(); // n = f size
  out.resize(n, 0.0);             // f size

  for (size_t i = 0; i < n; ++i) {                       //   for frequencies
    auto ff = bvec::absv(bvec::get_fslice(in_nom, i));   // get all stacks
    auto dd = bvec::absv(bvec::get_fslice(in_denom, i)); // get all stacks
    for (size_t j = 0; j < ff.size(); ++j) {
      ff[j] /= dd[j];
    }
    out[i] = bvec::mean(ff);
  }
}

void advanced_ampl_stack(const std::vector<std::vector<std::complex<double>>> &in, std::vector<double> &out, const double &fraction_to_use) {

  size_t n = in.at(0).size(); // n = f size
  out.resize(n, 0.0);         // f size

  for (size_t i = 0; i < n; ++i) { //   for frequencies

    auto ff = bvec::absv(bvec::get_fslice(in, i)); // get all stacks
                                                   //        two_pass_variance var;
                                                   //        var.variance(ff.cbegin(), ff.cend());
                                                   //        out[i] = var.d_mean;
    out[i] = bvec::median_range_mean(ff, fraction_to_use);
    // out[i] = bvec::mean(ff);
  }
}

class raw_spectra {
public:
  /*!
   * \brief raw_spectra
   * \param fft_freqs copy of a shared pointer; the raw spectra contain trimmed results - so a subset of the complete FFT
   */

  raw_spectra(std::shared_ptr<BS::thread_pool> &pool, std::shared_ptr<fftw_freqs> &fft_freqs) {
    this->pool = pool;
    this->fft_freqs = fft_freqs;
    if (this->fft_freqs == nullptr) {
      throw std::runtime_error("raw_spectra: fft_freqs is nullptr");
    }
    if (pool == nullptr) {
      throw std::runtime_error("raw_spectra: pool is nullptr");
    }
  }

  raw_spectra(std::shared_ptr<BS::thread_pool> &pool, std::vector<std::shared_ptr<channel>> &channels) {
    this->pool = pool;
    // all fft_freqs will be the same - so take the fist valid one - only if this->fft_freqs is nullptr
    for (auto &c : channels) {
      if ((c->fft_freqs != nullptr) && (this->fft_freqs == nullptr)) {
        this->fft_freqs = c->fft_freqs;
        break;
      }
    }
    if (this->fft_freqs == nullptr) {
      throw std::runtime_error("raw_spectra: fft_freqs is nullptr");
    }
    if (pool == nullptr) {
      throw std::runtime_error("raw_spectra: pool is nullptr");
    }
  }

  ~raw_spectra() {
    this->fft_freqs.reset();
    this->pool.reset();
  }

  /*!
   * \brief set_raw_spectra moves the raw spectra from the channel to the raw_spectra object (after channel has read it from the file)
   * \param channel
   */
  void set_raw_spectra(std::shared_ptr<channel> chan);

  void simple_stack_all();

  void simple_stack_all_div(const std::shared_ptr<raw_spectra> raw, const std::string &channel_type);

  void scale_by_stacked_spectra_local(std::vector<double> &nominator, const bool is_remote = false, const bool is_emap = false);

  void advanced_stack_all(const double &fraction_to_use);

  /*!
   * @brief uses previous calculated stacked spectra and smooths it with a parzen window; that can also be advanced stacked or a another method
   */
  void parzen_stack_all();

  std::vector<double> get_abs_sa_spectra(const std::string &channel_type, const bool is_remote = false, const bool is_emap = false) const;
  std::pair<double, double> get_abs_sa_spectra_min_max(const std::string &channel_type, const bool is_remote = false, const bool is_emap = false) const;
  std::vector<double> get_abs_sa_prz_spectra(const std::string &channel_type, const bool is_remote = false, const bool is_emap = false) const;
  std::vector<double> get_abs_spectra(const std::string &channel_type, const size_t nstack = 0, const bool is_remote = false, const bool is_emap = false) const;

  std::vector<std::vector<std::complex<double>>> ex; //!< spectra from fft, local or center site, [stacks][frequencies]
  std::vector<std::vector<std::complex<double>>> ey; //!< spectra from fft, local or center site
  std::vector<std::vector<std::complex<double>>> hx; //!< spectra from fft, local or center site
  std::vector<std::vector<std::complex<double>>> hy; //!< spectra from fft, local or center site
  std::vector<std::vector<std::complex<double>>> hz; //!< spectra from fft, local or center site

  std::vector<std::vector<std::complex<double>>> rhx; //!< spectra from fft, remote site
  std::vector<std::vector<std::complex<double>>> rhy; //!< spectra from fft, remote site
  std::vector<std::vector<std::complex<double>>> rhz; //!< spectra from fft, remote site
  std::vector<std::vector<std::complex<double>>> rex; //!< spectra from fft, remote site
  std::vector<std::vector<std::complex<double>>> rey; //!< spectra from fft, remote site

  std::vector<std::vector<std::complex<double>>> eex; //!< spectra from fft, emap site
  std::vector<std::vector<std::complex<double>>> eey; //!< spectra from fft, emap site

  std::shared_ptr<fftw_freqs> fft_freqs;          //!< fftw_freqs object, need to know how the incoming spectra have been calculated
  std::shared_ptr<BS::thread_pool> pool;          //!< thread pool from main program
  std::vector<std::shared_ptr<channel>> channels; //!< all channels from the raw file for reference

  double bw = 0; // bandwidth of fft

private:
  std::vector<double> sa_ex; //!< stack all spectra from fft, local or center site
  std::vector<double> sa_ey; //!< stack all spectra from fft, local or center site
  std::vector<double> sa_hx; //!< stack all spectra from fft, local or center site
  std::vector<double> sa_hy; //!< stack all spectra from fft, local or center site
  std::vector<double> sa_hz; //!< stack all spectra from fft, local or center site

  std::vector<double> sa_rhx; //!< stack all spectra from fft, remote site
  std::vector<double> sa_rhy; //!< stack all spectra from fft, remote site
  std::vector<double> sa_rhz; //!< stack all spectra from fft, remote site
  std::vector<double> sa_rex; //!< stack all spectra from fft, remote site
  std::vector<double> sa_rey; //!< stack all spectra from fft, remote site

  std::vector<double> sa_eex; //!< stack all spectra from fft, emap site
  std::vector<double> sa_eey; //!< stack all spectra from fft, emap site

  std::vector<double> sa_prz_ex; //!< stack all spectra smoothed from fft, local or center site
  std::vector<double> sa_prz_ey; //!< stack all spectra smoothed from fft, local or center site
  std::vector<double> sa_prz_hx; //!< stack all spectra smoothed from fft, local or center site
  std::vector<double> sa_prz_hy; //!< stack all spectra smoothed from fft, local or center site
  std::vector<double> sa_prz_hz; //!< stack all spectra smoothed from fft, local or center site

  std::vector<double> sa_prz_rhx; //!< stack all spectra smoothed from fft, remote site
  std::vector<double> sa_prz_rhy; //!< stack all spectra smoothed from fft, remote site
  std::vector<double> sa_prz_rhz; //!< stack all spectra smoothed from fft, remote site
  std::vector<double> sa_prz_rex; //!< stack all spectra smoothed from fft, remote site
  std::vector<double> sa_prz_rey; //!< stack all spectra smoothed from fft, remote site

  std::vector<double> sa_prz_eex; //!< stack all spectra smoothed from fft, emap site
  std::vector<double> sa_prz_eey; //!< stack all spectra smoothed from fft, emap site
};

std::pair<double, double> min_max_sa_spc(const std::vector<std::shared_ptr<raw_spectra>> &raws, const std::string &channel_type,
                                         const bool is_remote = false, const bool is_emap = false) {

  std::pair<double, double> result(DBL_MIN, DBL_MAX);
  std::vector<double> ampl_max_mins;
  for (const auto &raw : raws) {
    auto mm = raw->get_abs_sa_spectra_min_max(channel_type, is_remote, is_emap);
    ampl_max_mins.push_back(mm.first);
    ampl_max_mins.push_back(mm.second);
  }
  auto r1 = std::minmax_element(ampl_max_mins.begin(), ampl_max_mins.end());
  result.first = *r1.first;
  result.second = *r1.second;
  return result;
}

#endif // RAW_SPECTRA_H
