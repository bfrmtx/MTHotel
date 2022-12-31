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
#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

int main()
{


    size_t i, j;
    fs::path home_dir(getenv("HOME"));
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
            raws.emplace_back(std::make_shared<raw_spectra>(fft_freqs.back()));

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
        std::vector<std::jthread> threads;
        for (auto &chan : channels) {
            //chan->read_all_fftw();
            threads.emplace_back(std::jthread (&channel::read_all_fftw, chan, false, nullptr));
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
        std::cerr << "could not read all channels" << std::endl;
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
        std::vector<std::jthread> threads;

        fft_res_iter = fft_freqs.begin();
        for (auto &chan : channels) {
            auto fft_fres = *fft_res_iter++;
            //chan->prepare_to_raw_spc(fft_fres, false);
            // we have a pointer and don't need std::ref
            threads.emplace_back(std::jthread (&channel::prepare_to_raw_spc, chan, fft_fres, false));

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
        std::cerr << "could not dequeue all channels" << std::endl;
        return EXIT_FAILURE;
    }




    i = 0;
    for (auto &chan : channels) {
        // swapping should be fast
        raws[i++]->get_raw_spectra(chan->spc, chan->channel_type, chan->is_remote, chan->is_emap);
    }




    // ******************************** stack all *******************************************************************************

    try {
        std::vector<std::jthread> threads;
        for (auto &rw : raws) {
            threads.emplace_back(std::jthread (&raw_spectra::advanced_stack_all, rw, std::ref(median_limit)));
            //rw->advanced_stack_all(0.7);
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
        std::cerr << "could not stack all channels" << std::endl;
        return EXIT_FAILURE;
    }



    plt::title("FFT length");

    i = 0;
    j = 0;
    std::vector<std::string> marks{"b-", "r-", "g-", "c-", "m-", "b--", "r--", "g--", "c--", "m--"};
    for (const auto &rws : raws) {

        std::string lab =  "fs: " +  std::to_string(rws->fft_freqs->get_sample_rate()) + " wl:" + std::to_string((int)rws->fft_freqs->get_wl()) + " rl:" + std::to_string((int)rws->fft_freqs->get_rl()) + " " + channels.at(j++)->filename();
        //if (i == zp) lab += " zp";
        //if (i == lw) lab += " lw";
        //plt::named_loglog(lab, rws->fft_freqs->get_selected_frequencies(), rws->get_abs_sa_prz_spectra(channel_type), marks.at(i));
        plt::named_loglog(lab, rws->fft_freqs->get_frequencies(), rws->get_abs_sa_spectra(channel_type), marks.at(i));

        ++i;
        if (i == marks.size()) i = 0;

    }
//    for (size_t i = 0; i < channels.size(); ++i) {

//        if ((fft_freqs.at(i)->get_wl() == 4096) && (fft_freqs.at(i)->get_rl() == 1024)) {
//            plt::named_loglog("padded 1024->4096", fft_freqs.at(i)->get_frequencies(), raws.at(i)->get_abs_sa_spectra(channel_type), "r+");
//        }
//        else if ((fft_freqs.at(i)->get_wl() == 1024) && (fft_freqs.at(i)->get_rl() == 1024)) {
//            plt::named_loglog("1024", fft_freqs.at(i)->get_frequencies(), raws.at(i)->get_abs_sa_spectra(channel_type), "bo");
//        }

//        else plt::named_loglog(std::to_string(fft_freqs.at(i)->get_wl()),fft_freqs.at(i)->get_frequencies(), raws.at(i)->get_abs_sa_spectra(channel_type));

//    }
    plt::xlabel("f [Hz]");
    plt::legend();


    plt::show();

    //std::cout << std::endl;





    return EXIT_SUCCESS;
}
