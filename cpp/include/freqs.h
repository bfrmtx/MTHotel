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

    T h_scale = (T)2.;
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


class fftw_freqs {

public:

    fftw_freqs(const double &sample_rate, const size_t &wl) : wl(wl), sample_rate(sample_rate){

            // this indices have a size of 513 - that is the output from fftw
            this->idx_range.first = 0; // DC part is at [0]
            this->idx_range.second = this->wl/2 + 1; // if wl = 1024, Nyquist is at [512] which is the 513th element
            //this->wincal = sqrt(1.0/(this->sample_rate * double(this->wl/2) ) );
            double fwl = double(wl);
            this->wincal = sqrt(1.0 / (0.5 * fwl * 0.5) ) * 2.0;
    }

    size_t get_wl() const {
        return this->wl;
    }

    size_t get_fl() const {
        return this->idx_range.second -this->idx_range.first;
    }

    double get_sample_rate() const {
        return this->sample_rate;
    }

    std::pair<double, double> get_frange() const {
        return std::make_pair<double, double>((double(this->idx_range.first) * (this->sample_rate/double(this->wl))),
                                              (double(this->idx_range.second - 1) * (this->sample_rate/double(this->wl))) );
    }

    std::pair<size_t, size_t> index_range() const {
        return this->idx_range;
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

    size_t set_target_freqs(const std::vector<double> &fin) {
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

            if (lf_set) this->targets.emplace_back(f);
            if (f > hf) break;
        }

        return targets.size();

    }

    std::vector<double> get_target_freqs() const {
        return this->targets;
    }

    void scale(std::vector<std::complex<double>> &fftresult) const {
        for (auto &c : fftresult) c *= this->wincal;

    }

    void hanning(std::vector<std::complex<double>> &fftresult) const {

    }

private:

    std::vector<double> targets;
    size_t wl = 0;
    double sample_rate = 0.0;

    std::pair<size_t, size_t> idx_range {0, SIZE_MAX};
    double wincal = 0.0;

};

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
