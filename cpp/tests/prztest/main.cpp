#include <iostream>
#include <algorithm>
#include <memory>
#include <functional>
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

#include "raw_spectra.h"


#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;


int main()
{


    size_t i;
    size_t lw = 3;  // long window
    size_t zp = 4;  // zero padded ... last file
    const double median_limit = 0.7;

    fs::path home_dir(getenv("HOME"));
    auto survey = std::make_shared<survey_d>(home_dir.string() + "/devel/ats_data/three_fgs/indi");
    auto station_26 = survey->get_station("S26"); // that is a shared pointer from survey

    // standard windows 512 x 4
    auto run_002 = station_26->get_run(2);
    auto run_003 = station_26->get_run(3);
    auto run_004 = station_26->get_run(4);

    std::vector<std::shared_ptr<channel>> channels;
    std::string channel_type("Ey");

    // shared pointer from survey
    try {
        channels.emplace_back(run_004->get_channel(channel_type));
        channels.emplace_back(run_003->get_channel(channel_type));
        channels.emplace_back(run_002->get_channel(channel_type));


        // a new instance - not a copy/link; the readbuffer is inside the class
        channels.emplace_back(std::make_shared<channel>(run_003->get_channel(channel_type)));
        channels.emplace_back(std::make_shared<channel>(run_003->get_channel(channel_type)));
    }
    catch (const std::string &error) {

        std::cerr << error << std::endl;
        return EXIT_FAILURE;

    }
    std::vector<std::shared_ptr<fftw_freqs>> fft_freqs;
    std::vector<std::shared_ptr<raw_spectra>> raws;

    i = 0;
    size_t wl = 1024, fixwl;
    fixwl = 1024;
    // create the fftw interface with individual window length and read length
    for (auto &chan : channels) {
        if (i == lw)  fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), fixwl, fixwl));
        else if (i == zp) fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), 4096, fixwl)); // first 4096
        else fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), wl, wl));
        wl *= 4;
        ++i;
        chan->set_fftw_plan(fft_freqs.back());
        // here each channel is treated as single result - by default it would contain 5 channels
        raws.emplace_back(std::make_shared<raw_spectra>(fft_freqs.back()));

    }


    double f_or_s;
    std::string unit;

    // example iterator usage  fft with auto & channels
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
        // set the range of fft spectra to use here
        fft_fres->set_lower_upper_f(1.0/2048.0, 0.011, true); // cut off spectra
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
        raws[i++]->get_raw_spectra(chan->spc, chan->channel_type, chan->is_remote, chan->is_emap);
    }

    std::cout << std::endl;

    //std::vector<double> target_freqs {1./1024.};
    std::vector<double> target_freqs;

    for (size_t j = 1; j < fft_freqs[0]->get_frequencies().size()-9;  ) {
        target_freqs.push_back(fft_freqs[0]->get_frequencies().at(j));
        j += 8;
    }

    // ******************************** parzen vectors *******************************************************************************

    try {
        for (auto &fftr : fft_freqs) {
            fftr->set_target_freqs(target_freqs, 0.15);
            fftr->create_parzen_vectors();
        }
        std::vector<std::jthread> threads;

        for (auto &fftr : fft_freqs) {
            threads.emplace_back(std::jthread (&fftw_freqs::create_parzen_vectors, fftr));
            // fftr->create_parzen_vectors();

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

    // ******************************** parzen stack *******************************************************************************
    try {
        std::vector<std::jthread> threads;
        for (auto &rw : raws) {
            threads.emplace_back(std::jthread (&raw_spectra::parzen_stack_all, rw));
            //rw->parzen_stack_all();
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

    // ******************************** cont *******************************************************************************


    plt::title("FFT Zero Padding");

    //    plt::named_loglog("zero padded", fft_freqs[zp]->get_frequencies(), raws.at(zp)->get_abs_sa_spectra(channel_type),  "bo");
    //    plt::loglog(fft_freqs[lw]->get_frequencies(), raws.at(lw)->get_abs_sa_spectra(channel_type), "r+");


    //    plt::loglog(fft_freqs[0]->get_frequencies(), raws.at(0)->get_abs_sa_spectra(channel_type));
    //    plt::loglog(fft_freqs[1]->get_frequencies(), raws.at(1)->get_abs_sa_spectra(channel_type));
    //    plt::loglog(fft_freqs[2]->get_frequencies(), raws.at(2)->get_abs_sa_spectra(channel_type));

    i = 0;
    std::vector<std::string> marks{"b-", "r-", "g-", "c-", "m-", "b--", "r--", "g--", "c--", "m--"};
    for (const auto &rws : raws) {

        std::string lab =  "fs: " +  std::to_string(rws->fft_freqs->get_sample_rate()) + " wl:" + std::to_string((int)rws->fft_freqs->get_wl()) + " rl:" + std::to_string((int)rws->fft_freqs->get_rl());
        if (i == zp) lab += " zp";
        //if (i == lw) lab += " lw";
        //plt::named_loglog(lab, rws->fft_freqs->get_selected_frequencies(), rws->get_abs_sa_prz_spectra(channel_type), marks.at(i));
        plt::named_loglog(lab, rws->fft_freqs->get_frequencies(), rws->get_abs_sa_spectra(channel_type), marks.at(i));

        ++i;
        if (i == marks.size()) i = 0;

    }

    std::vector<std::string> marks2{"b--", "r--", "g--", "c--", "m--"};
    i = 0;
    for (const auto &rws : raws) {

        std::string lab =  "pz fs: " +  std::to_string(rws->fft_freqs->get_sample_rate()) + " wl:" + std::to_string((int)rws->fft_freqs->get_wl()) + " rl:" + std::to_string((int)rws->fft_freqs->get_rl());
        if (i == zp) lab += " zp";
        //if (i == lw) lab += " lw";
        plt::named_loglog(lab, rws->fft_freqs->get_selected_frequencies(), rws->get_abs_sa_prz_spectra(channel_type), marks2.at(i));
        //plt::named_loglog(lab, rws->fft_freqs->get_frequencies(), rws->get_abs_sa_spectra(channel_type), marks2.at(i));

        ++i;

    }


    plt::ylabel("x / Sqrt(Hz)");
    plt::xlabel("f [Hz]");
    plt::legend();
    plt::show();


    //std::cin.get();

    /*


    double sample_rate = 1.0/16.0;
    // double sample_rate = 512;
    bool invert = false;
    if (sample_rate < 4) invert = true;
    fftw_freqs freqs(sample_rate, 8192);

    std::filesystem::path fileex("/survey-master/indi/stations/S26/run_004/295_ADU-07e_C000_TEx_16s.json");
    channel ex(fileex);

    std::vector<double> f;

    std::filesystem::path sqlfile("/usr/local/procmt/bin/info.sql3");
    auto sql_info = std::make_unique<sqlite_handler>();
    std::string sql_query = "SELECT * FROM default_mt_frequencies";
    auto in_targets = sql_info->sqlite_vector_double(sqlfile, sql_query);
    std::sort(in_targets.begin(), in_targets.end());


    std::vector<std::complex<double>> fake_fft(f.size());
    size_t i = 0;
    for (const auto &val : f) fake_fft[i++] = std::complex<double>(val, 0);


    freqs.set_lower_upper_f(1.0/16384.0, 1.0/1024.0, true);
    // freqs.set_lower_upper_f(1, 256, true);
    f = freqs.get_frequencies();
    auto frange = freqs.get_frange();
    std::cout << "range" <<std::endl;

    freqs.vplot(f, 12, 12, invert);
    std::cout << std::endl;


    freqs.set_target_freqs(in_targets);
    std::cout << "target freqs" << std::endl;
    freqs.vplot(freqs.get_target_freqs(), 12, 12, invert);
    std::cout << std::endl;

    //freqs.vplot(in_targets, 12, 12, true);



    //    for (const auto &val : f) {
    //        std::cout << val << std::endl;
    //    }

    std::cout << std::endl;
    std::cout << frange.first << " <-> " << frange.second << std::endl;



    std::vector<double> t(1000);
    std::vector<double> l(t.size());

    for(size_t i = 0; i < t.size(); i++) {
        t[i] = i / 100.0;
        l[i] = sin(2.0 * M_PI * 1.0 * t[i]);
    }


    plt::title("Sample figure");
    plt::plot(t, l);
    plt::show();

    std::vector<std::vector<double>> x, y, z;
    for (double i = -5; i <= 5;  i += 0.25) {
        std::vector<double> x_row, y_row, z_row;
        for (double j = -5; j <= 5; j += 0.25) {
            x_row.push_back(i);
            y_row.push_back(j);
            z_row.push_back(::std::sin(::std::hypot(i, j)));
        }
        x.push_back(x_row);
        y.push_back(y_row);
        z.push_back(z_row);
    }

    plt::plot_surface(x, y, z);
    plt::show();
    */

        /*
    freqs.set_lower_upper_f(0, 6, true);
    frange = freqs.get_frange();
    std::cout << std::endl;
    std::cout << frange.first << " <-> " << frange.second << std::endl;

        freqs.set_lower_upper_f(1, 6, false);
    frange = freqs.get_frange();
    std::cout << std::endl;
    std::cout << frange.first << " <-> " << frange.second << std::endl;

    freqs.set_lower_upper_f(8, 16, true);
    frange = freqs.get_frange();
    std::cout << std::endl;
    std::cout << frange.first << " <-> " << frange.second << std::endl;

    auto fake_trim = freqs.trim_fftw_result(fake_fft);

    for (const auto &val : fake_trim) std::cout << val << std::endl;
    std::cout << std::endl;

    std::cout << "iters f:" << std::endl;
    f = freqs.get_frequencies();
    for (const auto &val : f) std::cout << val << std::endl;
    std::cout << std::endl;

    auto idxs = freqs.iter_range(10, 14, fake_trim, true);
    auto used_f = freqs.iter_freqs(idxs, fake_trim);

    for (auto ix = idxs.first; ix < idxs.second; ++ix) {
        std::cout << *ix  << std::endl;
    }

    for (const auto &val : used_f) {
        std::cout << val << std::endl;
    }
    */
        // std::cout << std::endl;


        return 0;
}
