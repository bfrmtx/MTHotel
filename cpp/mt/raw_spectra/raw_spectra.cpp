#include "raw_spectra.h"

raw_spectra::raw_spectra(std::shared_ptr<fftw_freqs> &fft_freqs)
{
    this->fft_freqs = fft_freqs;
}

void raw_spectra::get_raw_spectra(std::vector<std::vector<std::complex<double> > > &swapme, const std::string &channel_type,
                                  const double &bw, const bool is_remote, const bool is_emap)
{
    this->bw = bw;
    if (channel_type == "Ex") {
        if (!is_emap && !is_remote) std::swap(swapme, this->ex);
        else if (is_emap && !is_remote) std::swap(swapme, this->eex);
        else if (!is_emap && is_remote) std::swap(swapme, this->rex);
    }

    if (channel_type == "Ey") {
        if (!is_emap && !is_remote) std::swap(swapme, this->ey);
        else if (is_emap && !is_remote) std::swap(swapme, this->eey);
        else if (!is_emap && is_remote) std::swap(swapme, this->rey);
    }

    if (channel_type == "Hx") {
        if (!is_emap && !is_remote) std::swap(swapme, this->hx);
        else if (!is_emap && is_remote) std::swap(swapme, this->rhx);
    }

    if (channel_type == "Hy") {
        if (!is_emap && !is_remote) std::swap(swapme, this->hy);
        else if (!is_emap && is_remote) std::swap(swapme, this->rhy);
    }

    if (channel_type == "Hz") {
        if (!is_emap && !is_remote) std::swap(swapme, this->hz);
        else if (!is_emap && is_remote) std::swap(swapme, this->rhz);
    }



}

void raw_spectra::simple_stack_all()
{
    std::vector<std::jthread> threads;

    if (this->ex.size()) {
        threads.emplace_back(simple_ampl_stack, std::ref(this->ex), std::ref(this->sa_ex));
    }
    if (this->ey.size()) {
        threads.emplace_back(simple_ampl_stack, std::ref(this->ey), std::ref(this->sa_ey));
    }
    if (this->hx.size()) {
        threads.emplace_back(simple_ampl_stack, std::ref(this->hx), std::ref(this->sa_hx));
    }
    if (this->hy.size()) {
        threads.emplace_back(simple_ampl_stack, std::ref(this->hy), std::ref(this->sa_hy));
    }
    if (this->hz.size()) {
        threads.emplace_back(simple_ampl_stack, std::ref(this->hz), std::ref(this->sa_hz));
    }


    if (this->rex.size()) {
        threads.emplace_back(simple_ampl_stack, std::ref(this->rex), std::ref(this->sa_rex));
    }
    if (this->rey.size()) {
        threads.emplace_back(simple_ampl_stack, std::ref(this->rey), std::ref(this->sa_rey));
    }
    if (this->rhx.size()) {
        threads.emplace_back(simple_ampl_stack, std::ref(this->rhx), std::ref(this->sa_rhx));
    }
    if (this->rhy.size()) {
        threads.emplace_back(simple_ampl_stack, std::ref(this->rhy), std::ref(this->sa_rhy));
    }
    if (this->rhz.size()) {
        threads.emplace_back(simple_ampl_stack, std::ref(this->rhz), std::ref(this->sa_rhz));
    }

    if (this->eex.size()) {
        threads.emplace_back(simple_ampl_stack, std::ref(this->eex), std::ref(this->sa_eex));
    }
    if (this->eey.size()) {
        threads.emplace_back(simple_ampl_stack, std::ref(this->eey), std::ref(this->sa_eey));
    }

}

