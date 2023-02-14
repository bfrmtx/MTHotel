#include "gnuplotter.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <vector>
#include <thread>
#include <sstream>




int main() {


    std::string init_err;

    auto gplt = std::make_unique<gnuplotter<double, double>>(init_err);

    if (init_err.size()) {
        std::cout << init_err << std::endl;
        return EXIT_FAILURE;
    }

    size_t sz(4), i;
    std::vector<double> a(sz), b(sz), c(sz), d(sz);


    for (i = 0; i < sz; ++i) {
        a[i] = i * 10;
        b[i] = 3 + i * 2;
        c[i] = 12 + i * 2;
        d[i] = 5 + i * 2;
    }

    std::stringstream str;
    std::stringstream datas;


    gplt->cmd << "set terminal qt size 1024,768" << std::endl;
    gplt->cmd << "set title 'Fplot'" << std::endl;
    //gplt->cmd << "set key off" << std::endl;

    gplt->cmd << "set xlabel 'frequency'" << std::endl;
    gplt->cmd << "set ylabel 'Amplitude'" << std::endl;
    gplt->cmd << "set grid" << std::endl;
    gplt->set_x_range(a.front(), a.back());
    gplt->set_xy_linespoints(a, b, "in", 1, "lc rgb 'red' lt 5 pt 14 ps 2 pi -1");
    //gplt->set_xy_linespoints(a, c, "out", 1, "lc rgb 'red' lt 5 pt 14 ps 2");

    // plot '084_ADU-07e_C000_TEx_8s.atss' binary array=(64) format='%float64'
    gplt->set_xy_linespoints(a, d, "bin", 1, "lc rgb 'blue' lt 5 pt 14 ps 2  ");
    gplt->plot();

    for (i = 0; i < sz; ++i) {
        std::cout << a[i] << " " << b[i] << " " << d[i] << std::endl;
    }
    std::cout << std::endl;





    std::cout << "run finished - w/o exception" << std::endl;
    return 0;
}

/*
FILE *p = popen("gnuplot -persist\n", "w");
double data[5] = { 1, 2, 3, 4, 10 };
std::vector<double> dat {1, 2, 3, 4, 10};
if (p)
{
    fprintf(p, "set xrange [1:10]\n");
    fprintf(p, "set yrange [1:15]\n");

    fprintf(p, "plot '-' binary endian=little array=5 format=\"%%float64\" with linespoints\n");
    fwrite(data, sizeof(data), 1, p );
    fwrite()
    fflush(p);

    fflush(stderr);
    getchar();
    fprintf(p,"exit \n");
    pclose(p);
}

*/
