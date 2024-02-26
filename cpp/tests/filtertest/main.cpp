#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../../include/sqlite_handler.h"
#include "../../include/survey.h"
#include "../../mt/fir_filter/fir_filter.h"
namespace fs = std::filesystem;

int main(int argc, char **argv) {

  std::string filter_name;
  std::shared_ptr<survey_d> survey;
  std::shared_ptr<station_d> station;
  std::vector<std::shared_ptr<run_d>> runs;
  std::vector<std::shared_ptr<channel>> channels;
  std::vector<std::shared_ptr<channel>> filter_channels;
  fs::path out_dir;

  auto pool = std::make_shared<BS::thread_pool>();

  bool br = false; // prepare for runs

  unsigned l = 1;
  try {

    while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
      std::string marg(argv[l]);

      if (marg.compare("-u") == 0) {
        fs::path survey_name = std::string(argv[++l]);
        if (!fs::is_directory(survey_name)) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " -u survey_dir needs an existing directory with stations inside" << std::endl;
          err_str << " given: " << survey_name.string() << std::endl;
          throw std::runtime_error(err_str.str());
        }
        survey_name = fs::canonical(survey_name);
        survey = std::make_shared<survey_d>(survey_name);
        if (survey == nullptr) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " first use -u survey_name in order to init the survey";
          throw std::runtime_error(err_str.str());
        }
      }
      if (marg.compare("-s") == 0) {
        if (survey == nullptr) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " first use -u surveyname in order to init the survey";
          throw std::runtime_error(err_str.str());
        }
        auto station_name = std::string(argv[++l]);
        station = survey->get_station(station_name); // that is a shared pointer from survey
        if (station == nullptr) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " secondly use -s station_name in order to init station";
          throw std::runtime_error(err_str.str());
        }
      }
      if (marg.compare("-r") == 0) {
        br = true; // prepare for runs
      }

      if (marg.compare("-fil") == 0) {
        filter_name = std::string(argv[++l]);
      }

      if (marg.compare("-o") == 0) {
        out_dir = (argv[++l]);
        if (!fs::is_directory(out_dir)) {
          // create it recursive
          fs::create_directories(out_dir);
        }
        // check if it was created
        if (!fs::is_directory(out_dir)) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << " -o out_dir needs an existing directory; could not create it" << std::endl;
          throw std::runtime_error(err_str.str());
        }
        out_dir = fs::canonical(out_dir);
      }

      if ((marg.compare("-h") == 0) || marg.compare("--help") == 0) {
        std::cout << "usage: " << argv[0] << " [options] [run numbers] [filenames]" << std::endl;
        std::cout << "options:" << std::endl;
        std::cout << "  -u survey_dir" << std::endl;
        std::cout << "  -s station_name" << std::endl;
        std::cout << "  -r run_number(s)" << std::endl;
        std::cout << "  -fil filter_name" << std::endl;
        std::cout << "  -h, --help" << std::endl;
        std::cout << "-fil mtx32 -u /ats_data/Northern_Mining -s Sarıçam -r 7 8" << std::endl;
        std::cout << "\nOR\n"
                  << std::endl;
        std::cout << "-fil mtx32 -o out_dir filename1 filename2 filename3 ..." << std::endl;
        return EXIT_SUCCESS;
      }

      if (marg.compare("-") == 0) {
        std::cerr << "\nunrecognized option " << argv[l] << std::endl;
        return EXIT_FAILURE;
      }

      ++l;
    }

    // all options including the last -r is here, now get the runs
    while ((l < unsigned(argc))) {
      std::string marg(argv[l]);
      if ((survey != nullptr) && (station != nullptr) && br) {
        size_t ri = stoul(marg);
        runs.emplace_back(station->get_run(ri));
      }
      // else take file names as channels
      else {
        channels.emplace_back(std::make_shared<channel>(marg));
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
  if (!runs.size() && br) {
    std::cerr << "no run numbers given" << std::endl;
    return EXIT_FAILURE;
  } else if (!channels.size() && !br) {
    std::cerr << "no filenames for channels given" << std::endl;
    return EXIT_FAILURE;
  }

  // check if out_dir is set above and not empty
  if (out_dir.empty() && !br) {
    std::cerr << "no output directory given" << std::endl;
    return EXIT_FAILURE;
  }

  std::vector<std::shared_ptr<fir_filter>> filters;

  // *************************** start with the simple file i/o without survey ********************************
  if (!br) {

    if (channels.size()) {
      for (auto &chan : channels) {
        filters.emplace_back(std::make_shared<fir_filter>());
        try {
          auto out_chan = filters.back()->set_filter(chan, filter_name);
          out_chan->set_dir(out_dir);
          filter_channels.emplace_back(out_chan);
        } catch (const std::runtime_error &error) {
          std::cerr << error.what() << std::endl;
          return EXIT_FAILURE;
        }
      }
    }

    for (const auto &chan : channels) {
      std::cout << chan->get_datetime() << std::endl;
      std::cout << chan->get_sample_rate() << std::endl;
      std::cout << chan->filename() << std::endl;
    }

    size_t i = 0;
    for (const auto &chan : filter_channels) {
      std::cout << filters[i++]->get_info() << std::endl;
      std::cout << chan->get_datetime() << std::endl;
      std::cout << chan->get_sample_rate() << std::endl;
      std::cout << chan->get_filepath_wo_ext() << std::endl;
      chan->write_header();
    }

    try {
      size_t thread_index = 0;
      for (auto &filter : filters) {
        std::cout << "emplacing thread " << thread_index++ << std::endl;
        // filter
        pool->push_task(&fir_filter::filter, filter);
      }
      pool->wait_for_tasks();

    } catch (const std::runtime_error &error) {
      std::cerr << error.what() << std::endl;
      std::cerr << "could not execute filter" << std::endl;
      return EXIT_FAILURE;
    } catch (...) {
      std::cerr << "could not execute filter" << std::endl;
      return EXIT_FAILURE;
    }
    std::cout << "done" << std::endl;
    return EXIT_SUCCESS;
  }
  // *************************** end with the simple file i/o without survey ********************************

  // *************************** start with the survey ********************************

  // get the runs
  std::vector<std::shared_ptr<channel>> temp_channels;
  std::vector<fs::path> new_runs;
  if (runs.size()) {
    for (auto &run : runs) {
      auto chan = run->get_channels();
      if (chan.size()) {
        for (auto &ch : chan) {
          filters.emplace_back(std::make_shared<fir_filter>());
          try {
            auto out_chan = filters.back()->set_filter(ch, filter_name);
            out_chan->set_dir(out_dir);
            filter_channels.emplace_back(out_chan);
            temp_channels.emplace_back(out_chan);
          } catch (const std::runtime_error &error) {
            std::cerr << error.what() << std::endl;
            return EXIT_FAILURE;
          }
        }
        for (auto &tchan : temp_channels) {
          // if new
          auto new_run = station->add_create_run(tchan);
          if (!new_run.empty()) {
            // if new_run is not in new_runs then add it
            if (std::find(new_runs.begin(), new_runs.end(), new_run) == new_runs.end())
              new_runs.emplace_back(new_run);
          }
          tchan->write_header();
        }
        temp_channels.clear();
      }
      // channels.insert(channels.end(), chan.begin(), chan.end());
    }
  }
  std::vector<std::pair<std::string, size_t>> new_runs_with_index;
  // for (const auto &r : new_runs) {
  //   std::cout << "new run: " << r << std::endl;
  //   new_runs_with_index.emplace_back(survey->get_station_run(r));
  // }
  for (const auto &r : new_runs_with_index) {
    std::cout << "new run: " << r.first << " " << r.second << std::endl;
  }

  try {
    size_t thread_index = 0;
    for (auto &filter : filters) {
      std::cout << "emplacing thread " << thread_index++ << std::endl;
      // filter
      pool->push_task(&fir_filter::filter, filter);
    }
    pool->wait_for_tasks();

  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    std::cerr << "could not execute filter" << std::endl;
    return EXIT_FAILURE;
  } catch (...) {
    std::cerr << "could not execute filter" << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "done" << std::endl;
  return EXIT_SUCCESS;

  return (0);
}
