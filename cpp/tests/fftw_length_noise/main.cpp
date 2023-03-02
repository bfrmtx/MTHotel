#include <iostream>
#include <iomanip>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <complex>
#include <chrono>
#include <filesystem>
#include <algorithm>
#include <survey.h>
#include "atss.h"
#include "freqs.h"
#include <fftw3.h>

#include <random>
#include "raw_spectra.h"
#include "gnuplotter.h"





int main()
{


    size_t i;



    std::vector<std::shared_ptr<channel>> channels;
    std::string channel_type("Ex");
    double sample_freq = 1024;
    channels.emplace_back(std::make_shared<channel>(channel_type, sample_freq));
    channels.emplace_back(std::make_shared<channel>(channel_type, sample_freq));
    channels.emplace_back(std::make_shared<channel>(channel_type, sample_freq));

    std::vector<std::shared_ptr<fftw_freqs>> fft_freqs;
    std::vector<std::shared_ptr<raw_spectra>> raws;
    auto pool = std::make_shared<BS::thread_pool>();


    i = 0;
    size_t rl, wl = 1024;
    rl = wl;
    try {
        // create the fftw interface with individual window length and read length
        for (auto &chan : channels) {
            fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), wl, rl));
            wl *= 4;
            rl = wl;
            ++i;
            chan->set_fftw_plan(fft_freqs.back());
            // here each channel is treated as single result - by default it would contain 5 channels
            raws.emplace_back(std::make_shared<raw_spectra>(pool, fft_freqs.back()));

        }
        // add a zero padded
        wl = 4096;
        rl = 1024;

        channels.emplace_back(std::make_shared<channel>(channel_type, 1024));
        fft_freqs.emplace_back(std::make_shared<fftw_freqs>(channels.back()->get_sample_rate(), wl, rl));
        channels.back()->set_fftw_plan(fft_freqs.back());
        // here each channel is treated as single result - by default it would contain 5 channels
        raws.emplace_back(std::make_shared<raw_spectra>(pool, fft_freqs.back()));
    }
    catch (const std::string &error) {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;

    }
    catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << std::endl;
        return EXIT_FAILURE;

    }
    catch (...) {
        std::cerr << "could not allocate all channels" << std::endl;
        return EXIT_FAILURE;

    }

    double f_or_s;
    std::string unit;

    // example usage  fft with auto & channels
    auto fft_res_iter = fft_freqs.begin();
    for (const auto &chan : channels) {
        auto fft_fres = *fft_res_iter++;
        mstr::sample_rate_to_str(chan->get_sample_rate(), f_or_s, unit);
        std::cout << "use sample rates of " << f_or_s << " " << unit <<  " wl:" << fft_fres->get_wl() << "  read length:" << fft_fres->get_rl() <<  std::endl;
        chan->set_fftw_plan(fft_fres);
    }


    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::normal_distribution<> dist{5,2};
    std::vector<double> noise_data(16385 * 512);
    double sin_freq = 60;
    double sn = 0;

    //        for (auto &nd : noise_data) {
    //            nd = dist(gen) + (sin(sin_freq * (2 * M_PI) * sn++ / sample_freq)) / 10.0;
    //        }

    for (auto &nd : noise_data) {
        nd = (sin(sin_freq * (2 * M_PI) * sn++ / sample_freq));
    }

    // **** here I do the FFT
    for (auto &chan : channels) {
        chan->read_all_fftw_gussian_noise(noise_data, true);
        std::cout << chan->qspc.size() << " readings" << std::endl;

    }

    std::string init_err_ts;
    auto gplt_ts = std::make_unique<gnuplotter<double, double>>(init_err_ts);

    if (init_err_ts.size()) {
        std::cout << init_err_ts << std::endl;
        return EXIT_FAILURE;
    }

    size_t ts_beg = 0, ts_end(sin_freq);
    std::vector<double> xax(ts_end - ts_beg);
    std::vector<double> yax(ts_end - ts_beg);
    i = 0;
    for (auto &v : yax) {
        xax[i] = ts_beg + i;
        v = noise_data[i++];
    }

    gplt_ts->cmd << "set terminal qt size 2048,1600 enhanced" << std::endl;
    gplt_ts->cmd << "set title 'TS Data'" << std::endl;
    //gplt->cmd << "set key off" << std::endl;
    gplt_ts->cmd << "set xlabel 'ts'" << std::endl;
    gplt_ts->cmd << "set ylabel 'amplitude [mV]'" << std::endl;
    gplt_ts->cmd << "set grid" << std::endl;
    gplt_ts->cmd << "set key font \"Hack, 10\"" << std::endl;

    gplt_ts->set_x_range(ts_beg, ts_end);
    //gplt->set_y_range(ampl_max_min.first * 0.8, ampl_max_min.second * 0.8);
    gplt_ts->set_xy_lines(xax, yax, "input", 1);
    gplt_ts->plot();
    gplt_ts.reset();

    std::vector<double> max_mins;               // adjust x-axis of gnuplot; otherwise scales in log mode to full decade
    fft_res_iter = fft_freqs.begin();           // auto iterator example
    for (auto &chan : channels) {
        auto fft_fres = *fft_res_iter++;
        auto mm = fft_fres->auto_range(0.01, 0.5); // cut off spectra
        max_mins.push_back(mm.first);
        max_mins.push_back(mm.second);
        // bwin cal
        chan->prepare_to_raw_spc(fft_fres, true, false);
    }
    std::sort(max_mins.begin(), max_mins.end());

    i = 0;
    for (auto &chan : channels) {
        raws[i++]->get_raw_spectra(chan->spc, chan->channel_type, chan->bw, chan->is_remote, chan->is_emap);
    }

    for (auto &raw : raws) {
        raw->simple_stack_all();
    }

    auto ampl_max_min = min_max_sa_spc(raws, channel_type);
    //
    std::string init_err;
    auto gplt = std::make_unique<gnuplotter<double, double>>(init_err);

    if (init_err.size()) {
        std::cout << init_err << std::endl;
        return EXIT_FAILURE;
    }

    // make a copy for data tests and so on **************************************

    std::vector<std::vector<double>> vs(channels.size());
    for (size_t i = 0; i < raws.size(); ++i) {
        vs[i] = raws.at(i)->get_abs_sa_spectra(channel_type);
    }
    // do some changes *****************************************
    // unscaled
    i = 0;
    for (auto &v : vs) {
        for (auto &d : v) d /= (0.5*fft_freqs.at(i)->get_rl());
        //for (auto &d : v) d /= (double)fft_freqs.at(i)->get_raw_stacks();
        ++i;
    }

    gplt->cmd << "set terminal qt size 2048,1600 enhanced" << std::endl;
    gplt->cmd << "set title 'FFT length'" << std::endl;
    //gplt->cmd << "set key off" << std::endl;
    gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
    gplt->cmd << "set ylabel 'amplitude [mV/√Hz]'" << std::endl;
    gplt->cmd << "set grid" << std::endl;
    gplt->cmd << "set key font \"Hack, 10\"" << std::endl;
    //  gplt->cmd << "set xtics (170, 180, 190)" << std::endl;

    //gplt->cmd << "set logscale xy" << std::endl;
    //gplt->set_x_range(max_mins.front(), max_mins.back());
    gplt->set_x_range(sin_freq - 20., sin_freq + 20);
    // gplt->set_y_range(0 , 2);
    // gplt->set_y_range(ampl_max_min.first * 0.8, ampl_max_min.second * 1.2);

    auto fieldw = field_width(fft_freqs);


    for (size_t i = 0; i < channels.size(); ++i) {

        // formatted output
        std::ostringstream label;
        auto delta_f =  (channels.at(i)->get_sample_rate() / ((double)fft_freqs.at(i)->get_rl()));
        mstr::sample_rate_to_str(channels.at(i)->get_sample_rate(), f_or_s, unit, true);
        label << "fs " << std::setw(fieldw) <<  f_or_s << " " << unit << " wl: " << std::setw(fieldw) << fft_freqs.at(i)->get_wl() << " rl: " << std::setw(fieldw) << fft_freqs.at(i)->get_rl() << " Δf: " << std::setw(6) << delta_f;



        if (delta_f == 1 && !i) gplt->set_xy_points(fft_freqs.at(i)->get_frequencies(), vs[i], label.str(), 2, "pt 6");
        else if (delta_f == 1 && i) gplt->set_xy_linespoints(fft_freqs.at(i)->get_frequencies(), vs[i], label.str(), 2, 2, "pt 9");
        else gplt->set_xy_lines(fft_freqs.at(i)->get_frequencies(), vs[i], label.str(), 1);
    }

    gplt->plot();
    gplt.reset();







    return EXIT_SUCCESS;
}
