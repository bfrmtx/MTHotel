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
#include "gnuplotter.h"

#include <random>
#include "raw_spectra.h"
//#include "matplotlibcpp.h"
//namespace plt = matplotlibcpp;

namespace fs = std::filesystem;

/*
-u /home/bfr/devel/ats_data/Northern_Mining
-s Sarıçam
-r 7 8

*/
int main(int argc, char* argv[])
{
    std::cout << "*******************************************************************************" << std::endl << std::endl;
    std::cout << "reads data from a survey (e.g. Northern Mining, Station Sarıçam" << std::endl;
    std::cout << "performs different FFT lentghs and padding" << std::endl;
    std::cout << "performs different FFT lengths and padding" << std::endl;
    std::cout << "results should give same results - all curves on top of each other" << std::endl;
    std::cout << "the envelope of the padded must be be the non-padded - you just obtain points inbetween" << std::endl;
    std::cout << "the base level is differrent: you have a different amount of stacks; noise may go down for shorter wl -> more stacks" << std::endl;
    std::cout << std::endl <<  "*******************************************************************************" << std::endl << std::endl;



    fs::path stationname;
    std::vector<size_t> run_nos;

    std::shared_ptr<survey_d> survey;


    unsigned l = 1;
    while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
        std::string marg(argv[l]);

        if (marg.compare("-u") == 0) {
            fs::path surveyname = std::string(argv[++l]);
            if (!fs::is_directory(surveyname)) {
                std::cerr << "-u surveydir needs an existing directoy with stations inside" << std::endl;
                return EXIT_FAILURE;
            }
            surveyname = fs::canonical(surveyname);
            survey = std::make_shared<survey_d>(home_dir.string() + "/devel/ats_data/Northern_Mining");

        }

        if (marg.compare("-s") == 0) {
            auto stationname = std::string(argv[++l]);

            if (!fs::is_directory(surveyname)) {
                std::cerr << "-u surveydir needs an existing directoy with stations inside" << std::endl;
                return EXIT_FAILURE;
            }
            surveyname = fs::canonical(surveyname);
        }
    }


    size_t i;
    //fs::path home_dir(getenv("HOME"));
    auto station = survey->get_station("Sarıçam"); // that is a shared pointer from survey

    auto run = station->get_run(8);

    std::vector<std::shared_ptr<channel>> channels;
    std::string channel_type("Ex");

    // shared pointer from survey
    try {
        channels.emplace_back(run->get_channel(channel_type)); // that is a link, NOT a copy; you can do it only ONCE
        // a new instance - not a copy/link; the readbuffer is inside the class
        channels.emplace_back(std::make_shared<channel>(run->get_channel(channel_type)));
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
    size_t wl = channels.at(0)->get_sample_rate();
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

    double f_lower = channels.at(0)->get_sample_rate() / 32;
    double f_upper =  channels.at(0)->get_sample_rate() / 4;
    fft_res_iter = fft_freqs.begin();
    for (auto &chan : channels) {
        auto fft_fres = *fft_res_iter++;
        fft_fres->set_lower_upper_f(f_lower, f_upper, true); // cut off spectra
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
    //        raw->advanced_stack_all(0.8);
    //    }


    auto gplt = std::make_unique<gnuplotter>(1);
    gplt->cmd << "set terminal qt size 1024,768" << std::endl;
    gplt->cmd << "set title 'FFT length'" << std::endl;
    //gplt->cmd << "set key off" << std::endl;

    gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
    gplt->cmd << "set ylabel 'Amplitude'" << std::endl;
    gplt->cmd << "set grid" << std::endl;

    gplt->cmd << "set logscale xy" << std::endl;
    gplt->set_x_range(f_lower, f_upper);

    // plt::title("FFT length");
    for (size_t i = 0; i < channels.size(); ++i) {
        if (fft_freqs.at(i)->get_wl() != fft_freqs.at(i)->get_rl()) {
            std::string padded("padded ");
            padded += std::to_string(fft_freqs.at(i)->get_rl());
            padded += " -> ";
            padded += std::to_string(fft_freqs.at(i)->get_wl());
            gplt->set_xy_lines(fft_freqs.at(i)->get_frequencies(), raws.at(i)->get_abs_sa_spectra(channel_type), padded, 1);
            // plt::named_loglog("padded 1024->4096", fft_freqs.at(i)->get_frequencies(), raws.at(i)->get_abs_sa_spectra(channel_type), "r+");
        }
        else {
            gplt->set_xy_lines(fft_freqs.at(i)->get_frequencies(), raws.at(i)->get_abs_sa_spectra(channel_type), std::to_string(fft_freqs.at(i)->get_wl()), 1);
        }
    }
    gplt->plot();






    return EXIT_SUCCESS;
}
