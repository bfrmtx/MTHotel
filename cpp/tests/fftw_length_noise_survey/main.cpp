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
#include "mini_math.h"


namespace fs = std::filesystem;

/*


-u /home/bfr/devel/ats_data/Eastern_Mining -sb -s LH12 -r 1 2 3 4 5 6 8 9
-u /home/bfr/devel/ats_data/Eastern_Mining -sb -s LH12 -r 5 6
-u /home/bfr/devel/ats_data/Eastern_Mining -sb -s LH12 -r 5 6
-u /home/bfr/devel/ats_data/Northern_Mining -sb -s Sarıçam -r 7 8
-u /home/bfr/devel/ats_data/Northern_Mining -s wwo -r 1 2
-u /home/bfr/devel/ats_data/Northern_Mining -s Sarıçam -r 7 8
-u ~/devel/ats_data/HF_FFT -s YEN1107 -r  2 3  use for training

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



    bool same_base = false;   // // always compare against first RMS, default no (outer), yes likely for inner f range
    bool inner_range = false;
    std::shared_ptr<survey_d> survey;
    std::shared_ptr<station_d> station;
    std::vector<std::shared_ptr<run_d>> runs;
    auto pool = std::make_shared<BS::thread_pool>();

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

            if (marg.compare("-sb") == 0) {
                same_base = true; // prepare for runs
            }

            if (marg.compare("-r") == 0) {
                br = true; // prepare for runs
            }

            if (marg.compare("-i") == 0) {
                inner_range = true; // prepare for runs
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
    std::vector<std::shared_ptr<channel>> channels_noise;


    std::string channel_type("Ex");

    size_t min_wl = 1024;
    size_t min_rl = 256;
    size_t wl = 0;
    size_t padded = 1024;  // min padded

    std::random_device rd{};
    std::mt19937 gen{rd()};
    std::normal_distribution<> dist{0,2};  // mean 0, sigma 2

    for (auto &run : runs) {

        // shared pointer from survey
        try {
            channels.emplace_back(run->get_channel(channel_type)); // that is a link, NOT a copy; you can do it only ONCE
            wl = (size_t)channels.back()->get_sample_rate();
            if (wl < min_wl) wl = min_wl;                          // min is rl 256, wl 256
            fft_freqs.emplace_back(std::make_shared<fftw_freqs>(channels.back()->get_sample_rate(), wl, wl));
            channels.back()->set_fftw_plan(fft_freqs.back());
            raws.emplace_back(std::make_shared<raw_spectra>(pool, fft_freqs.back()));

            // want that for zero padding
            if (channels.back()->get_sample_rate() < 1024) {        // min is rl 256, wl 1024
                channels.emplace_back(std::make_shared<channel>(run->get_channel(channel_type)));
                fft_freqs.emplace_back(std::make_shared<fftw_freqs>(channels.back()->get_sample_rate(), wl*4, wl));
                channels.back()->set_fftw_plan(fft_freqs.back());
                raws.emplace_back(std::make_shared<raw_spectra>(pool, fft_freqs.back()));
            }
            /*
            wl *= 4;  // sharper fft, at least 1024                  min is rl 1024, wl 1024
            // a new instance - not a copy/link; the readbuffer is inside the class
            channels.emplace_back(std::make_shared<channel>(run->get_channel(channel_type)));
            fft_freqs.emplace_back(std::make_shared<fftw_freqs>(channels.back()->get_sample_rate(), wl, wl));
            channels.back()->set_fftw_plan(fft_freqs.back());
            raws.emplace_back(std::make_shared<raw_spectra>(pool, fft_freqs.back()));

            // add noise channels

            for (auto &chan : channels) {
                channels_noise.emplace_back(std::make_shared<channel>(chan)); // these are linked
            }
*/



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



    size_t thread_index = 0;

    std::cout << "FFT" << std::endl;

    try {

        for (auto &chan : channels) {
            std::cout << "emplacing thread " << thread_index++ << std::endl;
            pool->push_task(&channel::read_all_fftw, chan, false, nullptr);
        }
        pool->wait_for_tasks();

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


    auto fs = fft_freqs.at(0)->get_frequencies();


    inner_outer<double> innerouter;
    fft_res_iter = fft_freqs.begin();
    for (auto &chan : channels) {
        auto fft_fres = *fft_res_iter++;
        innerouter.set_low_high(fft_fres->auto_range(0.006, 0.4)); // cut off spectra
        chan->prepare_to_raw_spc(fft_fres, false);
    }

    i = 0;
    for (auto &chan : channels) {
        raws[i++]->get_raw_spectra(chan->spc, chan->channel_type, chan->bw, chan->is_remote, chan->is_emap);
    }

    std::cout << "stacking" << std::endl;
    thread_index = 0;

    try{
        for (auto &raw : raws) {
            std::cout << "push thread " << thread_index++ << std::endl;
            raw->advanced_stack_all(0.8);
        }
        pool->wait_for_tasks();
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
    gplt->cmd << "set format y '1E^%+03T'"  << std::endl;
    gplt->cmd << "set logscale xy" << std::endl;
    if (inner_range) gplt->set_x_range(innerouter.get_inner());
    else gplt->set_x_range(innerouter.get_outer());
    gplt->cmd << "set key font \"Hack, 10\"" << std::endl;
    // gplt->set_x_range(40, 60);

    if (inner_range) {
        std::cout << "inner range: " << mstr::f_to_string(innerouter.get_inner().first) << " <-> " << mstr::f_to_string(innerouter.get_inner().second) << std::endl;
        for (size_t i = 0; i < channels.size(); ++i) {
            std::cout << mstr::f_to_string(fft_freqs[i]->get_frange().first) << " <-> " << mstr::f_to_string(fft_freqs[i]->get_frange().second) << std::endl;
            auto f = fft_freqs.at(i)->get_frequencies();
            std::cout << "";
        }
    }
    std::cout << "outer range: " << mstr::f_to_string(innerouter.get_outer().first) << " <-> " << mstr::f_to_string(innerouter.get_outer().second) << std::endl;
    for (size_t i = 0; i < channels.size(); ++i) {
        std::cout << mstr::f_to_string(fft_freqs[i]->get_frange().first) << " <-> " << mstr::f_to_string(fft_freqs[i]->get_frange().second) << std::endl;
        auto f = fft_freqs.at(i)->get_frequencies();
        std::cout << "";
    }

    auto rmss = std::make_shared<std::vector<double>>(channels.size());
    for (size_t i = 0; i < channels.size(); ++i) {

        try {
            std::vector<double> f, v;
            if (inner_range)  f = fft_freqs[i]->get_frequency_slice(innerouter.get_inner());
            else   f = fft_freqs[i]->get_frequency_slice(innerouter.get_outer());
            if (inner_range)  v = trim_by_index<double>(raws.at(i)->get_abs_sa_spectra(channel_type), fft_freqs[i]->get_index_slice());
            else  v = raws.at(i)->get_abs_sa_spectra(channel_type);
            double rms = 0;
            for (const auto val : v) rms += val * val;
            rms /= double(v.size());
            rms = sqrt(rms);

            std::cout << "rms: " << rms << std::endl;
            ( *rmss )[i] = rms;
        }

        catch (const std::string &error ) {
            std::cerr << error <<std::endl;
            return EXIT_FAILURE;
        }


    }

    auto labels = gnuplot_labels(fft_freqs, rmss);
    std::cout << "formatted" << std::endl;
    for (const auto &lab : labels) {
        std::cout << lab.str() << std::endl;
    }

    // plt::title("FFT length");
    try {
        for (size_t i = 0; i < channels.size(); ++i) {
            auto v = raws.at(i)->get_abs_sa_spectra(channel_type);

            auto label = labels.at(i).str();
            // gplt->set_xy_lines(fft_freqs.at(i)->get_frequencies(), raws.at(i)->get_abs_sa_spectra(channel_type), label.str(), 1);

            if (fft_freqs.at(i)->get_wl() != fft_freqs.at(i)->get_rl()) {
                gplt->set_xy_lines(fft_freqs.at(i)->get_frequencies(), v, label, 1, "lc rgbcolor \"red\" dt 0");
            }
            else if (fft_freqs.at(i)->get_wl() > fft_freqs.at(i)->get_sample_rate()) {
                gplt->set_xy_lines(fft_freqs.at(i)->get_frequencies(), v, label, 1, "lc rgbcolor \"green\" dt 2");
            }
            else gplt->set_xy_lines(fft_freqs.at(i)->get_frequencies(), v, label, 1,  "lc rgbcolor \"blue\"");


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
