#ifndef RAW_SPECTRA_H
#define RAW_SPECTRA_H

#include <algorithm>
#include <complex>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "BS_thread_pool.hpp"
#include "channel.hpp"
#include "freqs.hpp"
#include "mt_base.hpp"
#include "prz_vector.hpp"
#include "spc_base.hpp"
#include "vector_math.hpp"

#include <filesystem>
namespace fs = std::filesystem;

// ***************************************************** R A W   S P E C T R A ****************************************************************

/*!
 * @brief The raw_spectra class, is a spc_base (my raw spectra!!) AND functionality for stacking from sa. sa_prz, sa_avg, coh
 * @details the integrated classes are AGAIN spc_base classes, but mostly of simple vector<double> type, for example sa aka "stack all". so thr RESULTS of a processing are stored here.
 */
class raw_spectra : public spc_base<std::vector<std::complex<double>>> {
public:
  /*!
   * @brief raw_spectra for basic creation of the raw_spectra object, the channels (after their fft) are set later, e.g. by move_raw_spectra
   * @details only checks the thread pool pointer HENCE that teh tread pool mostly resides in the MAIN module.
   */
  raw_spectra(std::shared_ptr<BS::thread_pool<BS::tp::none>> &pool) {
    this->pool = pool;
    if (pool == nullptr) {
      throw std::runtime_error("raw_spectra: pool is nullptr");
    }
  }

  /*!
   * @brief calculates the coherency between all channels in this raw_spectra object (permutate all channels, Hx vs Hy, Hx vs Hz, Hy vs Hz and so on)
   sure - we have no "auto"; this function will call the private function do_cross_coherence_raw_spectra
   */
  void coherence_raw_spectra(const size_t &n_f_smooth);

  void coherence_raw_spectra_prz(); // we call it parzen but is a simple spline smoothing

  /*!
   * @brief raw_spectra destructor
   */
  ~raw_spectra() = default;

  /*!
   * \brief move_raw_spectra moves the raw spectra from the channel to the raw_spectra object (after channel has read it from the file)
   * \param channel
   */
  void move_raw_spectra(std::shared_ptr<channel> chan);

  /*!
   * @brief this is a quick method, used to check the amplitude spectra of the raw data, e.g. use 50% of the data
   \param fraction_to_use 0.1 - 1 - here 0.5 is best, "median limit" is used as a threshold, 1 is all data == simple stack all
   the sa spectra have to be created before! then all spectra found, will be used for stacking
   this method will call the private functions do_advanced_stack_auto and do_advanced_stack_cross
   */
  void advanced_stack_all(const double &fraction_to_use = 1.0);

  // void simple_stack_all_div(const std::shared_ptr<raw_spectra> raw, const std::string &channel_type);

  /*!
   * @brief uses PREVIOUS calculated stacked spectra and smooths it with a parzen window; that can also be advanced stacked or a another method, which populates the sa (stack all)
   */
  void parzen_stack_all();

  /*!
   * @brief uses PREVIOUS calculated stacked spectra and smooths it by averaging over n lines of the spectra; that can also be advanced stacked or a another method, which populates the sa (stack all)
   */
  void smooth_stack_all();

