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



    std::vector<std::shared_ptr<channel>> channels;
    std::string channel_type("Hx");
    double sample_freq = 1024;
    channels.emplace_back(std::make_shared<channel>(channel_type, sample_freq));
    channels.emplace_back(std::make_shared<channel>(channel_type, sample_freq));
    channels.emplace_back(std::make_shared<channel>(channel_type, sample_freq));



    std::vector<std::shared_ptr<fftw_freqs>> fft_freqs;
    std::vector<std::shared_ptr<raw_spectra>> raws;

    i = 0;
    size_t rl, wl = 1024;
    rl = wl;
    // create the fftw interface with individual window length and read length
    for (auto &chan : channels) {
        fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), wl, rl));
        wl *= 4;
        rl = wl;
        ++i;
        chan->set_fftw_plan(fft_freqs.back());
        // here each channel is treated as single result - by default it would contain 5 channels
        raws.emplace_back(std::make_shared<raw_spectra>(fft_freqs.back()));

    }
    // add a zero padded
    wl = 4096;
    rl = 1024;
    try {
        channels.emplace_back(std::make_shared<channel>(channel_type, 1024));
        fft_freqs.emplace_back(std::make_shared<fftw_freqs>(channels.back()->get_sample_rate(), wl, rl));
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


    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::normal_distribution<> dist{5,2};
    std::vector<double> noise_data(16385 * 512);
    double sin_freq = 180;
    double sn = 0;
    // sin(sin_freq * (2 * pi) * i / sample_freq);
    for (auto &nd : noise_data) {
        nd = dist(gen) + (sin(sin_freq * (2 * M_PI) * sn++ / sample_freq)) / 10.0;
    }


    for (auto &chan : channels) {
        chan->read_all_fftw_gussian_noise(noise_data);
    }

    fft_res_iter = fft_freqs.begin();
    for (auto &chan : channels) {
        auto fft_fres = *fft_res_iter++;
        fft_fres->set_lower_upper_f(160, 200, true); // cut off spectra
        chan->prepare_to_raw_spc(fft_fres, false);
    }

    i = 0;
    for (auto &chan : channels) {
        raws[i++]->get_raw_spectra(chan->spc, chan->channel_type, chan->is_remote, chan->is_emap);
    }

    for (auto &raw : raws) {
        raw->simple_stack_all();
    }

    //    for (auto &raw : raws) {
    //        raw->advanced_stack_all();
    //    }


//    for (const auto &sp : raws.at(0)->get_abs_sa_spectra(channel_type)) {
//        std::cout << sp << " ";

//    }
//    std::cout << std::endl;

    plt::title("FFT length");
    //plt::figure_size(1200, 780); // poison
    //plt::ylim(0,1);

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
