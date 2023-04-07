#include <iostream>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <complex>
#include <chrono>
#include <filesystem>
#include <algorithm>


#include "read_ascii.h"
#include "gnuplotter.h"




int main(int argc, char* argv[])
{


    size_t i, j;

    std::vector<std::filesystem::path> files;
    std::vector<std::vector<double>> xs;
    std::vector<std::vector<double>> ys;

    bool plot = false;

    int l = 1;
    while (argc > 1 && (l <argc) && *argv[l] == '-') {
        std::string marg(argv[l]);
        if (marg.compare("-plot") == 0) {
            plot = true;
        }
        ++l;
    }


    // read files after options
    while (argc > l && (l < argc)) {
        std::string marg(argv[l]);
        if (!std::filesystem::exists(marg)) {
            std::cerr << marg << " does not EXIST";
            return EXIT_FAILURE;
        }
        files.emplace_back(std::filesystem::path(marg));

        ++l;
    }
    for (auto &file : files) {
        xs.emplace_back(std::vector<double>());
        ys.emplace_back(std::vector<double>());
        read_two_col(file, xs.back(), ys.back());

    }

    std::vector<double> sosx;
    std::vector<double> sosy;

    for (const auto &y : ys) {
        double result = 0;
        for (const auto &yval : y) result += yval * yval;
        sosy.emplace_back(result/double(y.size()));

    }

    if (plot) {
        std::string init_err;
        auto gplt = std::make_unique<gnuplotter<double, double>>(init_err);
        if (init_err.size()) {
            std::cout << init_err << std::endl;
            return EXIT_FAILURE;
        }

        gplt->cmd << "set terminal qt size 2048,1600 enhanced" << std::endl;
        gplt->cmd << "set title 'rda plot'" << std::endl;
        gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
        gplt->cmd << "set ylabel 'amplitude'" << std::endl;
        gplt->cmd << "set grid" << std::endl;

        gplt->cmd << "set logscale xy" << std::endl;
        gplt->cmd << "set xrange [20:250]" << std::endl;

        gplt->cmd << "set obj rect from 47, 0.01 to 53, 0.1 fc lt 2 fillstyle pattern 5 noborder" << std::endl;
        gplt->cmd << "set obj rect from 147, 0.01 to 153, 0.1 fc lt 2 fillstyle pattern 5 noborder" << std::endl;

        try {
            for (size_t i = 0; i < files.size(); ++i) {
                std::stringstream label;
                label << files.at(i).filename();
                gplt->set_xy_lines(xs.at(i), ys.at(i), label.str(), 1);
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
    }

    return EXIT_SUCCESS;
}