  std::pair<double, double> get_abs_sa_spectra_min_max(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const;

  // std::vector<double> get_abs_sa_spectra(const std::pair<std::string, std::string> &name, const bool is_remote = false, const bool is_emap = false) const;
  // std::vector<double> get_abs_sa_prz_spectra(const std::pair<std::string, std::string> &name, const bool is_remote = false, const bool is_emap = false) const;
  // std::vector<double> get_abs_sa_avg_spectra(const std::pair<std::string, std::string> &name, const bool is_remote = false, const bool is_emap = false) const;

  // ************************************************** G E T   S P E C T R A ****************************************************************

  /**
   * @brief Generic function to get spectra data as a vector
   * @param data_container The spc_base container to get from (sa, sa_prz, coh, etc.)
   * @param get_freqs Function to retrieve the appropriate frequencies
   * @param chan_pair Channel pair to search for
   * @return std::vector<double> with the spectra data
   */
  std::vector<double> get_spectra_generic(const spc_base<double> &data_container,
                                          const std::function<std::vector<double>()> &get_freqs,
                                          const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const;

  std::vector<double> get_abs_sa_spectra(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const {
    return get_spectra_generic(this->sa, [this]() { return this->fft_freqs->get_frequencies(); }, chan_pair);
  }
  std::vector<double> get_abs_sa_prz_spectra(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const {
    return get_spectra_generic(this->sa_prz, [this]() { return this->fft_freqs->get_selected_frequencies(); }, chan_pair);
  }
  std::vector<double> get_abs_sa_avg_spectra(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const {
    return get_spectra_generic(this->sa_avg, [this]() { return this->fft_freqs->get_frequencies(); }, chan_pair);
  }
  std::vector<double> get_coh_spectra(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const {
    return get_spectra_generic(this->coh, [this]() { return this->fft_freqs->get_frequencies(); }, chan_pair);
  }
  std::vector<double> get_coh_prz_spectra(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const {
    return get_spectra_generic(this->coh_prz, [this]() { return this->fft_freqs->get_selected_frequencies(); }, chan_pair);
  }
  std::vector<double> get_noise_spectra(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const {
    return get_spectra_generic(this->noise, [this]() { return this->fft_freqs->get_frequencies(); }, chan_pair);
  }
  std::vector<double> get_noise_prz_spectra(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const {
    return get_spectra_generic(this->noise_prz, [this]() { return this->fft_freqs->get_selected_frequencies(); }, chan_pair);
  }

  std::shared_ptr<fftw_freqs> fft_freqs;               //!< fftw_freqs object, need to know how the incoming spectra have been calculated, for ALL spectra
  std::shared_ptr<BS::thread_pool<BS::tp::none>> pool; //!< thread pool from main program
  std::vector<std::shared_ptr<channel>> channels;      //!< all channels from the raw file for reference

  std::string get_sensor_name(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const;
  std::string get_sensor_serial(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const;
  std::string get_sensor_name_serial(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair, const bool cat_underscore = false) const;
  std::string get_sampling_rate(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) const;

  void multiply_sa_spectra(const double &factor);
  // ************************************************** D U M P   S P E C T R A ****************************************************************
  /**
   * @brief Generic dump function for spectra data
   * @param data_container The spc_base container to dump from (sa, sa_prz, coh, etc.)
   * @param spectra_type String identifier for the type of spectra ("", "_prz", "_avg", etc.)
   * @param get_freqs Function to retrieve the appropriate frequencies
   */
  void dump_spectra_generic(const spc_base<double> &data_container, const std::string &spectra_type, const std::function<std::vector<double>()> &get_freqs) const;

  void dump_sa_spectra() const {
    dump_spectra_generic(this->sa, "", [this]() { return this->fft_freqs->get_frequencies(); });
  }

  void dump_sa_prz_spectra() const {
    dump_spectra_generic(this->sa_prz, "_prz", [this]() { return this->fft_freqs->get_selected_frequencies(); });
  }

  void dump_sa_avg_spectra() const {
    dump_spectra_generic(this->sa_avg, "_avg", [this]() { return this->fft_freqs->get_frequencies(); });
  }

  void dump_coh_spectra() const {
    dump_spectra_generic(this->coh, "_coh", [this]() { return this->fft_freqs->get_frequencies(); });
  }

  void dump_coh_prz_spectra() const {
    dump_spectra_generic(this->coh_prz, "_coh_prz", [this]() { return this->fft_freqs->get_selected_frequencies(); });
  }

  void dump_noise_spectra() const {
    dump_spectra_generic(this->noise, "_noise", [this]() { return this->fft_freqs->get_frequencies(); });
  }

  void dump_noise_prz_spectra() const {
    dump_spectra_generic(this->noise_prz, "_noise_prz", [this]() { return this->fft_freqs->get_selected_frequencies(); });
  }

  double bw = 0; // bandwidth of fft

  spc_base<double> sa;     //!< stack all amplitude spectra from fft (map stores shared_ptr<vector<double>> per channel pair)
  spc_base<double> sa_prz; //!< stack all amplitude spectra smoothed (parzening) from fft
  spc_base<double> sa_avg; //!< stack all amplitude spectra smoothed by averaging simply over n lines around the center, must be odd!
  // coh will initialized by coherence_raw_spectra function, so by itself; but we know the spectra until that point
  spc_base<double> coh; //!< coherency data, can be simple averaged; we can not use a real parzening here.; recommended is 4 to 8 lines
  // the later classes will be initialized by the coherence_raw_spectra function
  spc_base<double> coh_prz;   //!< coherency data interpolated to target frequencies; I call it "parzen" because this will be done e.g. after parzening
  spc_base<double> noise;     //!< noise data, derived from coherence
  spc_base<double> noise_prz; //!< noise data, derived from coherence, interpolated to target frequencies

  void load_from_rundir_result(const std::string &which_spectra_type, const std::filesystem::path &top_dir);

private:
  void do_advanced_stack_auto(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair, const double &fraction_to_use);
  void do_advanced_stack_cross(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair, const double &fraction_to_use);
  void do_cross_coherence_raw_spectra(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair, const size_t &chunk_size);
  void do_coh_noise_prz(const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair);
};

// end class raw_spectra

inline std::pair<double, double> min_max_sa_spc(const std::vector<std::shared_ptr<raw_spectra>> &raws, const std::pair<std::shared_ptr<channel>, std::shared_ptr<channel>> &chan_pair) {

  std::pair<double, double> result(DBL_MIN, DBL_MAX);
  std::vector<double> ampl_max_mins;
  for (const auto &raw : raws) {
    auto mm = raw->get_abs_sa_spectra_min_max(chan_pair);
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