void raw_spectra::advanced_stack_all(const double &fraction_to_use)
{
    std::vector<std::jthread> threads;

    if (this->ex.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->ex), std::ref(this->sa_ex), std::ref(fraction_to_use)));
    }
    if (this->ey.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->ey), std::ref(this->sa_ey), std::ref(fraction_to_use)));
    }
    if (this->hx.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->hx), std::ref(this->sa_hx), std::ref(fraction_to_use)));
    }
    if (this->hy.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->hy), std::ref(this->sa_hy), std::ref(fraction_to_use)));
    }
    if (this->hz.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->hz), std::ref(this->sa_hz), std::ref(fraction_to_use)));
    }


    if (this->rex.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->rex), std::ref(this->sa_rex), std::ref(fraction_to_use)));
    }
    if (this->rey.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->rey), std::ref(this->sa_rey), std::ref(fraction_to_use)));
    }
    if (this->rhx.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->rhx), std::ref(this->sa_rhx), std::ref(fraction_to_use)));
    }
    if (this->rhy.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->rhy), std::ref(this->sa_rhy), std::ref(fraction_to_use)));
    }
    if (this->rhz.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->rhz), std::ref(this->sa_rhz), std::ref(fraction_to_use)));
    }

    if (this->eex.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->eex), std::ref(this->sa_eex), std::ref(fraction_to_use)));
    }
    if (this->eey.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->eey), std::ref(this->sa_eey), std::ref(fraction_to_use)));
    }

}

size_t raw_spectra::advanced_ampl_stack_t(const std::vector<std::vector<std::complex<double> > > &in, std::vector<double> &out, const double &fraction_to_use) {
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

    return out.size();
}


std::future<size_t> raw_spectra::advanced_stack_all_t(std::shared_ptr<BS::thread_pool> &pool, const double &fraction_to_use)
{
    // pool.push_task(&raw_spectra::simple_stack_all, raw);


    if (this->ex.size()) {
       return  pool->submit(&raw_spectra::advanced_ampl_stack_t, std::ref(*this), std::ref(this->ex), std::ref(this->sa_ex), std::ref(fraction_to_use));
        //this->advanced_ampl_stack_t(this->ex, this->sa_ex, fraction_to_use);
        //threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->ex), std::ref(this->sa_ex), std::ref(fraction_to_use)));
    }


/*    if (this->ey.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->ey), std::ref(this->sa_ey), std::ref(fraction_to_use)));
    }
    if (this->hx.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->hx), std::ref(this->sa_hx), std::ref(fraction_to_use)));
    }
    if (this->hy.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->hy), std::ref(this->sa_hy), std::ref(fraction_to_use)));
    }
    if (this->hz.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->hz), std::ref(this->sa_hz), std::ref(fraction_to_use)));
    }


    if (this->rex.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->rex), std::ref(this->sa_rex), std::ref(fraction_to_use)));
    }
    if (this->rey.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->rey), std::ref(this->sa_rey), std::ref(fraction_to_use)));
    }
    if (this->rhx.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->rhx), std::ref(this->sa_rhx), std::ref(fraction_to_use)));
    }
    if (this->rhy.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->rhy), std::ref(this->sa_rhy), std::ref(fraction_to_use)));
    }
    if (this->rhz.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->rhz), std::ref(this->sa_rhz), std::ref(fraction_to_use)));
    }

    if (this->eex.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->eex), std::ref(this->sa_eex), std::ref(fraction_to_use)));
    }
    if (this->eey.size()) {
        threads.emplace_back(std::jthread(advanced_ampl_stack, std::ref(this->eey), std::ref(this->sa_eey), std::ref(fraction_to_use)));
    }
    */

    return std::future<size_t>();
}


