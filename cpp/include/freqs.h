#ifndef FREQS_H
#define FREQS_H

#include <complex>
#include <cstddef>
#include <climits>
#include <cfloat>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <utility>
#include <vector>
#include <iterator>
#include <functional>
#include <sstream>

#include "mt_base.h"
#include "prz_vector.h"
#include "strings_etc.h"

template <typename T, typename Iterator> void detrend(Iterator first, const Iterator last) {
    T dBias       = 0.0;
    T dTrend      = 0.0;
    T dTrendPart1 = 0.0;
    T dTrendPart2 = 0.0;
    T dWindowLen  = 0.0;

    T dCounter;

    // determine the window length
    dWindowLen = (last - first);

    // determine trend
    Iterator iterMidth = first + ( (std::size_t(dWindowLen) / 2));
    dTrendPart1        = accumulate (iterMidth, last,  0.0);
    dTrendPart2        = accumulate (first, iterMidth, 0.0);
    dTrend             = (4.0 / (dWindowLen * dWindowLen)) * (dTrendPart1 - dTrendPart2);

    // detrend
    dCounter = 0.0;
    Iterator iterBegin = first;
    Iterator iterEnd   = last;
    while (iterBegin != iterEnd)
    {
        (*iterBegin) -= (dCounter * dTrend);
        ++dCounter;
        ++iterBegin;
    }
    // determine Bias value
    dBias  = accumulate (first, last, 0.0);
    dBias /= dWindowLen;

    // remove bias
    iterBegin = first;
    iterEnd   = last;
    while (iterBegin != iterEnd)
    {
        (*iterBegin) -= dBias;
        ++iterBegin;
    }


}
template <typename T, typename Iterator> void detrend_and_hanning (Iterator first, const Iterator last) {
    // old procmt detrend - adopted by Martin
    // declaration of variables
    T dBias       = 0.0;
    T dTrend      = 0.0;
    T dTrendPart1 = 0.0;
    T dTrendPart2 = 0.0;
    T dWindowLen  = 0.0;

    T dCounter;

    // determine the window length
    dWindowLen = (last - first);

    // determine trend
    Iterator iterMidth = first + ( (std::size_t(dWindowLen) / 2));
    dTrendPart1        = accumulate (iterMidth, last,  0.0);
    dTrendPart2        = accumulate (first, iterMidth, 0.0);
    dTrend             = (4.0 / (dWindowLen * dWindowLen)) * (dTrendPart1 - dTrendPart2);

    // detrend
    dCounter = 0.0;
    Iterator iterBegin = first;
    Iterator iterEnd   = last;
    while (iterBegin != iterEnd)
    {
        (*iterBegin) -= (dCounter * dTrend);
        ++dCounter;
        ++iterBegin;
    }

    // determine Bias value
    dBias  = accumulate (first, last, 0.0);
    dBias /= dWindowLen;

    // remove bias
    iterBegin = first;
    iterEnd   = last;
    while (iterBegin != iterEnd)
    {
        (*iterBegin) -= dBias;
        ++iterBegin;
    }

    T i = T(0);

    T h_period = M_PI/ ((T)(last - first)), hc = (T)0., hw = (T)0.;

    T h_scale = (T)2.;  // scale of Hanning is 2 for amplitudes (1.63 for Engergy - not used)
    while (first != last) {
        hc = cos(i * h_period);
        hw = h_scale * (1. - hc * hc);
        *first *= hw;                                   // hanning
        ++i;
        ++first;
    }

}

bool is_pow2 (const size_t &wl)
{
    if (!wl) return false;
    return  ((wl & -wl) == wl);
}

size_t next_power_of_two(const size_t &n){
    size_t target, m;
    if (n > (SIZE_MAX - 1) / 2)
        throw "next_power_of_two: vector too large";
    target = n;
    for (m = 1; m < target; m *= 2) {
        if (SIZE_MAX / 2 < m)
            throw "next_power_of_two: vector too large";
    }
    return m;
}

 /*!
     * \brief gen_equidistant_logvector generate a equally logarithic spaced vector
     * \param start from start (included)
     * \param stop   to stop (included)
     * \param steps_per_decade steps per decade (like 11 for MSF-06e or 7 for MFS-07e)
     * \return vector of frequencies (or null vector if failed)
*/
std::vector<double> gen_equidistant_logvector(const double &start, const double &stop, const size_t &steps_per_decade) {

    // dist would be log_stop - log_start
    // we calculate per decade
    std::vector<double> result;

    if ( (start <= 0) || (stop <= 0)) return result;
    if (stop < start) return result;

    double step = 1.0/ double(steps_per_decade);
    size_t i= 0;
    double value = DBL_MAX;

    do {
        double x = start * pow(10.0, double(i++));
        double lx = log10(x);
        for (size_t j = 0; j < steps_per_decade; ++j) {
            value = pow(10.0, lx + step * double(j));
            //Fsetting
            if (value <= stop) result.push_back(value);
        }
    } while (value <= stop);

    if ( (result.back() < stop) && result.size() ) result.push_back(stop);

    return result;
}



