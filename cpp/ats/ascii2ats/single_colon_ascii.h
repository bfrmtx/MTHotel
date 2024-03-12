#ifndef SINGLE_COLON_ASCII_H
#define SINGLE_COLON_ASCII_H

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

std::vector<double> read_single_colon_ascii(const std::filesystem::path filename) {
  std::vector<double> data;
  if (!std::filesystem::exists(filename)) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << "::  file not exists " << filename;
    throw std::runtime_error(err_str.str());
  }

  std::fstream afile;
  std::string str;

  afile.open(filename, std::ios::in);
  if (!afile.is_open()) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    err_str << ":: can not open ascii for reading " << filename;
    throw std::runtime_error(err_str.str());
  }

  data.reserve(10000000);

  while (std::getline(afile, str)) {
    std::stringstream ss;
    ss << str;
    double d;
    ss >> d;
    data.push_back(d);
  }
  data.shrink_to_fit();
  return data;
}

#endif // SINGLE_COLON_ASCII_H
