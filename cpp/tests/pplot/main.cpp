#include <iostream>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <complex>
#include <chrono>
#include <filesystem>
#include <algorithm>


#include "matplotlibcpp.h"
namespace plt = matplotlibcpp;

int main()
{



    // u and v are respectively the x and y components of the arrows we're plotting
//       std::vector<int> x, y, u, v;
//       for (int i = -5; i <= 5; i++) {
//           for (int j = -5; j <= 5; j++) {
//               x.push_back(i);
//               u.push_back(-i);
//               y.push_back(j);
//               v.push_back(-j);
//           }
//       }

//       plt::quiver(x, y, u, v);
//       plt::show();

    //plt::backend("QtAgg");





    std::vector<double> t(100);
    std::vector<double> l(t.size());

    for(size_t i = 1; i < t.size(); i++) {
        t[i] = i+20;
        l[i] = i+90;
    }
    //plt::plot(t, l);
    plt::loglog(t,l);


    plt::title("Sample figure/data");

//    for(size_t i = 1; i < t.size(); i++) {
//        t[i] = i / 100.0;
//        l[i] = 50 + sin(2.0 * M_PI * 1.0 * t[i]);
//    }
//    plt::plot(t, l);
    plt::show();
/*
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
    //        std::cout << std::endl;


    return 0;
}
