#include <iostream>
#include <string>

#include "atss.h"

int main(int argc, char *argv[]) {
  std::vector<std::shared_ptr<channel>> channels;
  // Check if the channel argument is provided
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " file" << std::endl;
    return 1;
  } else {
    for (int i = 1; i < argc; i++) {
      std::string filename(argv[i]);
      channels.push_back(std::make_shared<channel>(filename));
    }
  }

  for (auto &ch : channels) {
    std::cout << "writing ascii file for " << ch->filename() << "\n";
    ch->to_ascii();
  }

  return 0;
}