void raw_spectra::parzen_stack_all()
{
    std::vector<std::jthread> threads;
    // size_t parzen(const std::vector<T> &data, const std::vector<S> &selected_freqs, const std::vector<std::vector<S>> &parzendists, std::vector<T> &result ) {
    if (this->sa_ex.size()) {
        threads.emplace_back(std::jthread(parzen<double, double>, std::ref(this->sa_ex), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_ex)));
    }
    if (this->sa_ey.size()) {
        threads.emplace_back(std::jthread(parzen<double, double>, std::ref(this->sa_ey), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_ey)));
    }
    if (this->sa_hx.size()) {
        threads.emplace_back(std::jthread(parzen<double, double>, std::ref(this->sa_hx), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_hx)));
    }
    if (this->sa_hy.size()) {
        threads.emplace_back(std::jthread(parzen<double, double>, std::ref(this->sa_hy), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_hy)));
    }
    if (this->sa_hz.size()) {
        threads.emplace_back(std::jthread(parzen<double, double>, std::ref(this->sa_hz), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_hz)));
    }
    if (this->sa_rex.size()) {
        threads.emplace_back(std::jthread(parzen<double, double>, std::ref(this->sa_rex), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_rex)));
    }
    if (this->sa_rey.size()) {
        threads.emplace_back(std::jthread(parzen<double, double>, std::ref(this->sa_rey), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_rey)));
    }
    if (this->sa_rhx.size()) {
        threads.emplace_back(std::jthread(parzen<double, double>, std::ref(this->sa_rhx), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_rhx)));
    }
    if (this->sa_rhy.size()) {
        threads.emplace_back(std::jthread(parzen<double, double>, std::ref(this->sa_rhy), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_rhy)));
    }
    if (this->sa_rhz.size()) {
        threads.emplace_back(std::jthread(parzen<double, double>, std::ref(this->sa_rhz), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_rhz)));
    }
    if (this->sa_eex.size()) {
        threads.emplace_back(std::jthread(parzen<double, double>, std::ref(this->sa_eex), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_eex)));
    }
    if (this->sa_eey.size()) {
        threads.emplace_back(std::jthread(parzen<double, double>, std::ref(this->sa_eey), std::ref(this->fft_freqs->selected_freqs), std::ref(this->fft_freqs->parzendists), std::ref(this->sa_prz_eey)));
    }

}

std::vector<double> raw_spectra::get_abs_sa_spectra(const std::string &channel_type, const bool is_remote, const bool is_emap) const
{
    if (channel_type == "Ex") {
        if (!is_emap && !is_remote && this->sa_ex.size()) {
            return this->sa_ex;
        }
        else if (is_emap && !is_remote && this->sa_eex.size()) {
            return this->sa_eex;
        }
        else if (!is_emap && is_remote && this->sa_rex.size()) {
            return this->sa_rex;
        }
    }


    if (channel_type == "Ey") {
        if (!is_emap && !is_remote && this->sa_ey.size()) {
            return this->sa_ey;
        }
        else if (is_emap && !is_remote && this->sa_eey.size()) {
            return this->sa_eey;
        }
        else if (!is_emap && is_remote && this->sa_rey.size()) {
            return this->sa_rey;
        }
    }

    if (channel_type == "Hx") {
        if (!is_emap && !is_remote && this->sa_hx.size()) {
            return this->sa_hx;
        }
        else if (!is_emap && is_remote && this->sa_rhx.size()) {
            return this->sa_rhx;
        }
    }

    if (channel_type == "Hy") {
        if (!is_emap && !is_remote && this->sa_hy.size()) {
            return this->sa_hy;
        }
        else if (!is_emap && is_remote && this->sa_rhy.size()) {
            return this->sa_rhy;
        }
    }

    if (channel_type == "Hz") {
        if (!is_emap && !is_remote && this->sa_hz.size()) {
            return this->sa_hz;
        }
        else if (!is_emap && is_remote && this->sa_rhz.size()) {
            return this->sa_rhz;
        }
    }

    return  std::vector<double>();

}

std::pair<double, double> raw_spectra::get_abs_sa_spectra_min_max(const std::string &channel_type, const bool is_remote, const bool is_emap) const
{
    std::pair<double, double> result(DBL_MIN, DBL_MAX);
    auto v = this->get_abs_sa_spectra(channel_type, is_remote, is_emap);
    auto xmm = std::minmax_element(v.cbegin(), v.cend());
    result.first = *xmm.first;
    result.second = *xmm.second;

    return result;

}

