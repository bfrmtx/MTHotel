#include "atss.h"
#include <iostream>
#include <vector>

bool a = false;                                 //!< a all details
bool c = false;                                 //!< c calibration details
std::vector<std::shared_ptr<channel>> channels; //!< channels

int main(int argc, char *argv[]) {
  unsigned l = 1;
  try {

    bool br = false;
    while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
      std::string marg(argv[l]);

      if (marg.compare("-a") == 0) {
        a = true;
      }
      if (marg.compare("-c") == 0) {
        c = true;
      }

      if ((marg.compare("-h") == 0) || marg.compare("--help") == 0) {
        std::cout << "usage: " << argv[0] << " [options] [run numbers] [filenames]" << std::endl;
        std::cout << "options:" << std::endl;
        std::cout << "  -a all details" << std::endl;
        std::cout << "  -c calibration details" << std::endl;
        std::cout << "  -h, --help" << std::endl;
        std::cout << " files" << std::endl;
        return EXIT_SUCCESS;
      }

      if (marg.compare("-") == 0) {
        std::cerr << "\nunrecognized option " << argv[l] << std::endl;
        return EXIT_FAILURE;
      }

      ++l;
    }

    // all options are here, now get the files
    while ((l < unsigned(argc))) {
      std::string marg(argv[l]);
      channels.emplace_back(std::make_shared<channel>(marg));
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
  if (!channels.size()) {
    std::cerr << "no filenames  given" << std::endl;
    return EXIT_FAILURE;
  }

  for (const auto &chan : channels) {
    std::cout << chan->get_run_dir() << " " << chan->filename() << " " << chan->start_datetime(1) << " " << chan->start_datetime(2) << " " << chan->stop_datetime(1) << " " << chan->stop_datetime(2) << std::endl;
  }
  return 0;
}