/*!
 * \brief The fftw_freqs class stores the FFT parameters for FFTW; the fftw plan is INSIDE the channel as well as the result of the FFTW
 */
class fftw_freqs {

public:

    fftw_freqs(const double &sample_rate, const size_t &wl, const size_t &rl) : wl(wl), rl(rl), sample_rate(sample_rate){

        if (rl > wl) {
            std::string err_str = __func__;
            err_str += ":: read length must be equal or smaller than window length, rl: " + std::to_string(rl) + " wl: " + std::to_string(wl) ;
            throw err_str;
        }
        // this indices have a size of 513 - that is the output from fftw
        this->idx_range.first = 0; // DC part is at [0]
        this->idx_range.second = this->wl/2 + 1; // if wl = 1024, Nyquist is at [512] which is the 513th element
        double frl = double(rl) / 2;
        this->wincal = sqrt(1./(sample_rate * frl) );   // zero padding does not count, take the read length
    }

    double get_bw() const {
        return this->sample_rate/2.;
    }

    double get_wincal() const {
        return this->wincal;
    }

    /*!
     * \brief get_wl window length
     * \return
     */
    size_t get_wl() const {
        return this->wl;
    }

    /*!
     * \brief get_rl get the read length - that is the TRUE corresponding time series segment
     * \return
     */
    size_t get_rl() const {
        return this->rl;
    }

    /*!
     * \brief get_fftw_scale the FFTW scales (multiplies!) in case to get from a +/- sine wave spectral peak of 1 @ f
     * \return
     */
    double get_fftw_scale() const {
        return 2.0 / (double(this->rl));
    }

    /*!
     * \brief get_fl get frequency length of spectral lines of the REMAINING SHORTENED FFT and/or ZERO PADDED FFT
     * \return
     */
    size_t get_fl() const {
        return this->idx_range.second - this->idx_range.first;
    }

    double get_sample_rate() const {
        return this->sample_rate;
    }

    /*!
     * \brief get_delta_f frequency resolution
     * \return
     */
    double get_delta_f() const {
        return this->sample_rate / (double(this->wl));
    }

    std::pair<double, double> get_frange() const {
        return std::make_pair<double, double>((double(this->idx_range.first) * (this->sample_rate/double(this->wl))),
                                              (double(this->idx_range.second - 1) * (this->sample_rate/double(this->wl))) );
    }

    std::pair<size_t, size_t> index_range() const {
        return this->idx_range;
    }

    void set_raw_stacks(const size_t &raw_stacks) {
        this->raw_stacks = raw_stacks;
    }

    size_t get_raw_stacks() const {
       return this->raw_stacks;
    }



    bool is_valid() const {
        if (!this->wl) return false;
        if (this->sample_rate <= 0.0) return false;
        if ((this->idx_range.second - this->idx_range.first) < 1) return false;
        return true;
    }



    std::vector<double> get_frequencies() const {
        std::vector<double> freqs;

        if (!this->is_valid()) return freqs;

        double fwl = double(wl);
        freqs.resize(this->idx_range.second - this->idx_range.first);
        size_t j = 0, i = 0;
        for (i = this->idx_range.first; i < this->idx_range.second; ++i) {
            freqs[j++] = ( double(i) * (this->sample_rate/fwl) );

        }

        return freqs;
    }

