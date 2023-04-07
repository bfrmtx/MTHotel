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

// for beeing accessable as jthreads core math stays outside if possible

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
     * \param fft_freqs copy of a shared pointer; the raw spectra contain trimmed results - so a subset of the complete FFT
     */

    raw_spectra(std::shared_ptr<BS::thread_pool> &pool, std::shared_ptr<fftw_freqs> &fft_freqs) {
        this->pool = pool;
        this->fft_freqs = fft_freqs;
    }

    ~raw_spectra() {
        this->fft_freqs.reset();
        this->pool.reset();
    }

    void get_raw_spectra(std::vector<std::vector<std::complex<double>>> &swapme, const std::string &channel_type,
                         const double &bw, const bool is_remote = false, const bool is_emap = false);


    void simple_stack_all();

    void advanced_stack_all(const double &fraction_to_use);

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
    std::shared_ptr<BS::thread_pool> pool;

    double bw = 0; // bandwidth of fft


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

/*

ex with future  // IF YOU CALL GET the join // wait for finished is called
size_t work()

std::future<size_t> thread_work() {
       return  pool->submit(&raw_spectra::work, std::ref(*this), std::ref(this->ex), std::ref(this->sa_ex), std::ref(fraction_to_use));

with outside func ... std::ref(*this) is missing
        this->pool->push_task(advanced_ampl_stack, std::ref(this->ex), std::ref(this->sa_ex), std::ref(fraction_to_use));

}

in main
            futures.emplace_back(raw->advanced_stack_all_t(pool, 0.8));

    for (auto &f : futures) {
        std::cout << "sz:" << f.get();
    }


with jthreads

//                threads.emplace_back(std::jthread (&raw_spectra::advanced_stack_all, raws[thread_index++], 0.8));
//                //raws[thread_index++]->advanced_stack_all(0.8);


 *
 */
