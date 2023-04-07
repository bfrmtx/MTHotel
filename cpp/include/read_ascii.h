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
        std::string err_str = __func__;
        err_str += "::  file not exists " + filename.string();
        throw err_str;
        return;
    }


    std::ifstream afile;
    afile.open(filename, std::ios::in);
    if (!afile.is_open()) {
        std::string err_str = __func__;
        err_str += ":: can not open ascii for reading " + filename.string();
        throw err_str;
        return;
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
        std::string err_str = __func__;
        err_str += ":: data zero or unequal " + filename.string() + ", x: " + std::to_string(xq.size()) + ", y: " + std::to_string(yq.size());
        throw err_str;
        return;
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
