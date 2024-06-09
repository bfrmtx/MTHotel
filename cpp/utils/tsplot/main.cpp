#include "atss.h"
#include "freqs.h"
#include "gnuplotter.h"
#include <algorithm>
#include <chrono>
#include <complex>
#include <fftw3.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "marg.h"

#include "mini_math.h"
namespace fs = std::filesystem;

// add doxygen comments for this file
// \file
// \brief reads data from a file and plots time series

/*


-u /home/bfr/devel/ats_data/Eastern_Mining -sb -s LH12 -r 1 2 3 4 5 6 8 9
-u /home/bfr/devel/ats_data/Eastern_Mining -sb -s LH12 -r 5 6
-u /home/bfr/devel/ats_data/Eastern_Mining -sb -s LH12 -r 5 6
-u /home/bfr/devel/ats_data/Northern_Mining -sb -s Sarıçam -r 7 8
-u /home/bfr/devel/ats_data/Northern_Mining -s wwo -r 1 2
-u /home/bfr/devel/ats_data/Northern_Mining -s Sarıçam -r 7 8
-u ~/devel/ats_data/HF_FFT -s YEN1107 -r  2 3  use for training

*/
int main(int argc, char *argv[]) {
  std::cout << "*******************************************************************************" << std::endl
            << std::endl;
  std::cout << "reads data from a survey (e.g. Northern Mining, Station Sarıçam" << std::endl;
  std::cout << "-u survey -s site -c [Ex, Ex, Hx, Hy, Hz] -r  1 2 ... " << std::endl;
  std::cout << "-start 0 -use 1024" << std::endl;
  std::cout << "\nOR give directly files \n"
            << std::endl;
  std::cout << "file1 file2 " << std::endl;
  std::cout << std::endl
            << "*******************************************************************************" << std::endl
            << std::endl;

  size_t start = 0;
  size_t use = 0;
  bool bdetrend = false;
  unsigned l = 1;
  try {
    get_args_single_survey(argc, argv);      // get all survey, station, run, channel arguments
    get_all_files_if_not_survey(argc, argv); // get all files if not survey, station, run, channel arguments
    bool br = false;
    while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
      std::string marg(argv[l]);
      if (marg.compare("-start") == 0) {
        start = std::stoul(argv[++l]);
      }
      if (marg.compare("-use") == 0) {
        use = std::stoul(argv[++l]);
      }
      if (marg.compare("-d") == 0) {
        bdetrend = true;
      }
      if (marg.compare("-") == 0) {
        std::cerr << "\nunrecognized option " << argv[l] << std::endl;
        return EXIT_FAILURE;
      }

      ++l;
    }

  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const std::invalid_argument &ia) {
    std::cerr << ia.what() << std::endl;
    std::cerr << "invalid run number" << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "general error" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "plotting: " << std::endl;

  for (const auto &chan : channels) {
    std::cout << chan->filename() << std::endl;
  }

  // start with gnuplot
  std::string init_err;
  auto gplt = std::make_shared<gnuplotter<double, double>>(init_err);
  if (!init_err.empty()) {
    std::cerr << init_err << std::endl;
    return EXIT_FAILURE;
  }
  gplt->cmd << "set terminal qt size 2048,768 enhanced" << std::endl;
  // gplt->cmd << "set title '" << all_coils_title << "'" << std::endl;
  gplt->cmd << "set xlabel '[samples]'" << std::endl;
  gplt->cmd << "set ylabel 'amplitude [mV || mV/km]'" << std::endl;
  gplt->cmd << "set key font \"Hack, 10\"" << std::endl;

  std::vector<double> x;
  for (size_t i = start; i < use + start; ++i) {
    x.push_back(double(i));
  }

  for (auto &chan : channels) {

    gplt->set_xy_lines(x, chan->read_single(use, start, bdetrend), chan->channel_type, 1);
  }

  gplt->plot();

  return EXIT_SUCCESS;
}
