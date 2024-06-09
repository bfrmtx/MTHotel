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
#include "base_constants.h"
#include "freqs.h"
#include "prz_vector.h"
#include "spc_base.h"
#include "vector_math.h"

#include <filesystem>
namespace fs = std::filesystem;

// ***************************************************** R A W   S P E C T R A ****************************************************************

/*!
 * \brief The raw_spectra class, map is base class
 */
class raw_spectra : public spc_base<std::vector<std::complex<double>>> {
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
    auto it = this->channels.begin();
    for (auto &c : this->channels) {
      c.reset();
    }
  }

  /*!
   * \brief move_raw_spectra moves the raw spectra from the channel to the raw_spectra object (after channel has read it from the file)
   * \param channel
   */
  void move_raw_spectra(std::shared_ptr<channel> chan);

  // /*!
  //  * @brief this is a quick method, used to check the amplitude spectra of the raw data
  //  */
  // void simple_stack_all();

  /*!
   * @brief this is a quick method, used to check the amplitude spectra of the raw data, e.g. use 50% of the data
   \param fraction_to_use 0.1 - 1 - here 0.5 is best, "median limit" is used as a threshold, 1 is all data
   the sa spectra have to be created before! then all spectra found, will be used for stacking
   */
  void advanced_stack_all(const double &fraction_to_use = 1.0);

  // void simple_stack_all_div(const std::shared_ptr<raw_spectra> raw, const std::string &channel_type);

  /*!
   * @brief uses PREVIOUS calculated stacked spectra and smooths it with a parzen window; that can also be advanced stacked or a another method
   */
  void parzen_stack_all();

  std::vector<double> get_abs_sa_spectra(const std::pair<std::string, std::string> &name, const bool is_remote = false, const bool is_emap = false) const;
  std::pair<double, double> get_abs_sa_spectra_min_max(const std::pair<std::string, std::string> &name, const bool is_remote = false, const bool is_emap = false) const;
  std::vector<double> get_abs_sa_prz_spectra(const std::pair<std::string, std::string> &name, const bool is_remote = false, const bool is_emap = false) const;

  std::shared_ptr<fftw_freqs> fft_freqs;          //!< fftw_freqs object, need to know how the incoming spectra have been calculated
  std::shared_ptr<BS::thread_pool> pool;          //!< thread pool from main program
  std::vector<std::shared_ptr<channel>> channels; //!< all channels from the raw file for reference

  std::string get_sensor_name(const std::pair<std::string, std::string> &name) const;
  std::string get_sensor_serial(const std::pair<std::string, std::string> &name) const;
  std::string get_sensor_name_serial(const std::pair<std::string, std::string> &name, const bool cat_underscore = false) const;
  std::string get_sampling_rate(const std::pair<std::string, std::string> &name) const;

  void multiply_sa_spectra(const double &factor);
  void dump_sa_spectra() const;
  void dump_sa_prz_spectra() const;

  double bw = 0; // bandwidth of fft

  spc_base<double> sa;     //!< stack all amplitude spectra from fft
  spc_base<double> sa_prz; //!< stack all amplitude spectra smoothed (parzening) from fft
private:
  void do_advanced_stack_auto(const std::pair<std::string, std::string> &name, const double &fraction_to_use);
  void do_advanced_stack_cross(const std::pair<std::string, std::string> &name, const double &fraction_to_use);
};

// end class raw_spectra

static std::pair<double, double> min_max_sa_spc(const std::vector<std::shared_ptr<raw_spectra>> &raws, const std::pair<std::string, std::string> &name,
                                                const bool is_remote = false, const bool is_emap = false) {

  std::pair<double, double> result(DBL_MIN, DBL_MAX);
  std::vector<double> ampl_max_mins;
  for (const auto &raw : raws) {
    auto mm = raw->get_abs_sa_spectra_min_max(name, is_remote, is_emap);
    ampl_max_mins.push_back(mm.first);
    ampl_max_mins.push_back(mm.second);
  }
  auto r1 = std::minmax_element(ampl_max_mins.begin(), ampl_max_mins.end());
  result.first = *r1.first;
  result.second = *r1.second;
  return result;
}

#endif // RAW_SPECTRA_H

// std::vector<double> get_abs_spectra(const std::string &channel_type, const size_t nstack = 0, const bool is_remote = false, const bool is_emap = false) const;

// void scale_by_stacked_spectra_local(std::vector<double> &nominator, const bool is_remote = false, const bool is_emap = false);

// void simple_ampl_stack_div(const std::string &name, const std::vector<std::vector<std::complex<double>>> &in_nom, const std::vector<std::vector<std::complex<double>>> &in_denom, single_spectra<double> &xsp_out) {

//   if (in_nom.size() != in_denom.size()) {
//     throw std::runtime_error("simple_ampl_stack: in_nom.size() != in_denom.size()");
//   }

//   size_t n = in_nom.at(0).size();  // n = f size
//   std::vector<double> out(n, 0.0); // f size

//   for (size_t i = 0; i < n; ++i) {                       //   for frequencies
//     auto ff = bvec::absv(bvec::get_fslice(in_nom, i));   // get all stacks
//     auto dd = bvec::absv(bvec::get_fslice(in_denom, i)); // get all stacks
//     for (size_t j = 0; j < ff.size(); ++j) {
//       ff[j] /= dd[j];
//     }
//     out[i] = bvec::mean(ff);
//   }
//   xsp_out.add_spectra(name, out);
// }
