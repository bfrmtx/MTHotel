

#include <filesystem>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

// #include "BS_thread_pool.h"
#include "atmheader.h"
#include "atsheader.h"
#include "atsheader_def.h"
#include "atsheader_xml.h"
#include "survey.h"

#include "../../ats/atstools/pt_tojson.h"
//
// pt2surv -outdir /home/bfr/tmp/ptr /home/bfr_mount/geo-nas/GEOFertigung/Feldmessdaten/2023/MFS-06e/2023-11-28/
// scans all ats files;

int main(int argc, char *argv[]) {

  bool tojson = true;                                 //!< convert to JSON and binary - the new format
  std::vector<std::shared_ptr<atsheader>> atsheaders; //!< all ats files or ats headers
  fs::path outdir, clone_dir;
  fs::path json_caldir; //!< find directory with JSON cal files

  bool no_create = true;
  bool no_echannel = false;

  unsigned l = 1;
  while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
    std::string marg(argv[l]);

    if (marg.compare("-") == 0) {
      std::cerr << "\nunrecognized option " << argv[l] << std::endl;
      return EXIT_FAILURE;
    }

    if ((marg.compare("--help") == 0) || (marg.compare("-help") == 0)) {
      std::cout << "pt2surv -outdir /home/bfr/tmp/ptr /home/bfr_mount/2023/MFS-06e/2023-11-28/" << std::endl;
      std::cout << "scans all ats files from that directory and their subdirectories which may contain the pt data" << std::endl;
      std::cout << "e.g. folder input names such as 1064-257-1065, indicating the coils used in the ats subs" << std::endl;
      std::cout << "the output directory will be created if it does not exist" << std::endl;
      std::cout << "the site numbers will be increased when f or sub has changed" << std::endl;
      std::cout << "-no_e : do not use E channel" << std::endl;
      return EXIT_SUCCESS;
    }

    if (marg.compare("-outdir") == 0) {
      outdir = std::string(argv[++l]);
      try {
        if (!fs::exists(outdir)) {
          fs::create_directories(outdir);
          no_create = false;
        }
      } catch (...) {
        std::cerr << "could not create outdir " << argv[l - 1] << std::endl;
        return EXIT_FAILURE;
      }
      // try if outdir exists but is empty
      try {
        if (fs::is_empty(outdir) && fs::exists(outdir) && fs::is_directory(outdir)) {
          fs::create_directories(outdir);
          no_create = false;
        }
      } catch (...) {
        std::cerr << "outdir exists but can not create sub dirs " << argv[l - 1] << std::endl;
        return EXIT_FAILURE;
      }
      outdir = fs::canonical(outdir);
    }
    if (marg.compare("-no_e") == 0) {
      no_echannel = true;
    }
    ++l;
  }

  // **** check for source directory and ats files
  clone_dir = std::string(argv[argc - 1]);
  if (!fs::exists(clone_dir)) {
    std::cerr << "clone directory does not exist! : " << clone_dir << std::endl;
    return EXIT_FAILURE;
  }
  if (clone_dir.empty()) {
    std::cerr << "clone needs a survey directory as last argument" << std::endl;
    return EXIT_FAILURE;
  }
  clone_dir = fs::canonical(clone_dir);

  if (!fs::is_directory(clone_dir)) {
    std::cerr << "clone needs a survey directory as last argument" << std::endl;
    return EXIT_FAILURE;
  }

  // **** check for outdir

  std::unique_ptr<survey_d> survey;
  if (outdir.empty()) {
    std::cout << "please supply -outdir name" << std::endl;
    return EXIT_FAILURE;
  }

  try {
    survey = std::make_unique<survey_d>(outdir, no_create);
  } catch (std::filesystem::filesystem_error &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "create SURVEY" << std::endl;
    return EXIT_FAILURE;
  }

  // **** check for existing stations, otherwise create, starting with pt_1

  auto stations = survey->get_station_names();
  std::filesystem::path station_path;
  std::string station_name;

  if (!stations.size()) {
    std::cout << "no stations found, creating pt_1" << std::endl;
    // station_path = survey->create_station("pt_1");
    station_name = "pt_1";
  } else {
    std::vector<int> numbers;
    for (const auto &station : stations) {
      // std::cout << "station: " << station << std::endl;
      // split at _
      // check if first part is pt
      // check if second part is a number
      // if yes, check if it is the highest number
      // if yes, set it as highest number
      // if no, ignore
      if (station.compare(0, 3, "pt_") == 0) {
        std::string number = station.substr(3);
        std::cout << "pt_" << number << std::endl;
        try {
          numbers.push_back(std::stoi(number));
        } catch (...) {
          std::cerr << "could not convert station number to int" << std::endl;
          return EXIT_FAILURE;
        }
      }
    }
    // sort numbers
    std::sort(numbers.begin(), numbers.end());
    // get highest number
    int highest = numbers.back();
    std::cout << "adding pt_" << ++highest << std::endl;
    // station_path = survey->create_station("pt_" + std::to_string(highest));
    station_name = "pt_" + std::to_string(highest);
  }
  std::string extension_lower = ".ats";
  std::string extension_upper = ".ATS";
  for (auto const &dir_entry : fs::recursive_directory_iterator(clone_dir)) {
    if (dir_entry.is_regular_file() && ((dir_entry.path().extension() == extension_lower) || (dir_entry.path().extension() == extension_upper))) {
      // file_names.push_back( entry.path().string() ) ;

      if (no_echannel && (dir_entry.path().filename().string().find("_TE") == std::string::npos)) {
        std::cout << "found E channel, skip" << std::endl;
        atsheaders.emplace_back(std::make_shared<atsheader>(dir_entry.path()));
      }
      if (!no_echannel) {
        atsheaders.emplace_back(std::make_shared<atsheader>(dir_entry.path()));
      }
    }
  }

  if (!atsheaders.size()) {
    std::cout << "no ats files found" << std::endl;
    return EXIT_FAILURE;
  }

  auto pool = std::make_shared<BS::thread_pool>();

  try {
    for (const auto &atsh : atsheaders) {
      // pool->push_task(collect_atsheaders, std::ref(atsh), std::ref(survey), std::ref(shift_start_time));
      collect_atsheaders(atsh, survey);
    }
    // pool->wait();

  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "could not execute all threads" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "done collecting" << std::endl;

  auto vch = survey->get_all_channels();

  for (const auto &ch : vch) {
    if (ch->cal->is_empty() && fs::exists(json_caldir)) {
      auto read_name = ch->cal->gen_json_filename_from_blank(ch->cal->chopper);
      std::cout << "empty calibration, try to read: " << read_name << std::endl;
      if (fs::exists(json_caldir / read_name)) {
        ch->cal->read_file((json_caldir / read_name), false);
      }
    }
  }

  for (auto &ch : vch) {
    ch->tmp_station = station_name;
    std::cout << ch->get_atss_filepath() << std::endl;
  }

  for (size_t i = 0; i < vch.size(); ++i) {
    std::cout << survey->get_channel_from_all(i)->filename(".json") << std::endl;
  }

  // std::sort(vch.begin(), vch.end(), compare_channel_start_lt);

  try {
    survey->mk_tree();
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "could not execute all threads" << std::endl;
    return EXIT_FAILURE;
  }

  // std::cout << "done make tree" << std::endl;

  try {
    for (size_t i = 0; i < vch.size(); ++i) {
      // fill_survey_tree(survey, i);
      // pool->push_task(fill_survey_tree, std::ref(survey), i);
      pool->detach_task([&survey, i]() { fill_survey_tree(survey, i); });
    }
    pool->wait();
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "could not execute all threads" << std::endl;
    return EXIT_FAILURE;
  }

  return 0;
}