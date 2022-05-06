#ifndef ATSSFILE_H
#define ATSSFILE_H

#include "../../include/json.h"

#include <complex>
#include <vector>

class atssfile
{
public:
    atssfile();
    std::vector<double> dat;
    std::vector<std::complex<double>> cal;
    std::vector<std::complex<double>> spc;
};

#endif // ATSSFILE_H
