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


    auto gplt = std::make_unique<gnuplotter>(1);

   // FILE *plotHandle = popen("gnuplot -persist\n", "w");
        // fprintf(pipe, "\n");

    size_t sz(4), i;
    std::vector<double> a(sz), b(sz), c(sz);

    for (i = 0; i < sz; ++i) {
        a[i] = i;
        b[i] = 3 + i * 2;
        c[i] = 12 + i * 2;

    }

    std::stringstream str;
    std::stringstream datas;


    gplt->cmd << "set terminal qt size 1024,768" << std::endl;
    gplt->cmd << "set title 'Fplot'" << std::endl;
    //gplt->cmd << "set key off" << std::endl;

    gplt->cmd << "set xlabel 'frequency'" << std::endl;
    gplt->cmd << "set ylabel 'Amplitude'" << std::endl;
    gplt->cmd << "set grid" << std::endl;
    gplt->set_x_range(0, sz);
    gplt->set_xy_linespoints(a, b, "in", 1, "lc rgb 'red' lt 5 pt 14 ps 2 pi -1");
    gplt->set_xy_linespoints(a, c, "out", 1, "lc rgb 'red' lt 5 pt 14 ps 2");
    gplt->plot();

    std::cout << gplt->cmd.str() << std::endl;
    std::cout << gplt->stmp.str() << std::endl;
    std::cout << gplt->data.str() << std::endl;


    std::cout << "run finished - w/o exception" << std::endl;
    return 0;
}
