#include <iostream>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <complex>
#include <chrono>
#include <fftw3.h>

#include "../../../oss/fftreal/FFTReal.h"
#include "../../../oss/highfive/H5File.hpp"


bool is_pow2 (const size_t wl)
{
    if (!wl) return false;
    return  ((wl & -wl) == wl);
}

size_t next_power_of_two(const size_t n){
    size_t target, m;
    if (n > (SIZE_MAX - 1) / 2)
        throw "Vector too large";
    target = n;
    for (m = 1; m < target; m *= 2) {
        if (SIZE_MAX / 2 < m)
            throw "Vector too large";
    }

    return m;
}

namespace hf = HighFive;

int main()
{


    bool mkhdf = false;

    size_t i, j;
    std::vector<double> in;
    std::vector<std::complex<double>> out;

    std::vector<double> out_reals;
    std::vector<std::complex<double>> out_cplx;


    std::vector<double>  inv;
    std::vector<std::complex<double>> cinv;

    fftw_plan p;

    size_t N = 8;
    in.resize(N);

    out_reals.resize(in.size());
    for  (i = 0; i < in.size() / 2; ++i ) in[i] = 1;
    for  (i = in.size() / 2; i < in.size(); ++i ) in[i] = 0;
    out.resize(in.size() / 2 + 1);


    p = fftw_plan_dft_r2c_1d(in.size(), &in[0], reinterpret_cast<fftw_complex*>(&out[0]) , FFTW_ESTIMATE);
    auto start = std::chrono::steady_clock::now();
    for (j = 0; j < 1; ++j) {
        fftw_execute(p);
    }
    auto end = std::chrono::steady_clock::now();
    std::cout << "Elapsed time in milliseconds: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms" << std::endl;


    for (const auto v : in) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

    for (const auto v : out) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

//    ffft::FFTReal <double> fft_object(in.size());
//    start = std::chrono::steady_clock::now();
//    for (j = 0; j < 1; ++j) {
//        fft_object.do_fft (&out_reals[0], &in[0]);     // x (real) --FFT---> f (complex)
//    }
//    end = std::chrono::steady_clock::now();
//    std::cout << "Elapsed time in milliseconds: "
//              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
//              << " ms" << std::endl;
//    for (j = 0; j < N/2; ++j) out_cplx.emplace_back(std::complex<double> (out_reals[j], out_reals[N/2 + j]));



//    for (const auto v : out_cplx) {
//        std::cout << v << " ";
//    }
    std::cout << std::endl;

    if (mkhdf) {

        hf::File file("/tmp/new_file.h5", hf::File::ReadWrite | hf::File::Create | hf::File::Truncate);

        std::vector<std::complex<double>> data(50, std::complex<double>(1,1));
        std::vector<std::complex<double>> data2(50, std::complex<double>(2,2));

        std::vector<bool> dmask(50, false);

        // let's create a dataset of native integer with the size of the vector 'data'

        //    hf::DataSet dataset1 = file.createDataSet<std::complex<double>>("/Northern Mining/Sarıçam/Hx",  hf::DataSpace::From(data));
        //    hf::DataSet dataset2 = file.createDataSet<std::complex<double>>("/Northern Mining/Sarıçam/Ex",  hf::DataSpace::From(data2));
        auto nm = file.createGroup("Northern Mining");  // survey
        auto site = nm.createGroup("Sarıçam");          // station
        auto run = site.createGroup("1");
        run.createAttribute("sample_freq", double(1024));
        hf::DataSet dataset1 = run.createDataSet<std::complex<double>>("Hx",  hf::DataSpace::From(data));
        hf::DataSet dataset2 = run.createDataSet<std::complex<double>>("Ex",  hf::DataSpace::From(data2));

        //hf::DataSet dataset1b = run.createDataSet<bool>("Hx",  hf::DataSpace::From(dmask));



        dataset2.createAttribute("units", std::string("mV/km"));
        dataset1.createAttribute("units", std::string("mV"));
        // dataset1b.createAttribute("units", std::string("true/false"));



        // let's write our vector of int to the HDF5 dataset
        dataset1.write(data);
        dataset2.write(data2);
        // dataset1b.write(dmask);


        file.flush();

        hf::File filei("/tmp/new_file.h5", hf::File::ReadOnly);

        auto surv = filei.listObjectNames();

        for (const auto &s : surv) {
            std::cout << s << std::endl;
            site = filei.getGroup(s);



        }


    }

    // read back
    //    std::vector<int> result;
    //    dataset.read(result);


    return 0;
}
