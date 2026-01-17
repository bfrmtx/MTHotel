#include "BS_thread_pool.hpp"
#include "ptspc_lib.h"
#include "survey_tree.hpp"
#include <filesystem>
#include <iostream>

int main(int argc, char **argv) {
  auto pool = std::make_shared<BS::thread_pool<BS::tp::none>>();
  auto ptspc = std::make_unique<ptspc_lib>(pool);
  std::list<std::string> args;
  // collect command line arguments
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
  try {
    ptspc->get_options(args, false);
    // ptspc.read_survey();
  } catch (const std::exception &e) {
    std::cerr << "Error in get_options: " << e.what() << std::endl;
    return 1;
  }
  try {
    ptspc->process_station_configs(true);
  } catch (const std::exception &e) {
    std::cerr << "Error in process_station_configs: " << e.what() << std::endl;
    return 1;
  }
  try {
    std::cout << "Preparing FFT for all channels..." << std::endl;
    ptspc->prepare_fft(true);
  } catch (const std::exception &e) {
    std::cerr << "Error in prepare_fft: " << e.what() << std::endl;
    return 1;
  }
  try {
    ptspc->process_raw_spectra();
  } catch (const std::exception &e) {
    std::cerr << "Error while waiting for thread pool tasks to finish: " << e.what() << std::endl;
    return 1;
  }
  std::cout << "FFT preparation completed." << std::endl;
  try {
    ptspc->set_inner_outer_frequencies_prepare_spectra();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
  }

  // prepare fft for all channels
  std::cout << "All tasks finished successfully." << std::endl;
  return EXIT_SUCCESS;
}