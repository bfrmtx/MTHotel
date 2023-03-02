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

#include "BS_thread_pool.h"





int main(int argc, char* argv[])
{

    bool files = false;            // change to true for file output

    unsigned l = 1;
    while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
        std::string marg(argv[l]);

        if (marg.compare("-f") == 0) {
            files = true;
        }
        if (marg.compare("-") == 0) {
            std::cerr << "\nunrecognized option " << argv[l] << std::endl;
            return EXIT_FAILURE;
        }
        ++l;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // make some individual data for each channel and treat all sperately
    // one idea: three different frequencies for one sample frequency

    size_t i, j;
    size_t file_count = 0;


    std::filesystem::path outpath(std::filesystem::temp_directory_path() / "fft_noise_timeser");

    if (files) {
        try {
            std::filesystem::create_directory(outpath);
        }
        catch (...) {
            std::cerr << "could not create " << outpath.string() << std::endl;
            return EXIT_FAILURE;
        }
    }


    std::string basename ("spc_ts_sine");

    std::vector<std::shared_ptr<channel>> channels;
    std::string channel_type("Ex");
    double sample_freq = 1024;
    size_t rl = 1024, wl = 1024;

    channels.emplace_back(std::make_shared<channel>(channel_type, sample_freq));
    channels.emplace_back(std::make_shared<channel>(channel_type, sample_freq));
    channels.emplace_back(std::make_shared<channel>(channel_type, sample_freq));



    size_t dat_sz = 1024 * 64;

    std::vector<std::vector<double>> noise_data;
    for (i = 0; i < channels.size(); ++i) {
        noise_data.emplace_back(std::vector<double>(dat_sz));
    }

    auto pool = std::make_shared<BS::thread_pool>();


    // I add 1% noise floor
    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::normal_distribution<> dist{0,2};  // mean 0, sigma 2
    i = 0;
    for (auto &nd : noise_data) {
        for (auto &v : nd) v = dist(gen) / 100.;
        ++i;
    }

    std::vector<double> sin_freqs(noise_data.size());
    sin_freqs[0] = 180;
    sin_freqs[1] = 102;
    sin_freqs[2] = 64;

    // add the sine to the data
    double sn = 0;
    i = 0;
    for (auto &nd : noise_data) {
        sn = 0;
        for (auto &v : nd) v += (sin(sin_freqs.at(i) * (2 * M_PI) * sn++ / sample_freq));
        ++i;
    }



    const size_t ts_beg = 0, ts_end(sin_freqs.at(0)/4);
    std::vector<double> xax(ts_end - ts_beg);

    std::vector<std::vector<double>> yaxs;
    for (i = 0; i < noise_data.size(); ++i) {
        yaxs.emplace_back(std::vector<double>(ts_end - ts_beg));
    }

    i = 0;
    for (auto &v : xax) v = ts_beg + i++;

    j = 0;
    for (auto &yax : yaxs) {
        i = 0;
        for (auto &v : yax) v = noise_data[j][i++];
        ++j;
    }
    std::string init_err;
    auto gplt_ts = std::make_unique<gnuplotter<double, double>>(init_err);

    if (init_err.size()) {
        std::cout << init_err << std::endl;
        return EXIT_FAILURE;
    }

    if (!files) gplt_ts->set_qt_terminal(basename, ++file_count);
    else gplt_ts->set_svg_terminal(outpath, basename, 1, ++file_count);

    gplt_ts->cmd << "set multiplot layout 3, 1" << std::endl;
    i = 0;
    for (const auto &yax : yaxs) {
        gplt_ts->cmd << "set title 'TS Data, sine \\@ " << sin_freqs[i] << " Hz, f_{sample} " << sample_freq << " Hz'" << std::endl;
        gplt_ts->cmd << "set key off" << std::endl;
        gplt_ts->cmd << "set xlabel 'ts [ms]'" << std::endl;
        gplt_ts->cmd << "set ylabel 'amplitude [mV]'" << std::endl;
        gplt_ts->cmd << "set grid" << std::endl;
        gplt_ts->set_x_range(ts_beg, ts_end);
        gplt_ts->set_y_range(-1.2, 1.2);
        gplt_ts->set_xy_linespoints(xax, yax, "input", 1, 2,"lt rgb \"blue\" pt 6");
        gplt_ts->plot();
        ++i;
    }

    gplt_ts.reset();

    // ********************************+ S P E C T R A *********************************************************************



    // add a channel for zero padding
    channels.emplace_back(std::make_shared<channel>(channels.back()));
    noise_data.emplace_back(noise_data.back());

    std::vector<std::shared_ptr<gnuplotter<double, double>>> vgplt;

    // two different read- and window length
    for (size_t iwl = 0; iwl < 2; ++iwl) {

        if (iwl) {
            wl *= 4;
            rl *= 4;
        }
        //auto gplt = std::make_unique<gnuplotter<double, double>>(init_err);

        for (size_t pl = 0; pl < 3; ++pl) {
            i = 0;
            std::vector<std::shared_ptr<fftw_freqs>> fft_freqs;
            std::vector<std::shared_ptr<raw_spectra>> raws;
            for ( auto &chan : channels) {
                // last zero padding
                if (i == channels.size()-1) fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), wl*4, rl));
                else fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), wl, rl));
                chan->set_fftw_plan(fft_freqs.back());
                raws.emplace_back(std::make_shared<raw_spectra>(pool, fft_freqs.back()));
                ++i;

            }

            // **** here I do the FFT of a single vector - e.g. a noise vector
            i = 0;




            for (auto &chan : channels) {
                // const bool bdetrend_hanning = true
                //                if (pl == 0) chan->read_all_fftw_gussian_noise(noise_data[i++], false);  // could be a thread
                //                else chan->read_all_fftw_gussian_noise(noise_data[i++], true);

                if (pl == 0) pool->push_task(&channel::read_all_fftw_gussian_noise, chan, noise_data[i++], false);
                else pool->push_task(&channel::read_all_fftw_gussian_noise, chan, noise_data[i++], true);

                //std::cout << chan->qspc.size() << " readings" << std::endl;
            }
            pool->wait_for_tasks();





            for (auto &chan : channels) {
                std::cout << chan->qspc.size() << " readings" << std::endl;
            }

            std::vector<double> max_mins;               // adjust x-axis of gnuplot; otherwise scales in log mode to full decade
            auto fft_res_iter = fft_freqs.begin();      // auto iterator example
            i = 0;
            for (auto &chan : channels) {
                auto fft_fres = *fft_res_iter++;
                auto mm = fft_fres->auto_range(0.01, 0.4); // cut off spectra
                max_mins.push_back(mm.first);
                max_mins.push_back(mm.second);
                // const bool bcal = true, const bool bwincal = true is amplitude spectral density
                if (pl < 2) chan->prepare_to_raw_spc(fft_fres, true, false);  // make vector from queue
                else chan->prepare_to_raw_spc(fft_fres, true, true);  // make vector from queue

                raws[i++]->get_raw_spectra(chan->spc, chan->channel_type, chan->bw, chan->is_remote, chan->is_emap); // swap!
            }
            std::sort(max_mins.begin(), max_mins.end()); // of x-axis frequencies

            for (auto &raw : raws) {
                raw->simple_stack_all();
            }
            pool->wait_for_tasks();


            auto ampl_min_max = min_max_sa_spc(raws, channel_type);
            std::cout << ampl_min_max.first << " " << ampl_min_max.second << std::endl;

            //
            double f_or_s;
            std::string unit;
            init_err.clear();
            vgplt.emplace_back(std::make_shared<gnuplotter<double, double>>(init_err));

            if (init_err.size()) {
                std::cout << init_err << std::endl;
                return EXIT_FAILURE;
            }
            auto gplt = vgplt.back();


            // make a copy for data tests and so on **************************************

            std::vector<std::vector<double>> vs(channels.size());
            for (size_t i = 0; i < raws.size(); ++i) {
                vs[i] = raws.at(i)->get_abs_sa_spectra(channel_type);
            }
            i = 0;
            if (pl < 2) {
                // do some changes *****************************************
                for (auto &v : vs) {
                    //for (auto &d : v) d /= (0.5 * fft_freqs.at(i)->get_rl());
                    for (auto &d : v) d *= fft_freqs.at(i)->get_fftw_scale();
                    ++i;
                }
            }
            // pl == 2 is already normalized in chan->prepare_to_raw_spc

            if (!files) gplt->set_qt_terminal(basename, ++file_count);
            else gplt->set_svg_terminal(outpath, basename, 1, ++file_count);

            if (pl == 0) gplt->cmd << "set title 'FFT length, rectangular Window, div T/2'" << std::endl;
            if (pl == 1) gplt->cmd << "set title 'FFT length, Hanning Window, div T/2'" << std::endl;
            if (pl == 2) gplt->cmd << "set title 'FFT length, Hanning Window, div √(f_{sample} * T/2)'" << std::endl;



            gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
            gplt->cmd << "set grid" << std::endl;
            if (pl < 2)  gplt->cmd << "set ylabel 'amplitude [mV]'" << std::endl;
            else gplt->cmd << "set ylabel 'amplitude [mV/√Hz]'" << std::endl;



            gplt->cmd << "set key font \"Hack, 14\"" << std::endl;
            gplt->cmd << "set logscale xy" << std::endl;
            gplt->set_x_range(50, 200);
            gplt->set_y_range(0.0008, 1.5);
            auto fieldw = field_width(fft_freqs);
            for (size_t i = 0; i < channels.size(); ++i) {

                // formatted output
                std::ostringstream label;
                auto delta_f =  (channels.at(i)->get_sample_rate() / ((double)fft_freqs.at(i)->get_rl()));
                mstr::sample_rate_to_str(channels.at(i)->get_sample_rate(), f_or_s, unit, true);
                label << "fs " << std::setw(fieldw) <<  f_or_s << " " << unit << " wl: " << std::setw(fieldw) << fft_freqs.at(i)->get_wl() << " rl: " << std::setw(fieldw) << fft_freqs.at(i)->get_rl() << " Δf: " << std::setw(3) << delta_f;




                if (i == channels.size()-1) gplt->set_xy_linespoints(fft_freqs.at(i)->get_frequencies(), vs[i], label.str(), 2, 2, " lc rgbcolor \"grey\" dashtype 2 pt 12");
                else gplt->set_xy_linespoints(fft_freqs.at(i)->get_frequencies(), vs[i], label.str(), 1, 2, "pt 6");
                //else gplt->set_xy_lines(fft_freqs.at(i)->get_frequencies(), vs[i], label.str(), 1);
            }

            pool->push_task(&gnuplotter<double, double>::plot, gplt);

        }
    }
    pool->wait_for_tasks();

    auto stop_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop_time - start_time);

    std::cout << "Time taken by main: " << duration.count() << " ms" << std::endl;


    if (files) {
        std::cout << std::endl;
        std::cout << "files have been written to " << outpath.string() << std::endl;
        std::cout << std::endl;
    }


    return EXIT_SUCCESS;
}