    /*!
     * \brief get_frequency_slice get a slice of EXISTING frequencies AFTER FFT
     * \param min_f
     * \param max_f
     * \return
     */
    std::vector<double> get_frequency_slice(const double &min_f, const double &max_f)  {
        std::vector<double> freqs;

        if (!this->is_valid()) return freqs;
         double fwl = double(wl);
        if (min_f < ( double(this->idx_range.first) * (this->sample_rate/fwl) )) return freqs;
        if (min_f > ( double(this->idx_range.second) * (this->sample_rate/fwl) )) return freqs;

        auto all_f = this->get_frequencies();


        auto l = std::lower_bound(all_f.cbegin(), all_f.cend(), min_f);
        auto h = std::upper_bound(l, all_f.cend(), max_f);

        if ((l == h) || (l == all_f.cend())) {
            std::string err_str = __func__;
            err_str += ":: no frequencies found for slice!";
            throw err_str;
            return freqs;

        }
        this->idx_range_slice.first = size_t(l - all_f.cbegin());
        this->idx_range_slice.second = size_t(h - all_f.cbegin()); // we need size; and range iter is always < size



        freqs.resize(this->idx_range_slice.second - this->idx_range_slice.first);
        size_t j = 0, i = 0;
        for (i = this->idx_range_slice.first; i < this->idx_range_slice.second; ++i) {
            freqs[j++] = all_f[i];

        }

        if (!freqs.size()) {
            std::string err_str = __func__;
            err_str += ":: no frequencies found for slice!";
            throw err_str;
        }

        return freqs;
    }

    std::vector<double> get_frequency_slice(const std::pair<double, double> &min_max_f)  {
        return this->get_frequency_slice(min_max_f.first, min_max_f.second);
    }


    std::pair<size_t, size_t> get_index_slice() const  {
        return this->idx_range_slice;
    }


    /*!
     * \brief auto_range
     * \param rel_lower like 0.1
     * \param rel_upper like 0.5 (1.0 == all)
     * \return
     */
    std::pair<double, double> auto_range(const double &rel_lower, const double &rel_upper) {

        if (!this->is_valid()) {
            return std::make_pair<double, double>(0, DBL_MAX);
        }

        this->idx_range.first = 0; // DC part is at [0]
        this->idx_range.second = this->wl/2 + 1; // if wl = 1024, Nyquist is at [512] which is the 513th element

        auto lower = size_t(rel_lower * double(this->wl/2 + 1.));
        auto upper = (this->wl/2 + 1) - size_t((1.0 - rel_upper) * double(this->wl/2 + 1.));

//        if (upper - lower < min_fft_wl) {
//            return std::make_pair<double, double>(0, DBL_MAX);
//        }
        if (lower < 1) lower = 1;   // can't take DC
        this->idx_range.first = lower;
        this->idx_range.second = upper;
        return this->get_frange();
    }

    std::pair<double, double> set_lower_upper_f(const double &lf, const double &hf, const bool include_upper_f) {

        if (!this->is_valid()) {
            return std::make_pair<double, double>(0, DBL_MAX);
        }

        double f = 0.0;
        double fwl = double(this->wl);
        bool lf_set = false;
        bool hf_set = false;


        // reset
        this->idx_range.first = 0; // DC part is at [0]
        this->idx_range.second = this->wl/2 + 1; // if wl = 1024, Nyquist is at [512] which is the 513th element

        for (size_t i = this->idx_range.first; i < this->idx_range.second; ++i) {
            f = ( double(i) * (this->sample_rate/fwl) );
            if (this->almost_equal(f, lf)) {
                this->idx_range.first = i;
                lf_set = true;
            }
            else if ((f > lf) && !lf_set) {
                this->idx_range.first = i;
                lf_set = true;
            }
            if (this->almost_equal(f, hf)) {
                this->idx_range.second = i;
                hf_set = true;
                if ((this->idx_range.second < this->wl/2 + 1) && include_upper_f) ++this->idx_range.second;
            }
            else if ((f > hf) && !hf_set) {
                this->idx_range.second = i;
                hf_set = true;
            }
        }
        if (this->idx_range.second > this->wl/2 + 1) this->idx_range.second = this->wl/2 + 1;
        return this->get_frange();
    }

    /*!
     * \brief trim_fftw_result
     * \return vector according to the selected frequencies
     */
    std::vector<std::complex<double>> trim_fftw_result(const std::vector<std::complex<double>> &in_fftresult) const {
        std::vector<std::complex<double>> fftresult;
        fftresult.reserve(this->idx_range.second - this->idx_range.first);

        auto beg = in_fftresult.cbegin();
        auto end = in_fftresult.cbegin();


        std::advance(beg, this->idx_range.first);
        std::advance(end,  this->idx_range.second);

        fftresult.insert(fftresult.begin(), beg, end);


        return fftresult;
    }

