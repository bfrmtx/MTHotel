#include <iostream>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <complex>
#include <chrono>
#include <filesystem>
#include <algorithm>

#include "atss.h"
#include "freqs.h"

#include "sqlite_handler.h"

#include <fftw3.h>

#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

template<typename Iterator>
void cleanup(Iterator first, Iterator last, const double treat_as_zero) {

    while (first != last) {
        if (abs(*first) < treat_as_zero) *first = 0.0;
        ++first;
    }


}

template<typename Iterator>
void cleanup_cplx(Iterator first, Iterator last, const double treat_as_zero) {

    while (first != last) {
        if (abs(real(*first)) < treat_as_zero) *first = complex<double>(0.0, imag(*first));
        if (abs(imag(*first)) < treat_as_zero) *first = complex<double>(real(*first), 0.0);

        ++first;
    }


}

int main()
{

    std::filesystem::path lllf("/home/bfr/devel/ats_data/three_fgs/indi/ts/S25f/meas_2017-04-17_07-34-36/295_ADU-07e_C001_R000_TEy_16s");
    std::filesystem::path llf("/home/bfr/devel/ats_data/three_fgs/indi/ts/S25f/meas_2017-04-17_07-32-14/295_ADU-07e_C001_R000_TEy_4s");
    std::filesystem::path lf("/home/bfr/devel/ats_data/three_fgs/indi/ts/S25f/meas_2017-04-17_07-31-38/295_ADU-07e_C001_R000_TEy_1Hz");
    // long window vs zero padded
    std::filesystem::path lllfl("/home/bfr/devel/ats_data/three_fgs/indi/ts/S25f/meas_2017-04-17_07-34-36/295_ADU-07e_C001_R000_TEy_16s");
    std::filesystem::path lllfzp("/home/bfr/devel/ats_data/three_fgs/indi/ts/S25f/meas_2017-04-17_07-34-36/295_ADU-07e_C001_R000_TEy_16s");



    std::vector<std::filesystem::path> files;
    files.push_back(lllf);
    files.push_back(llf);
    files.push_back(lf);
    files.push_back(lllfl);
    files.push_back(lllfzp);


    std::vector<std::shared_ptr<channel>> channels;
    std::vector<fftw_plan> plans(files.size());
    std::vector<std::vector<double>> ins(files.size());
    std::vector<std::vector<std::complex<double>>> outs(files.size());
    std::vector<std::vector<std::complex<double>>> ress(files.size());
    std::vector<std::vector<double>> ampls(files.size());


    size_t i;
    size_t lw = 3;
    size_t zp = 4;


    for (const auto &file : files) {
        channels.emplace_back(std::make_shared<channel>(file));
    }

    std::vector<std::shared_ptr<fftw_freqs>> fft_freqs;

    i = 0;
    size_t wl = 1024;
    for (const auto &chan : channels) {
        if (i == lw) wl = 4096;
        if (i == zp) wl = 2048;
        fft_freqs.emplace_back(std::make_shared<fftw_freqs>(chan->get_sample_rate(), wl));
        wl *= 4;
        ++i;

    }

    double f_or_s;
    std::string unit;

    auto fft_res_iter = fft_freqs.begin();
    for (const auto &chan : channels) {
        auto fft_fres = *fft_res_iter++;
        mstr::sample_rate_to_str(chan->get_sample_rate(), f_or_s, unit);
        std::cout << "use sample rates of " << f_or_s << " " << unit <<  " wl:" << fft_fres->get_wl() << std::endl;


    }



    i = 0;
    for (const auto &freqs : fft_freqs) {
        std::cout << std::endl;
        ins[i].resize(freqs->get_wl());
        outs[i].resize(freqs->get_fl());
        plans[i] = fftw_plan_dft_r2c_1d(ins[i].size(), &ins[i][0], reinterpret_cast<fftw_complex*>(&outs[i][0]) , FFTW_ESTIMATE);
        freqs->set_lower_upper_f(1.0/8192.0, 0.011, true);
        freqs->vplot(freqs->get_frequencies(), 12, 12, true);
        std::cout << std::endl;
        ++i;

    }

    i = 0;
    for (const auto &chan : channels) {
        std::ifstream ifile;
        chan->prepare_read_atss(ifile);
        chan->read_data(ins[i], ifile, true);
        if (ins[i].size()) std::cout << "read " << ins[i].size() << std::endl;
        detrend_and_hanning<double>(ins[i].begin(), ins[i].end());
        ++i;
    }

    for (const auto &p : plans) {
        fftw_execute(p);
    }

    i = 0;
    size_t j;
    for (const auto &freqs : fft_freqs) {
        ress[i] = freqs->trim_fftw_result(outs[i]);
        std::cout << "xamps" << std::endl;

//        j = 0;
//        std::vector<double> dt(ress[i].size());
//        for (const auto v : ress[i]) dt[j++] = std::abs(v);
//        freqs->vplot(dt, 12, 12, true);

        freqs->scale(ress[i]);
        j = 0;
        ampls[i].resize(ress[i].size());
        for (const auto v : ress[i]) ampls[i][j++] = std::abs(v);

       // freqs->vplot(ampls[i], 12, 12, true);

        ++i;
    }
    std::cout << std::endl;

    i = 0;
    for (const auto &freqs : fft_freqs) {
        std::cout << "amplitudes:" << std::endl;
        freqs->vplot(ampls[i++], 12, 12, true);
        std::cout << std::endl;

    }
//    for (auto &v : ampls[lw]) v /= 4096;
//    for (auto &v : ampls[zp]) v /= 8192;
//    for (auto &v : ampls[0]) v /= 1024;
    fft_freqs[lw]->vplot(ampls[lw], 12, 12, true);
    plt::title("Sample figure");
    plt::loglog(fft_freqs[0]->get_frequencies(), ampls[0]);
    plt::loglog(fft_freqs[lw]->get_frequencies(), ampls[lw]);
    plt::loglog(fft_freqs[zp]->get_frequencies(), ampls[zp]);

    plt::show();

    /*


    double sample_rate = 1.0/16.0;
    // double sample_rate = 512;
    bool invert = false;
    if (sample_rate < 4) invert = true;
    fftw_freqs freqs(sample_rate, 8192);

    std::filesystem::path fileex("/survey-master/indi/ts/S25f/meas_2017-04-17_07-34-36/295_ADU-07e_C000_R000_TEx_16s.json");
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
            std::cout << std::endl;


    return 0;
}
