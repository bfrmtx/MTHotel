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


    size_t i;
    fs::path home_dir(getenv("HOME"));
    auto survey = std::make_shared<survey_d>(home_dir.string() + "/devel/ats_data/Northern_Mining");
    auto station = survey->get_station("Sarıçam"); // that is a shared pointer from survey

    auto run = station->get_run(1);

    std::vector<std::shared_ptr<channel>> channels;
    std::string channel_type("Hy");

    // shared pointer from survey
    try {
        channels.emplace_back(run->get_channel(channel_type)); // that is a link, NOT a copy; you can do it only ONCE
        // a new instance - not a copy/link; the readbuffer is inside the class
        channels.emplace_back(std::make_shared<channel>(run->get_channel(channel_type)));
        channels.emplace_back(std::make_shared<channel>(run->get_channel(channel_type)));
    }
    catch (const std::string &error) {

        std::cerr << error << std::endl;
        return EXIT_FAILURE;

    }
    std::vector<std::shared_ptr<fftw_freqs>> fft_freqs;
    std::vector<std::shared_ptr<raw_spectra>> raws;

    i = 0;
    size_t wl = 1024;
    // create the fftw interface with individual window length and read length
    for (auto &chan : channels) {
        fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), wl, wl));
        wl *= 4;
        ++i;
        chan->set_fftw_plan(fft_freqs.back());
        // here each channel is treated as single result - by default it would contain 5 channels
        raws.emplace_back(std::make_shared<raw_spectra>(fft_freqs.back()));

    }
    // add a zero padded
    try {
    channels.emplace_back(std::make_shared<channel>(run->get_channel(channel_type)));
    fft_freqs.emplace_back(std::make_shared<fftw_freqs>(channels.back()->get_sample_rate(), 4096, 1024));
    channels.back()->set_fftw_plan(fft_freqs.back());
    // here each channel is treated as single result - by default it would contain 5 channels
    raws.emplace_back(std::make_shared<raw_spectra>(fft_freqs.back()));
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


    for (auto &chan : channels) {
        chan->read_all_fftw();
    }

    fft_res_iter = fft_freqs.begin();
    for (auto &chan : channels) {
        auto fft_fres = *fft_res_iter++;
        fft_fres->set_lower_upper_f(2, 40, true); // cut off spectra
        chan->prepare_to_raw_spc(fft_fres, false);
    }

    i = 0;
    for (auto &chan : channels) {
        raws[i++]->get_raw_spectra(chan->spc, chan->channel_type, chan->is_remote, chan->is_emap);
    }

//    for (auto &raw : raws) {
//        raw->simple_stack_all();
//    }

    for (auto &raw : raws) {
        raw->advanced_stack_all(0.8);
    }



    plt::title("FFT length");
    for (size_t i = 0; i < channels.size(); ++i) {
        if ((fft_freqs.at(i)->get_wl() == 4096) && (fft_freqs.at(i)->get_rl() == 1024)) {
            plt::named_loglog("padded 1024->4096", fft_freqs.at(i)->get_frequencies(), raws.at(i)->get_abs_sa_spectra(channel_type), "r+");
        }
        else if ((fft_freqs.at(i)->get_wl() == 1024) && (fft_freqs.at(i)->get_rl() == 1024)) {
            plt::named_loglog("1024", fft_freqs.at(i)->get_frequencies(), raws.at(i)->get_abs_sa_spectra(channel_type), "bo");
        }

        else plt::named_loglog(std::to_string(fft_freqs.at(i)->get_wl()),fft_freqs.at(i)->get_frequencies(), raws.at(i)->get_abs_sa_spectra(channel_type));

    }
    plt::xlabel("f [Hz]");
    plt::legend();


    plt::show();

    //std::cout << std::endl;





    return EXIT_SUCCESS;
}