    std::vector<double> iter_freqs(const std::pair<std::vector<std::complex<double>>::iterator, std::vector<std::complex<double>>::iterator> &iter_range, std::vector<std::complex<double>> &fftresult) const {
        std::vector<double> freqs;

        if (!this->is_valid()) return freqs;

        auto n = std::distance(iter_range.first, iter_range.second);

        if (n < 1) return freqs;

        auto starts = std::distance(fftresult.begin(), iter_range.first);
        if (starts < 0) return freqs;

        starts += this->idx_range.first;

        freqs.resize(n);

        double f = 0.0;
        size_t j = 0;
        double fwl = double(this->wl);


        for (size_t i = starts; i < starts + n; ++i) {
            freqs[j++] = ( double(i) * (this->sample_rate/fwl) );
        }

        return freqs;
    }



    std::pair<std::vector<std::complex<double>>::iterator, std::vector<std::complex<double>>::iterator> iter_range(const double &lf, const double &hf, std::vector<std::complex<double>> &fftresult, const bool include_upper_f) const {

        auto range = std::make_pair<std::vector<std::complex<double>>::iterator, std::vector<std::complex<double>>::iterator >(fftresult.begin(), fftresult.begin());

        double f = 0.0;
        double fwl = double(this->wl);

        std::pair <size_t, size_t> fft_idx_range(0, this->idx_range.second - this->idx_range.first);
        size_t j = 0;
        bool lf_set = false;
        bool hf_set = false;

        for (size_t i = this->idx_range.first; i < this->idx_range.second; ++i) {
            f = ( double(i) * (this->sample_rate/fwl) );
            if (this->almost_equal(f, lf)) {

                fft_idx_range.first = j;
                lf_set = true;
            }
            else if ((f > lf) &&  !lf_set) {
                fft_idx_range.first = j;
                lf_set = true;
            }
            if (this->almost_equal(f, hf)) {
                fft_idx_range.second = j;
                hf_set = true;
                if ((fft_idx_range.second < this->idx_range.second - this->idx_range.first + 1) && include_upper_f) ++fft_idx_range.second;
            }
            else if ((f > hf) && !hf_set) {
                fft_idx_range.second = j;
                hf_set = true;
            }
            ++j;

        }
        if (fft_idx_range.second > (this->idx_range.second - this->idx_range.first + 1)) fft_idx_range.second = this->idx_range.second - this->idx_range.first + 1;

        std::advance(range.first, fft_idx_range.first);
        std::advance(range.second,  fft_idx_range.second);


        return range;
    }

    // https://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
    bool almost_equal(double x, double y, int ulp = 2) const
    {
        // the machine epsilon has to be scaled to the magnitude of the values used
        // and multiplied by the desired precision in ULPs (units in the last place)
        return std::fabs(x-y) <= std::numeric_limits<double>::epsilon() * std::fabs(x+y) * ulp
               // unless the result is subnormal
               || std::fabs(x-y) < std::numeric_limits<double>::min();
    }

