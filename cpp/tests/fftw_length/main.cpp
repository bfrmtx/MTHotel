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
#include "bthread.h"


namespace fs = std::filesystem;

/*
-u /home/bfr/devel/ats_data/Northern_Mining -s Sarıçam -r 7 8

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




    std::shared_ptr<survey_d> survey;
    std::shared_ptr<station_d> station;
    std::vector<std::shared_ptr<run_d>> runs;
    unsigned l = 1;
    try {

        bool br = false;
        while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
            std::string marg(argv[l]);

            if (marg.compare("-u") == 0) {
                fs::path surveyname = std::string(argv[++l]);
                if (!fs::is_directory(surveyname)) {
                    std::cerr << "-u surveydir needs an existing directoy with stations inside" << std::endl;
                    std::cerr << "given: " << surveyname.string() << std::endl;
                    return EXIT_FAILURE;
                }
                surveyname = fs::canonical(surveyname);
                survey = std::make_shared<survey_d>(surveyname);
                if (survey == nullptr) {
                    std::string err_str = __func__;
                    err_str += "first use -u surveyname in order to init the survey";
                    throw err_str;
                }

            }
            if (marg.compare("-s") == 0) {
                if (survey == nullptr) {
                    std::string err_str = __func__;
                    err_str += "first use -u surveyname in order to init the survey";
                    throw err_str;
                }
                auto stationname = std::string(argv[++l]);
                station = survey->get_station(stationname); // that is a shared pointer from survey
                if (station == nullptr) {
                    std::string err_str = __func__;
                    err_str += "secondly use -s stationname in order to init station";
                    throw err_str;
                }
            }

            if (marg.compare("-r") == 0) {
                br = true; // prepare for runs
            }

            if (marg.compare("-") == 0) {
                std::cerr << "\nunrecognized option " << argv[l] << std::endl;
                return EXIT_FAILURE;
            }

            ++l;
        }

        // all options including the last -r is here, now get the runs
        while ( (l < unsigned(argc))) {
            std::string marg(argv[l]);
            if ((survey != nullptr) && (station != nullptr) && br) {
                size_t ri = stoul(marg);
                runs.emplace_back(station->get_run(ri));
            }
            ++l;
        }
    }
    catch (const std::string &error) {
        std::cerr << error << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::invalid_argument& ia) {
        std::cerr << ia.what() << std::endl;
        std::cerr << "unvalid run number" << std::endl;
        return EXIT_FAILURE;
    }
    catch(...) {
        std::cerr << "general error" << std::endl;
        return EXIT_FAILURE;

    }
    if (!runs.size()) {
        std::cerr << "no run numbers given" << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<std::shared_ptr<fftw_freqs>> fft_freqs;
    std::vector<std::shared_ptr<raw_spectra>> raws;


    std::vector<std::shared_ptr<channel>> channels;
    std::vector<std::shared_ptr<channel>> channels_padded;

    std::string channel_type("Ex");

    size_t min_wl = 256;
    size_t min_rl = 256;
    size_t wl = 0;
    size_t padded = 1024;  // min padded


    for (auto &run : runs) {

        // shared pointer from survey
        try {
            channels.emplace_back(run->get_channel(channel_type)); // that is a link, NOT a copy; you can do it only ONCE
            wl = (size_t)channels.back()->get_sample_rate();
            if (wl < min_wl) wl = min_wl;                          // min is rl 256, wl 256
            fft_freqs.emplace_back(std::make_shared<fftw_freqs>(channels.back()->get_sample_rate(), wl, wl));
            channels.back()->set_fftw_plan(fft_freqs.back());
            raws.emplace_back(std::make_shared<raw_spectra>(fft_freqs.back()));

            // want that for zero padding
            if (channels.back()->get_sample_rate() < 1024) {        // min is rl 256, wl 1024
                channels.emplace_back(std::make_shared<channel>(run->get_channel(channel_type)));
                fft_freqs.emplace_back(std::make_shared<fftw_freqs>(channels.back()->get_sample_rate(), padded, wl));
                channels.back()->set_fftw_plan(fft_freqs.back());
                raws.emplace_back(std::make_shared<raw_spectra>(fft_freqs.back()));
            }

            wl *= 4;  // sharper fft, at least 1024                  min is rl 1024, wl 1024
            // a new instance - not a copy/link; the readbuffer is inside the class
            channels.emplace_back(std::make_shared<channel>(run->get_channel(channel_type)));
            fft_freqs.emplace_back(std::make_shared<fftw_freqs>(channels.back()->get_sample_rate(), wl, wl));
            channels.back()->set_fftw_plan(fft_freqs.back());
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

    }
    std::string unit;
    double f_or_s;


    // example usage  fft with auto & channels
    auto fft_res_iter = fft_freqs.begin();
    for (const auto &chan : channels) {
        auto fft_fres = *fft_res_iter++;
        mstr::sample_rate_to_str(chan->get_sample_rate(), f_or_s, unit);
        std::cout << "use sample rates of " << f_or_s << " " << unit <<  " wl:" << fft_fres->get_wl() << "  read length:" << fft_fres->get_rl() <<  std::endl;
    }


    size_t i;



    std::vector<size_t> execs = mk_mini_threads(0, channels.size());
    size_t thread_index = 0;

    std::cout << "FFT" << std::endl;

    try {
        for (const auto &ex : execs) {
            std::vector<std::jthread> threads;
            for (size_t j = 0; j < ex; ++j) {
                std::cout << "emplacing thread " << j << std::endl;
                threads.emplace_back(std::jthread (&channel::read_all_fftw, channels[thread_index++], false, nullptr));

            }
        }
    }
    catch (const std::string &error) {
        std::cerr << error << std::endl;
        std::cerr << "could not execute fftw" << std::endl;
        return EXIT_FAILURE;
    }
    catch(...) {
        std::cerr << "could not execute fftw" << std::endl;
        return EXIT_FAILURE;

    }
    std::cout << "done" << std::endl;



    //    for (auto &chan : channels) {
    //        chan->read_all_fftw();
    //    }

    auto fs = fft_freqs.at(0)->get_frequencies();


    std::vector<double> max_mins;
    fft_res_iter = fft_freqs.begin();
    for (auto &chan : channels) {
        auto fft_fres = *fft_res_iter++;
        auto mm = fft_fres->auto_range(0.001, 0.6); // cut off spectra
        max_mins.push_back(mm.first);
        max_mins.push_back(mm.second);
        chan->prepare_to_raw_spc(fft_fres, false);
    }

    std::sort(max_mins.begin(), max_mins.end());

    i = 0;
    for (auto &chan : channels) {
        raws[i++]->get_raw_spectra(chan->spc, chan->channel_type, chan->bw, chan->is_remote, chan->is_emap);
    }

    std::cout << "stacking" << std::endl;
//    thread_index = 0;
//    try{
//        for (const auto &ex : execs) {
//            std::vector<std::jthread> threads;
//            for (size_t j = 0; j < ex; ++j) {
//                std::cout << "emplacing thread" << j << std::endl;
//                threads.emplace_back(std::jthread (&raw_spectra::advanced_stack_all, raws[thread_index++], 0.8));

//            }
//        }

//    }

    auto pool = std::make_shared<BS::thread_pool>(4);
    thread_index = 0;
    std::vector<std::jthread> threads;
    std::vector<std::future<size_t>> futures;
    try{
        for (auto &raw : raws) {
            std::cout << "push thread " << thread_index++ << std::endl;
            futures.emplace_back(raw->advanced_stack_all_t(pool, 0.8));
        }
//        for (const auto &ex : execs) {
//            std::vector<std::jthread> threads;
//            for (size_t j = 0; j < ex; ++j) {
//                std::cout << "emplacing thread " << j << std::endl;
//                threads.emplace_back(std::jthread (&raw_spectra::advanced_stack_all, raws[thread_index++], 0.8));
//                //raws[thread_index++]->advanced_stack_all(0.8);

//            }
//        }

    }
    catch (const std::string &error) {
        std::cerr << error << std::endl;
        std::cerr << "could not execute advanced_stack_all" << std::endl;
        return EXIT_FAILURE;
    }
    catch(...) {
        std::cerr << "could not execute advanced_stack_all" << std::endl;
        return EXIT_FAILURE;

    }
    std::cout << "done" << std::endl;

    for (auto &f : futures) {
        std::cout << "sz:" << f.get();
    }

   // pool->wait_for_tasks();

    std::string init_err;

    auto gplt = std::make_unique<gnuplotter<double, double>>(init_err);

    if (init_err.size()) {
        std::cout << init_err << std::endl;
        return EXIT_FAILURE;
    }

    gplt->cmd << "set terminal qt size 2048,1600 enhanced" << std::endl;
    gplt->cmd << "set title 'FFT length'" << std::endl;
    //gplt->cmd << "set key off" << std::endl;

    gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
    gplt->cmd << "set ylabel 'amplitude'" << std::endl;
    gplt->cmd << "set grid" << std::endl;

    gplt->cmd << "set logscale xy" << std::endl;
    gplt->set_x_range(max_mins.front(), max_mins.back());

    // plt::title("FFT length");
    try {
        for (size_t i = 0; i < channels.size(); ++i) {
            std::stringstream label;


            mstr::sample_rate_to_str(channels.at(i)->get_sample_rate(), f_or_s, unit, true);
            label << "fs "<< f_or_s << " " << unit << " wl: " << fft_freqs.at(i)->get_wl();
            auto v = raws.at(i)->get_abs_sa_spectra(channel_type);

            if (fft_freqs.at(i)->get_wl() != fft_freqs.at(i)->get_rl()) {
                label << " rl:  " << fft_freqs.at(i)->get_rl();
            }
            std::cout << label.str() << " " << fft_freqs.at(i)->get_wincal() <<  std::endl;


            // gplt->set_xy_lines(fft_freqs.at(i)->get_frequencies(), raws.at(i)->get_abs_sa_spectra(channel_type), label.str(), 1);
            gplt->set_xy_lines(fft_freqs.at(i)->get_frequencies(), v, label.str(), 1);


        }
    }
    catch (const std::string &error) {
        std::cerr << error << std::endl;
        std::cerr << "could not pipe all files to gnuplot" << std::endl;
        return EXIT_FAILURE;
    }
    catch(...) {
        std::cerr << "could not pipe all files to gnuplot" << std::endl;
        return EXIT_FAILURE;

    }
    gplt->plot();





    return EXIT_SUCCESS;
}
