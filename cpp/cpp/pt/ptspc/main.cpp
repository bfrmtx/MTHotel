#include "BS_thread_pool.hpp"
#include "ptspc_lib.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <list>
#include <memory>
#include <string>

// ./ptspc -u /home/bfr/devel/ats_data/pst -highres -s MFS-07e_796_MFS-07e_798_MFS-07e_797 -r 1 -c 2 3 4 -r 2 -c 2 3 4

int main(int argc, char **argv) {
  auto pool = std::make_shared<BS::thread_pool<BS::tp::none>>();

  try {
    ptspc_lib processor(pool);
    std::list<std::string> arguments;
    for (int index = 1; index < argc; ++index) {
      arguments.emplace_back(argv[index]);
    }

    processor.get_options(arguments, false);
    processor.process_station_configs(true);
    processor.prepare_fft(true);
    processor.process_raw_spectra();
    processor.set_inner_outer_frequencies_prepare_spectra();
    processor.move_raw_spectra();
    processor.prepare_auto_spectra(true);
    processor.stack_spectra();
    processor.save();
  } catch (const std::exception &error) {
    std::cerr << "ptspc: " << error.what() << '\n';
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}