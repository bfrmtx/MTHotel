#include <algorithm>
#include <chrono>
#include <complex>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "cal_synthetic.h"
#include "freqs.h"
#include "gnuplotter.h"
#include "vector_math.h"

int main(int argc, char *argv[]) {

  size_t i, j;
  bool files = false; // change to true for file output
  size_t ppd = 8;     // points per decade

  std::vector<double> freqs;
  double min_f = 0.1, max_f = 1024;
  std::vector<std::vector<std::complex<double>>> trfs;
  std::vector<std::vector<double>> ampls;
  std::vector<std::vector<double>> phs;
  std::vector<std::string> what; // comma separated system functions
  std::vector<std::stringstream> labels;

  bool plot = false;

  int l = 1;
  while (argc > 1 && (l < argc) && *argv[l] == '-') {
    std::string marg(argv[l]);
    if (marg.compare("-files") == 0) {
      files = true;
    }
    if (marg.compare("-min") == 0) {
      min_f = atof(argv[++l]);
    }
    if (marg.compare("-max") == 0) {
      max_f = atof(argv[++l]);
    }
    if (marg.compare("-ppd") == 0) {
      ppd = atoi(argv[++l]);
      if (!ppd || ppd > 100)
        ppd = 8;
    }
    ++l;
  }

  if (argc <= 1) {
    std::cout << "Usage: " << argv[0] << " [-files] [-min min_f] [-max max_f] [-ppd ppd] [gen_trf_adb_08e_lf] [gen_trf_adb_08e_hf]" << std::endl;
    std::cout << "  -f: output to files" << std::endl;
    std::cout << "  -min: minimum frequency" << std::endl;
    std::cout << "  -max: maximum frequency" << std::endl;
    std::cout << "  -ppd: points per decade" << std::endl;
    std::cout << "  gen_trf_adb_08e_lf: generate transfer function for ADB 08e LF board" << std::endl;
    std::cout << "  gen_trf_adb_08e_hf: generate transfer function for ADB 08e HF board" << std::endl;
    std::cout << "  try simply: " << argv[0] << " gen_trf_adb_08e_lf gen_trf_adb_08e_hf" << std::endl;
    return EXIT_FAILURE;
  }

  // read filter options
  while (argc > l && (l < argc)) {
    std::string marg(argv[l]);
    what.emplace_back(marg);
    ++l;
  }

  freqs = gen_equidistant_logvector(min_f, max_f, ppd);

  double resistance = 10000;
  double gain_2 = 16;
  double gain_1 = 16;

  for (const auto &str : what) {
    if (str == "gen_trf_adb_08e_lf") {
      for (j = 0; j < 3; ++j) {
        // RF-1
        trfs.emplace_back(gen_trf_adb_08e_lf(freqs, ADU::LF_RF_1, ADU::off, ADU::div_1, resistance, gain_1));
        bvec::cplx2_vap(trfs.back(), ampls, phs, true);
        labels.emplace_back(std::stringstream());
        labels.back() << "ADB 08e LF, LF-RF-1, res: " << resistance << " Ohm,"
                      << "gain 1: " << gain_1;

        gain_1 /= 4.;
        // resistance *= 5;
      }
    }
  }

  for (auto &fil : what) {
    std::cout << fil << std::endl;
  }

  std::string init_err;

  // ************************ P H A S E **********************************

  auto gplt = std::make_unique<gnuplotter<double, double>>(init_err);
  if (init_err.size()) {
    std::cout << init_err << std::endl;
    return EXIT_FAILURE;
  }

  if (!files)
    gplt->cmd << "set terminal qt size 1024,768 enhanced" << std::endl;
  gplt->cmd << "set title 'ADU-08e LF board Phase'" << std::endl;
  gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
  gplt->cmd << "set ylabel 'phase'" << std::endl;
  gplt->cmd << "set grid" << std::endl;

  gplt->cmd << "set logscale x" << std::endl;
  gplt->set_x_range(min_f, max_f);
  gplt->cmd << "set key left bottom" << std::endl;
  ;
  for (i = 0; i < phs.size(); ++i) {
    gplt->set_xy_lines(freqs, phs[i], labels[i].str(), 1);
  }

  gplt->plot();
  gplt.reset();

  // ************************ A M P L I T U D E **********************************

  gplt = std::make_unique<gnuplotter<double, double>>(init_err);
  if (init_err.size()) {
    std::cout << init_err << std::endl;
    return EXIT_FAILURE;
  }

  if (!files)
    gplt->cmd << "set terminal qt size 1024,768 enhanced" << std::endl;
  gplt->cmd << "set title 'ADU-08e LF board Amplitude'" << std::endl;
  gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
  gplt->cmd << "set ylabel 'amplitude'" << std::endl;
  gplt->cmd << "set grid" << std::endl;

  gplt->cmd << "set logscale x" << std::endl;
  gplt->set_x_range(min_f, max_f);
  gplt->cmd << "set key left bottom" << std::endl;
  ;
  for (i = 0; i < phs.size(); ++i) {
    gplt->set_xy_lines(freqs, ampls[i], labels[i].str(), 1);
  }

  gplt->plot();
  gplt.reset();

  gplt.reset();
  phs.clear();
  ampls.clear();
  trfs.clear();
  labels.clear();

  // ************************************************************************************************************
  gain_1 = 16;
  min_f = 8192;
  max_f = 100000;
  freqs.clear();
  freqs = gen_equidistant_logvector(min_f, max_f, ppd);

  gplt = std::make_unique<gnuplotter<double, double>>(init_err);
  if (init_err.size()) {
    std::cout << init_err << std::endl;
    return EXIT_FAILURE;
  }

  for (const auto &str : what) {
    if (str == "gen_trf_adb_08e_hf") {
      for (j = 0; j < 3; ++j) {
        // RF-1
        trfs.emplace_back(gen_trf_adb_08e_hf(freqs, ADU::off, gain_1));
        bvec::cplx2_vap(trfs.back(), ampls, phs, true);
        labels.emplace_back(std::stringstream());
        labels.back() << "ADB 08e LF, HF, res: " << resistance << " Ohm,"
                      << "gain 1: " << gain_1;

        gain_1 /= 4.;
        // resistance *= 5;
      }
    }
  }
  if (!files)
    gplt->cmd << "set terminal qt size 1024,768 enhanced" << std::endl;
  gplt->cmd << "set title 'ADU-08e HF board Phase'" << std::endl;
  gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
  gplt->cmd << "set ylabel 'phase'" << std::endl;
  gplt->cmd << "set grid" << std::endl;

  gplt->cmd << "set logscale x" << std::endl;
  gplt->set_x_range(min_f, max_f);
  gplt->cmd << "set key left bottom" << std::endl;
  ;
  for (i = 0; i < phs.size(); ++i) {
    gplt->set_xy_lines(freqs, phs[i], labels[i].str(), 1);
  }

  gplt->plot();
  gplt.reset();

  // ************************ A M P L I T U D E **********************************

  gplt = std::make_unique<gnuplotter<double, double>>(init_err);
  if (init_err.size()) {
    std::cout << init_err << std::endl;
    return EXIT_FAILURE;
  }

  if (!files)
    gplt->cmd << "set terminal qt size 1024,768 enhanced" << std::endl;
  gplt->cmd << "set title 'ADU-08e HF board Amplitude'" << std::endl;
  gplt->cmd << "set xlabel 'frequency [Hz]'" << std::endl;
  gplt->cmd << "set ylabel 'amplitude'" << std::endl;
  gplt->cmd << "set grid" << std::endl;

  gplt->cmd << "set logscale x" << std::endl;
  gplt->set_x_range(min_f, max_f);
  gplt->cmd << "set key left bottom" << std::endl;
  ;
  for (i = 0; i < phs.size(); ++i) {
    gplt->set_xy_lines(freqs, ampls[i], labels[i].str(), 1);
  }

  gplt->plot();
  gplt.reset();

  return EXIT_SUCCESS;
}
