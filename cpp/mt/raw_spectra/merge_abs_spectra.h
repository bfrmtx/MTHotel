#ifndef MERGE_SPECTRA_H
#define MERGE_SPECTRA_H
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "vector_math.h"
#include <iostream>
#include <utility>

template <typename T>
class merge_spectra {

public:
  merge_spectra() = default;
  ~merge_spectra() = default;
  void add_spectra(const std::pair<std::string, std::string> &name, const std::shared_ptr<std::vector<double>> f, const std::shared_ptr<std::vector<T>> &spectra) {
    if (this->spc.find(name) != this->spc.end()) {
      // spectra already exists, we merge the two
      // first move the old spectra to a temporary vector to a new shared pointer
      std::vector<double> old_spectra = *this->spc[name];
      std::vector<T> old_f = *this->f[name];

      // create a new vector with the size of the old one + the new one
      bvec::merge_f_v_avg<double, T>(old_f, old_spectra, *f, *spectra, *this->f[name], *this->spc[name]);
      // show old and new vector for my debugging
      for (size_t i = 0; i < old_f.size(); ++i) {
        std::cout << old_f.at(i) << " " << old_spectra.at(i) << std::endl;
      }
      for (size_t i = 0; i < this->f[name]->size(); ++i) {
        std::cout << this->f[name]->at(i) << " " << this->spc[name]->at(i) << std::endl;
      }

    } else {
      this->spc[name] = spectra;
      this->f[name] = f;
    }
  }
  void add_spectra(const std::pair<std::string, std::string> &name, const std::vector<double> &f, const std::vector<T> &spectra) {
    this->add_spectra(name, std::make_shared<std::vector<double>>(f), std::make_shared<std::vector<T>>(spectra));
  }
  std::vector<T> get_spectra_vec(std::pair<std::string, std::string> &name_out) {
    if (this->spc.find(name_out) != this->spc.end()) {
      return *this->spc[name_out];
    } else {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::no spectra found for " << name_out.first << " " << name_out.second;
      throw std::runtime_error(err_str.str());
    }
  }

  std::vector<double> get_f_vec(std::pair<std::string, std::string> &name_out) {
    if (this->f.find(name_out) != this->f.end()) {
      return *this->f[name_out];
    } else {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::no f found for " << name_out.first << " " << name_out.second;
      throw std::runtime_error(err_str.str());
    }
  }

private:
  /* data */
  std::map<std::pair<std::string, std::string>, std::shared_ptr<std::vector<T>>> spc;
  std::map<std::pair<std::string, std::string>, std::shared_ptr<std::vector<double>>> f;
};

#endif // MERGE_SPECTRA_H