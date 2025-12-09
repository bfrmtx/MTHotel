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
  return 0;
}