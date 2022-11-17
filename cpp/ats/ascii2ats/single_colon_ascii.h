#ifndef SINGLE_COLON_ASCII_H
#define SINGLE_COLON_ASCII_H

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cmath>

std::vector<double> read_single_colon_ascii(const std::filesystem::path filename) {
    std::vector<double> data;
    if (!std::filesystem::exists(filename)) {
        std::string err_str = __func__;
        err_str += "::  file not exists " + filename.string();
        throw err_str;
        return data;
    }

    std::fstream afile;
    std::string str;
    size_t sz = 0;

    afile.open(filename, std::ios::in);
    if (!afile.is_open()) {
        std::string err_str = __func__;
        err_str += ":: can not open ascii for reading " + filename.string();
        throw err_str;
        return data;
    }

    data.reserve(10000000);

    while (std::getline(afile, str)) {
        data.push_back(std::stod(str));
    }

    return data;
}



#endif // SINGLE_COLON_ASCII_H