std::vector<double> raw_spectra::get_abs_sa_prz_spectra(const std::string &channel_type, const bool is_remote, const bool is_emap) const
{
    if (channel_type == "Ex") {
        if (!is_emap && !is_remote && this->sa_prz_ex.size()) {
            return this->sa_prz_ex;
        }
        else if (is_emap && !is_remote && this->sa_prz_eex.size()) {
            return this->sa_prz_eex;
        }
        else if (!is_emap && is_remote && this->sa_prz_rex.size()) {
            return this->sa_prz_rex;
        }
    }


    if (channel_type == "Ey") {
        if (!is_emap && !is_remote && this->sa_prz_ey.size()) {
            return this->sa_prz_ey;
        }
        else if (is_emap && !is_remote && this->sa_prz_eey.size()) {
            return this->sa_prz_eey;
        }
        else if (!is_emap && is_remote && this->sa_prz_rey.size()) {
            return this->sa_prz_rey;
        }
    }

    if (channel_type == "Hx") {
        if (!is_emap && !is_remote && this->sa_prz_hx.size()) {
            return this->sa_prz_hx;
        }
        else if (!is_emap && is_remote && this->sa_prz_rhx.size()) {
            return this->sa_prz_rhx;
        }
    }

    if (channel_type == "Hz") {
        if (!is_emap && !is_remote && this->sa_prz_hz.size()) {
            return this->sa_prz_hz;
        }
        else if (!is_emap && is_remote && this->sa_prz_rhz.size()) {
            return this->sa_prz_rhz;
        }
    }

    return  std::vector<double>();
}

std::vector<double> raw_spectra::get_abs_spectra(const std::string &channel_type, const size_t nstack, const bool is_remote, const bool is_emap) const
{
    std::vector<double> abs_spc;
    std::vector<std::complex<double>>::const_iterator citer_b;
    std::vector<std::complex<double>>::const_iterator citer_e(citer_b);

    if (channel_type == "Ex") {
        if (!is_emap && !is_remote && (this->ex.size() > nstack)) {
            citer_b = this->ex.at(nstack).cbegin();
            citer_e = this->ex.at(nstack).cend();
        }
        else if (is_emap && !is_remote && (this->eex.size() > nstack)) {
            citer_b = this->eex.at(nstack).cbegin();
            citer_e = this->eex.at(nstack).cend();
        }
        else if (!is_emap && is_remote && (this->rex.size()) > nstack) {
            citer_b = this->rex.at(nstack).cbegin();
            citer_e = this->rex.at(nstack).cend();
        }
    }

    if (channel_type == "Ey") {
        if (!is_emap && !is_remote && (this->ey.size() > nstack)) {
            citer_b = this->ey.at(nstack).cbegin();
            citer_e = this->ey.at(nstack).cend();
        }
        else if (is_emap && !is_remote && (this->eey.size() > nstack)) {
            citer_b = this->eey.at(nstack).cbegin();
            citer_e = this->eey.at(nstack).cend();
        }
        else if (!is_emap && is_remote && (this->rey.size() > nstack)) {
            citer_b = this->rey.at(nstack).cbegin();
            citer_e = this->rey.at(nstack).cend();
        }
    }


    if (channel_type == "Hx") {
        if (!is_emap && !is_remote && (this->hx.size() > nstack)) {
            citer_b = this->hx.at(nstack).cbegin();
            citer_e = this->hx.at(nstack).cend();
        }
        else if (!is_emap && is_remote && (this->rhx.size() > nstack)) {
            citer_b = this->rhx.at(nstack).cbegin();
            citer_e = this->rhx.at(nstack).cend();

        }
    }

    if (channel_type == "Hy") {
        if (!is_emap && !is_remote && (this->hy.size() > nstack)) {
            citer_b = this->hy.at(nstack).cbegin();
            citer_e = this->hy.at(nstack).cend();
        }
        else if (!is_emap && is_remote && (this->rhy.size() > nstack)) {
            citer_b = this->rhy.at(nstack).cbegin();
            citer_e = this->rhy.at(nstack).cend();

        }
    }

    if (channel_type == "Hz") {
        if (!is_emap && !is_remote && (this->hz.size() > nstack)) {
            citer_b = this->hz.at(nstack).cbegin();
            citer_e = this->hz.at(nstack).cend();
        }
        else if (!is_emap && is_remote && (this->rhz.size() > nstack)) {
            citer_b = this->rhz.at(nstack).cbegin();
            citer_e = this->rhz.at(nstack).cend();

        }
    }

    abs_spc.reserve(std::distance(citer_b, citer_e));

    while (citer_b != citer_e) {
        abs_spc.push_back(std::fabs(*citer_b++));
    }

    return abs_spc;


}



