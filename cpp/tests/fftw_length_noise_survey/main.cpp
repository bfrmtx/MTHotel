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
namespace splt = matplotlibcpp;



// test data from noise generators


int main()
{

    std::cout << "test data from noise generators "<< std::endl;

    size_t i, j;

    // hard coded test

    std::filesystem::path filepath(std::filesystem::temp_directory_path() / "aa/noise");

    auto survey = std::make_shared<survey_d>(filepath);
    auto station = survey->get_station("1"); // that is a shared pointer from survey

    size_t nruns = 3;
    size_t start_run = 4;
    std::string ch_type("Ex");

    // fetch the Ex channel of each
    std::vector<std::shared_ptr<run_d>> runs;

    try {
        for (j = start_run; j < start_run + nruns; ++j) {
            runs.emplace_back(station->get_run(j));
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
        std::cerr << "could not allocate all runs" << std::endl;
        return EXIT_FAILURE;

    }


    std::vector<std::shared_ptr<channel>> channels;

    i = 0;
    try {
        for (auto &run : runs) {
            channels.emplace_back(run->get_channel(ch_type));
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




    std::vector<std::shared_ptr<fftw_freqs>> fft_freqs;
    std::vector<std::shared_ptr<raw_spectra>> raws;

    try {
        for (auto &chan : channels) {
            size_t rl = 512, wl = 512;
            size_t xw = (size_t)chan->get_sample_rate();
            if (xw > 512) {
                rl = xw;
                wl = xw;
            }
            fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), wl, rl));
            chan->set_fftw_plan(fft_freqs.back());
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
        std::cerr << "could not allocate all ffts / raws" << std::endl;
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
        chan->open_atss_read();
    }


    for (auto &chan : channels) {
        chan->read_all_fftw();
    }


    fft_res_iter = fft_freqs.begin();
    for (auto &chan : channels) {
        auto fft_fres = *fft_res_iter++;
        fft_fres->auto_range(0.05, 0.9); // cut off spectra
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


    plt::figure_size(1400, 1024); // can be poison; at the beginning maybe ok
    plt::title("FFT length");
    //plt::ylim(0,1);

    for (size_t i = 0; i < channels.size(); ++i) {
        auto v = raws.at(i)->get_abs_sa_spectra(ch_type);
        //fft_freqs.at(i)->unscale(v);
        auto label = "wl: " + std::to_string(fft_freqs.at(i)->get_wl()) + " @" + mstr::sample_rate_to_str_simple(fft_freqs.at(i)->get_sample_rate());
        plt::named_loglog(label, fft_freqs.at(i)->get_frequencies(), v);

        //plt::named_loglog(label, fft_freqs.at(i)->get_frequencies(), raws.at(i)->get_abs_sa_spectra(ch_type));

    }
    plt::xlabel("f [Hz]");
    plt::ylabel("mV / √(Hz)"); // U+221A

    plt::legend();
    plt::save("/tmp/a.pdf");
    plt::show();

//    splt::title("FFT length sec");
//    //plt::figure_size(1200, 780); // poison
//    //plt::ylim(0,1);

//    for (size_t i = 0; i < channels.size(); ++i) {
//        auto v = raws.at(i)->get_abs_sa_spectra(ch_type);
//        //fft_freqs.at(i)->unscale(v);
//        auto label = "wl: " + std::to_string(fft_freqs.at(i)->get_wl()) + " @" + mstr::sample_rate_to_str_simple(fft_freqs.at(i)->get_sample_rate());
//        splt::named_loglog(label, fft_freqs.at(i)->get_frequencies(), v);

//        //plt::named_loglog(label, fft_freqs.at(i)->get_frequencies(), raws.at(i)->get_abs_sa_spectra(ch_type));

//    }
//    splt::xlabel("f [Hz]");
//    splt::legend();

//    splt::show();


    //std::cout << std::endl;





    return EXIT_SUCCESS;
}
