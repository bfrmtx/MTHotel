#ifndef MARG_H
#define MARG_H

#include "atss.h"
#include "survey.h"
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

std::shared_ptr<survey_d> survey;
std::shared_ptr<station_d> station;
std::vector<std::shared_ptr<run_d>> runs;
std::vector<std::shared_ptr<channel>> channels;
std::vector<std::string> channel_types;

static void get_args_single_survey(int argc, char *argv[]) {
  unsigned l = 1;
  // read all arguments starting with "-"; options after "-"  like 1 2 3 must be completely read"
  // so that the next option can be read , starting with "-"
  while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
    std::string marg(argv[l]);

    if (marg.compare("-u") == 0) {
      std::filesystem::path survey_name = std::string(argv[++l]);
      if (!std::filesystem::is_directory(survey_name)) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " -u survey_dir needs an existing directory with stations inside" << std::endl;
        err_str << " given: " << survey_name.string() << std::endl;
        throw std::runtime_error(err_str.str());
      }
      survey_name = std::filesystem::canonical(survey_name);
      // create the survey
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
        err_str << " first use -u survey_name in order to init the survey";
        throw std::runtime_error(err_str.str());
      }
      auto station_name = std::string(argv[++l]);
      // create the station
      station = survey->get_station(station_name); // that is a shared pointer from survey
      if (station == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " secondly use -s station_name in order to init station";
        throw std::runtime_error(err_str.str());
      }
    }
    if (marg.compare("-r") == 0) {
      if (station == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " first use -u survey_name in order to init the survey";
        err_str << " secondly use -s station_name in order to init station";
        throw std::runtime_error(err_str.str());
      }
      while (l < unsigned(argc) && *argv[l + 1] != '-') {
        std::string s_run_no = std::string(argv[++l]);
        // if s_run_no is a number
        if (std::all_of(s_run_no.begin(), s_run_no.end(), ::isdigit)) {
          size_t run_no = std::stoul(s_run_no);
          auto run = station->get_run(run_no);
          if (run == nullptr) {
            std::ostringstream err_str(__func__, std::ios_base::ate);
            err_str << station->get_name() << " has no run with number " << run_no << std::endl;
            throw std::runtime_error(err_str.str());
          }
          // create and append the runs
          runs.push_back(run);
        } else {
          // l was incremented before, so decrement it
          --l;
          break;
        }
      }
    }
    if (marg.compare("-c") == 0) {
      if (runs.size() == 0) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << " first use -u survey_name in order to init the survey";
        err_str << " secondly use -s station_name in order to init station";
        err_str << " thirdly use -r run_no in order to init run";
        throw std::runtime_error(err_str.str());
      }
      auto channel_name = std::string(argv[++l]);
      for (const auto &run : runs) {
        auto channel = run->get_channel(channel_name);
        if (channel == nullptr) {
          std::ostringstream err_str(__func__, std::ios_base::ate);
          err_str << run->get_name() << " has no channel with name " << channel_name << std::endl;
          throw std::runtime_error(err_str.str());
        }
        // create and append the channels
        channels.push_back(channel);
      }
    }
    ++l;
  }
}

static void get_all_files_if_not_survey(int argc, char *argv[]) {
  // here we assume to have survey, station, run as nullptr
  // check for nullptr fist
  if (survey != nullptr || station != nullptr || runs.size() != 0) {
    std::ostringstream err_str(__func__, std::ios_base::ate);
    throw std::runtime_error(err_str.str());
  }
  // scan all arguments from the end
  std::list<std::filesystem::path> file_names;
  for (int i = argc - 1; i > 0; --i) {
    std::string marg(argv[i]);
    if (std::filesystem::is_regular_file(marg)) {
      // store the file names
      file_names.push_front(marg);
    }
  }
  // first make them canonical
  // now we have all file names; remove all extensions from file names
  for (auto &file_name : file_names) {
    file_name = std::filesystem::canonical(file_name);
    file_name.replace_extension();
  }
  // now check for duplicates using full path
  file_names.sort();
  file_names.unique();

  // now we have all file names
  // copy and create from file_names into channels
  for (const auto &file_name : file_names) {
    auto chan = std::make_shared<channel>(file_name);
    if (chan == nullptr) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << " could not create channel from file " << file_name << std::endl;
      throw std::runtime_error(err_str.str());
    }
    channels.push_back(chan);
  }
}

#endif // MARG_H