#include <iostream>
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

std::vector<std::stringstream> ats_filenames_formatted(const std::vector<std::shared_ptr<channel>> channels) {

    std::vector<int> szs;
    szs.reserve(channels.size());

    for (const auto &ch : channels) {
        szs.emplace_back(ch->filename().size());
    }
    auto max = *std::max_element(szs.begin(), szs.end());
    std::vector<std::stringstream> names(channels.size());
    size_t i = 0;
    for (const auto &ch : channels) {
        names[i++] << std::setw(max) << ch->filename();
    }

    return names;
}

int main()
{


    size_t i, j;
    std::filesystem::path home_dir(getenv("HOME"));
    //    auto survey = std::make_shared<survey_d>(home_dir.string() + "/devel/ats_data/Northern_Mining");
    //    auto station = survey->get_station("Sarıçam"); // that is a shared pointer from survey
    //    auto survey = std::make_shared<survey_d>(home_dir.string() + "/devel/ats_data/Eastern_Mining");
    //    auto station = survey->get_station("LH13"); // that is a shared pointer from survey
    auto survey = std::make_shared<survey_d>(home_dir.string() + "/devel/ats_data/theo_noise_mfs06e");
    auto station = survey->get_station("Site_1"); // that is a shared pointer from survey
    std::vector<std::shared_ptr<channel>> channels;
    std::string channel_type("Ex");
    double median_limit = 0.7;  // stack 70% of the data
    try {

        if (!station->runs.size()) {
            std::string err_str = __func__;
            err_str += ":: can not WRITE HEADER!";
            throw err_str;
        }
        for (auto &run : station->runs) {
            channels.emplace_back(run->get_channel(channel_type));
            if (channels.back() == nullptr) channels.pop_back();
        }

    }
    catch (const std::string &error) {

        std::cerr << error << std::endl;
        return EXIT_FAILURE;

    }
    std::vector<std::shared_ptr<fftw_freqs>> fft_freqs;
    std::vector<std::shared_ptr<raw_spectra>> raws;
    auto pool = std::make_shared<BS::thread_pool>();


    try {
        // create the fftw interface with individual window length and read length
        for (auto &chan : channels) {
            // 1 Hz Bandwidth
            if (chan->get_sample_rate() > 256) {
                size_t xwl = size_t(chan->get_sample_rate());
                fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), xwl, xwl));

            }
            else if (chan->get_sample_rate() == 128) {
                size_t xwl = 1024;
                fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), xwl, xwl));

            }
            else {
                size_t xwl = 1024; // zero padding for the lower
                size_t rl = 256;
                fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), xwl, rl));
            }

            // fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), 1024, 1024));


            chan->set_fftw_plan(fft_freqs.back());
            // here each channel is treated as single result - by default it would contain 5 channels
            raws.emplace_back(std::make_shared<raw_spectra>(pool, fft_freqs.back()));

        }

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

    // ******************************** read all fft *******************************************************************************


    try {
        for (auto &chan : channels) {
            //chan->read_all_fftw();
            pool->push_task(&channel::read_all_fftw, chan, false, nullptr);
        }
        pool->wait_for_tasks();

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
        std::cerr << "could not read all fft channels" << std::endl;
        return EXIT_FAILURE;
    }


    fft_res_iter = fft_freqs.begin();
    for (auto &chan : channels) {
        auto fft_fres = *fft_res_iter++;
        auto mima = fft_fres->auto_range(0.01, 0.5); // cut off spectra
        std::cout << "setting range for sampling rate " << chan->get_sample_rate() << "Hz (" << chan->get_sample_rate() << "s) " << mima.first << "Hz (" << 1.0/ mima.first << "s), " <<  mima.second << "Hz (" << 1.0/ mima.second << "s)" << std::endl;
    }
    // ******************************** queue to vector *******************************************************************************

    try {

        fft_res_iter = fft_freqs.begin();
        for (auto &chan : channels) {
            auto fft_fres = *fft_res_iter++;
            // we have a pointer and don't need std::ref
            pool->push_task(&channel::prepare_to_raw_spc, chan, fft_fres, false, false);
        }
        pool->wait_for_tasks();

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
        std::cerr << "could not dequeue all channels" << std::endl;
        return EXIT_FAILURE;
    }




    i = 0;
    for (auto &chan : channels) {
        // swapping should be fast
        raws[i++]->get_raw_spectra(chan->spc, chan->channel_type, chan->bw, chan->is_remote, chan->is_emap);
    }




    // ******************************** stack all *******************************************************************************

    try {
        std::vector<std::jthread> threads;
        for (auto &rw : raws) {
            rw->advanced_stack_all(0.7);
        }
        pool->wait_for_tasks();

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
        std::cerr << "could not stack all channels" << std::endl;
        return EXIT_FAILURE;
    }

    std::string init_err;
    auto gplt = std::make_unique<gnuplotter<double, double>>(init_err);

    if (init_err.size()) {
        std::cout << init_err << std::endl;
        return EXIT_FAILURE;
    }

    gplt->cmd << "set terminal qt size 2048,1600 enhanced" << std::endl;
    gplt->cmd << "set title 'FFT length'" << std::endl;

    gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
    gplt->cmd << "set ylabel 'amplitude [mV/√Hz]'" << std::endl;
    gplt->cmd << "set grid" << std::endl;
    gplt->cmd << "set key font \"Hack, 10\"" << std::endl;
    gplt->cmd << "set logscale xy" << std::endl;

    auto labels = gnuplot_labels(fft_freqs);
    auto names = ats_filenames_formatted(channels);

    j = 0;
    for (const auto &rws : raws) {
        //std::ostringstream label;
        //label << "fs:" << rws->fft_freqs->get_sample_rate() << " wl:" << (int)rws->fft_freqs->get_wl() << " rl:" << (int)rws->fft_freqs->get_rl() << " " << mstr::escape_undersorce(channels.at(j)->filename());
        //label << "fs:" << rws->fft_freqs->get_sample_rate() << " wl:" << (int)rws->fft_freqs->get_wl() << " rl:" << (int)rws->fft_freqs->get_rl() << " " << "file\\_name";
//        auto label = labels.at(j).str();
//        label += " " + mstr::escape_undersorce(channels.at(j)->filename());
        labels[j] << " " << mstr::escape_undersorce(names.at(j).str());
        //std::cout << channels.at(j)->filename() << std::endl;
        gplt->set_xy_lines(rws->fft_freqs->get_frequencies(), rws->get_abs_sa_spectra(channel_type), labels[j].str(), 1);
        ++j;
    }
    gplt->plot();



    return EXIT_SUCCESS;
}
