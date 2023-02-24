#ifndef RAW_SPECTRA_H
#define RAW_SPECTRA_H

#include <memory>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <complex>
#include <algorithm>


#include "freqs.h"
#include "prz_vector.h"
#include "vector_math.h"
#include "BS_thread_pool.h"


void simple_ampl_stack(const std::vector<std::vector<std::complex<double>>> &in, std::vector<double> &out) {
    size_t n = in.at(0).size();  // n = f size
    out.resize(n, 0.0);             // f size


    for (size_t i = 0; i < n; ++i) {  //   for frequencies
         auto ff = bvec::absv(bvec::get_fslice(in, i)); // get all stacks
         out[i] = bvec::mean(ff);
    }

}

void advanced_ampl_stack(const std::vector<std::vector<std::complex<double>>> &in, std::vector<double> &out, const double &fraction_to_use) {

    size_t n = in.at(0).size();  // n = f size
    out.resize(n, 0.0);             // f size


    for (size_t i = 0; i < n; ++i) {  //   for frequencies

        auto ff = bvec::absv(bvec::get_fslice(in, i)); // get all stacks
//        two_pass_variance var;
//        var.variance(ff.cbegin(), ff.cend());
//        out[i] = var.d_mean;
        out[i] = bvec::median_range_mean(ff, fraction_to_use);
        //out[i] = bvec::mean(ff);
    }
}



class raw_spectra
{
public:

    /*!
     * \brief raw_spectra
     * \param fft_freqs copy of a shared pointer
     */

    raw_spectra(std::shared_ptr<fftw_freqs> &fft_freqs);

    void get_raw_spectra(std::vector<std::vector<std::complex<double>>> &swapme, const std::string &channel_type,
                         const double &bw, const bool is_remote = false, const bool is_emap = false);


    void simple_stack_all();

    void advanced_stack_all(const double &fraction_to_use);

    std::future<size_t> advanced_stack_all_t(std::shared_ptr<BS::thread_pool> &pool, const double &fraction_to_use);

    void parzen_stack_all();


    std::vector<double> get_abs_sa_spectra(const std::string &channel_type, const bool is_remote = false, const bool is_emap = false) const;
    std::pair<double, double> get_abs_sa_spectra_min_max(const std::string &channel_type, const bool is_remote = false, const bool is_emap = false) const;
    std::vector<double> get_abs_sa_prz_spectra(const std::string &channel_type, const bool is_remote = false, const bool is_emap = false) const;
    std::vector<double> get_abs_spectra(const std::string &channel_type, const size_t nstack = 0, const bool is_remote = false, const bool is_emap = false) const;



    std::vector<std::vector<std::complex<double>>> ex;  //!< spectra from fft, local or center site, [stacks][frequencies]
    std::vector<std::vector<std::complex<double>>> ey;  //!< spectra from fft, local or center site
    std::vector<std::vector<std::complex<double>>> hx;  //!< spectra from fft, local or center site
    std::vector<std::vector<std::complex<double>>> hy;  //!< spectra from fft, local or center site
    std::vector<std::vector<std::complex<double>>> hz;  //!< spectra from fft, local or center site

    std::vector<std::vector<std::complex<double>>> rhx;  //!< spectra from fft, remote site
    std::vector<std::vector<std::complex<double>>> rhy;  //!< spectra from fft, remote site
    std::vector<std::vector<std::complex<double>>> rhz;  //!< spectra from fft, remote site
    std::vector<std::vector<std::complex<double>>> rex;  //!< spectra from fft, remote site
    std::vector<std::vector<std::complex<double>>> rey;  //!< spectra from fft, remote site

    std::vector<std::vector<std::complex<double>>> eex;  //!< spectra from fft, emap site
    std::vector<std::vector<std::complex<double>>> eey;  //!< spectra from fft, emap site

    std::shared_ptr<fftw_freqs> fft_freqs;
    double bw = 0; // bandwidth of fft

    size_t advanced_ampl_stack_t(const std::vector<std::vector<std::complex<double>>> &in, std::vector<double> &out, const double &fraction_to_use);

private:

    std::vector<double> sa_ex;  //!< stack all spectra from fft, local or center site
    std::vector<double> sa_ey;  //!< stack all spectra from fft, local or center site
    std::vector<double> sa_hx;  //!< stack all spectra from fft, local or center site
    std::vector<double> sa_hy;  //!< stack all spectra from fft, local or center site
    std::vector<double> sa_hz;  //!< stack all spectra from fft, local or center site

    std::vector<double> sa_rhx;  //!< stack all spectra from fft, remote site
    std::vector<double> sa_rhy;  //!< stack all spectra from fft, remote site
    std::vector<double> sa_rhz;  //!< stack all spectra from fft, remote site
    std::vector<double> sa_rex;  //!< stack all spectra from fft, remote site
    std::vector<double> sa_rey;  //!< stack all spectra from fft, remote site

    std::vector<double> sa_eex;  //!< stack all spectra from fft, emap site
    std::vector<double> sa_eey;  //!< stack all spectra from fft, emap site

    // smoothed spectra

    std::vector<double> sa_prz_ex;  //!< stack all spectra smoothed from fft, local or center site
    std::vector<double> sa_prz_ey;  //!< stack all spectra smoothed from fft, local or center site
    std::vector<double> sa_prz_hx;  //!< stack all spectra smoothed from fft, local or center site
    std::vector<double> sa_prz_hy;  //!< stack all spectra smoothed from fft, local or center site
    std::vector<double> sa_prz_hz;  //!< stack all spectra smoothed from fft, local or center site

    std::vector<double> sa_prz_rhx;  //!< stack all spectra smoothed from fft, remote site
    std::vector<double> sa_prz_rhy;  //!< stack all spectra smoothed from fft, remote site
    std::vector<double> sa_prz_rhz;  //!< stack all spectra smoothed from fft, remote site
    std::vector<double> sa_prz_rex;  //!< stack all spectra smoothed from fft, remote site
    std::vector<double> sa_prz_rey;  //!< stack all spectra smoothed from fft, remote site

    std::vector<double> sa_prz_eex;  //!< stack all spectra smoothed from fft, emap site
    std::vector<double> sa_prz_eey;  //!< stack all spectra smoothed from fft, emap site

};


std::pair<double, double> min_max_sa_spc(const std::vector<std::shared_ptr<raw_spectra>> &raws, const std::string &channel_type,
                                         const bool is_remote = false, const bool is_emap = false) {

    std::pair<double, double> result(DBL_MIN, DBL_MAX);
    std::vector<double> ampl_max_mins;
    for (const auto &raw : raws) {
        auto mm  = raw->get_abs_sa_spectra_min_max(channel_type, is_remote, is_emap);
        ampl_max_mins.push_back(mm.first);
        ampl_max_mins.push_back(mm.second);
    }
    auto r1 = std::minmax_element(ampl_max_mins.begin(), ampl_max_mins.end());
    result.first = *r1.first;
    result.second = *r1.second;
    return result;
}

#endif // RAW_SPECTRA_H