    template<class T>
    void vplot(const std::vector<T> &v, const int &items_per_line, const int width, const bool invert = false) {

        int n = 0;
        auto tnull = T(0);
        for (const auto &val : v) {
            if (invert) {
                if (val == tnull) std::cout << std::setw(width) << val << " ";
                else std::cout << std::setw(width) <<  T(1)/val << " ";
            }
            else std::cout << std::setw(width) << val << " ";
            ++n;
            if (n / items_per_line) {
                n = 0;
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;
    }

    size_t set_target_freqs(const std::vector<double> &fin, const double &prz_radius) {
        double lf =  ( double(this->idx_range.first) * (this->sample_rate/double(this->wl)));
        double hf =  ( double(this->idx_range.second) * (this->sample_rate/double(this->wl)));
        bool lf_set = false;
        bool hf_set = false;

        for (const auto &f : fin) {
            if (this->almost_equal(f, lf)) {
                lf_set = true;
            }
            else if ((f > lf) &&  !lf_set) {
                lf_set = true;
            }

            if (lf_set) this->target_freqs.emplace_back(f);
            if (f > hf) break;
        }

        this->prz_radius = prz_radius;

        return target_freqs.size();

    }

    std::vector<double> get_target_freqs() const {
        return this->target_freqs;
    }

    void scale(auto &fftresult) const {
        for (auto &c : fftresult) c *= this->wincal;
    }

    /*!
     * \brief unscale for testing purpose
     * \param fftresult
     */
    auto unscale(auto &fftresult) const {
        for (auto &c : fftresult) c /= this->wincal;
    }


    size_t create_parzen_vectors() {
        return parzen_vector(this->get_frequencies(), this->target_freqs, this->prz_radius,
                             this->selected_freqs, this->parzendists);

    }

    std::vector<double> get_selected_frequencies() const {
        return this->selected_freqs;
    }



    double prz_radius = 0.1;
    std::vector<double> selected_freqs;
    std::vector<double> target_freqs;
    std::vector<std::vector<double>> parzendists;

private:

    size_t wl = 0;                      //!< window length of fft
    size_t rl = 0;                      //!< read length of time series data; if same like wl: standard fft, if greater: zero padding
    double sample_rate = 0.0;

    std::pair<size_t, size_t> idx_range {0, SIZE_MAX};
    std::pair<size_t, size_t> idx_range_slice {0, SIZE_MAX};
    double wincal = 0.0;
    size_t raw_stacks = 0;              //!< to amount of received ffts



};

/*!
 * \brief field_width get the max frequency and the witdth of this double number as string - used for gnuplot
 * \param fftws
 * \return 4 for number like 1024
 */
size_t field_width(const std::vector<std::shared_ptr<fftw_freqs>> &fftws) {

    std::vector<double> maxf;
    // rl is always <= wl !
    for (auto const &fs : fftws) {
        maxf.push_back(fs->get_wl());
    }

    std::ostringstream xw;
    xw << std::setprecision(0) << *std::max_element(maxf.begin(), maxf.end());
    return xw.str().size();

}

std::vector<std::stringstream> gnuplot_labels(const std::vector<std::shared_ptr<fftw_freqs>> &fftws, const std::shared_ptr<std::vector<double>> in_rmss = nullptr) {
    std::vector<double> fss(fftws.size());
    std::vector<size_t> wls(fftws.size());
    std::vector<size_t> rls(fftws.size());
    std::vector<double> bws(fftws.size());
    std::vector<double> dfs(fftws.size());
    std::vector<double> rmss;

    std::vector<std::stringstream> ssv(fftws.size());
    size_t i = 0;
    if (in_rmss != nullptr) {
        if (in_rmss->size() != fftws.size()) {
            std::cerr << "gnuplot_labels-> sizes different" << fftws.size() << " " << in_rmss->size() << std::endl;
            return ssv;
        }
        rmss.resize(fftws.size());
        for (const auto v : *in_rmss) rmss[i++] = v;
    }

    i = 0;
    for (const auto &fftw : fftws) {
        fss[i] = fftw->get_sample_rate();
        wls[i] = fftw->get_wl();
        rls[i] = fftw->get_rl();
        bws[i] = fftw->get_bw();
        dfs[i] = fftw->get_delta_f();
        ++i;
    }

    auto sfss = mstr::field_width_right_adjusted_freqs_periods(fss);
    auto swls = mstr::field_width_right_adjusted_ints(wls);
    auto srls = mstr::field_width_right_adjusted_ints(rls);
    auto sbws = mstr::field_width_right_adjusted_freqs_periods(bws);
    auto sdfs = mstr::field_width_right_adjusted_freqs_periods(dfs);


    i = 0;
    for (auto &ss : ssv) {
        ss << "fs: " << sfss.at(i).str();
        ss << " wl: " << swls.at(i).str();
        ss << " rl: " << srls.at(i).str();
        ss << " bw: " << sbws.at(i).str();
        ss << " Δf/Δp: " << sfss.at(i).str();
        ++i;
    }

    if (rmss.size()) {
        i = 0;
        auto srmss = mstr::field_width_right_adjusted_doubles(rmss);
        for (auto &ss : ssv) {
            ss << "rms: " << srmss.at(i).str();
            ++i;
        }
    }

    return ssv;
}

/*

std::pair<std::vector<double>::iterator, std::vector<double>::iterator> index_range_iter() const {
    std::pair<std::vector<double>::iterator, std::vector<double>::iterator> f_iters {thi, DBL_MAX};
}

template<class T>
typename std::enable_if<!std::numeric_limits<T>::is_integer, bool>::type
almost_equal(T x, T y, int ulp)
{
    // the machine epsilon has to be scaled to the magnitude of the values used
    // and multiplied by the desired precision in ULPs (units in the last place)
    return std::fabs(x-y) <= std::numeric_limits<T>::epsilon() * std::fabs(x+y) * ulp
           // unless the result is subnormal
           || std::fabs(x-y) < std::numeric_limits<T>::min();
}


*/


#endif // FREQS_H
