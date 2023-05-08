#ifndef READ_ASCII_H
#define READ_ASCII_H

#include <fstream>
#include <filesystem>
#include <queue>
#include <vector>
#include <string>
#include <sstream>
#include <queue>

void read_two_col(const std::filesystem::path &filename, std::vector<double> &x, std::vector<double> &y) {
    if (!std::filesystem::exists(filename)) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << "::  file not exists " << filename;
        throw err_str.str();
    }


    std::ifstream afile;
    afile.open(filename, std::ios::in);
    if (!afile.is_open()) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << ":: can not open ascii for reading " << filename;
        throw err_str.str();
    }

    std::string line;
    size_t i = 0;
    std::istringstream ss(line);


    double xv, yv;
    std::queue<double> xq, yq;

    while ( afile >> xv >> yv) {
        xq.emplace(xv);
        yq.emplace(yv);
    }

    if ( (xq.size() != yq.size()) || (!xq.size()) || (!yq.size()) ) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << ":: data zero or unequal " << filename <<  ", x: " << xq.size() << ", y: " << yq.size();
        throw err_str.str();
    }
    x.resize(xq.size());
    y.resize(yq.size());

    while (!xq.empty()) {
        x[i] = xq.front();
        y[i] = yq.front();
        ++i;
        xq.pop();
        yq.pop();
    }
}

#endif // READ_ASCII_H
