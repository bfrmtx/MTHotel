#include "BS_thread_pool.hpp"
#include "survey_tree.hpp"
#include <filesystem>
#include <iostream>

// std::recursive_mutex survey_d::survey_mutex; // Ensure the global mutex is initialized

std::vector<std::string> stations_to_create = {"Station_1", "Station_2", "Station_3", "Station_4", "Station_5", "Station_6", "Station_7", "Station_8", "Station_9", "Station_10"};

// vector size_t 1... 100
std::vector<size_t> station_runs = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70};

void create_inside_thread(std::shared_ptr<survey_tree> &tree, const std::string &station_name, std::vector<size_t> &runs) {
  try {
    auto station = tree->add_child(station_name);
    if (station) {
      std::cout << "Station added: " << station->get_name() << std::endl;
      // Add runs to the station
      for (const auto &run_no : runs) {
        station->add_child(run_no);
        std::cout << "Run " << run_no << " added to station: " << station->get_name() << std::endl;
      }
    } else {
      std::cerr << "Failed to add station: " << station_name << std::endl;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error while adding station " << station_name << ": " << e.what() << std::endl;
  }
}

int main(int argc, char **argv) {
  // std::filesystem::path survey_path = "/home/bfr/tmp/Eastern_Mining/";

  bool small_test = false;
  // get the command line argument which is true or false for small test
  if (argc > 1) {
    std::string arg1 = argv[1];
    if (arg1 == "small_test") {
      std::cout << "Running small test" << std::endl;
      small_test = true;
    } else {
      std::cout << "Running full test" << std::endl;
    }
  }

  auto logger = std::make_shared<xlogger>();
  std::filesystem::path survey_path = "/tmp/Eastern_Mining/";

  auto pool = std::make_shared<BS::thread_pool<BS::tp::none>>(8);
  std::shared_ptr<survey_tree> survey;
  try {
    survey = std::make_shared<survey_tree>(survey_path.string()); // Create the empty or not empty survey tree at the specified path
  } catch (const std::runtime_error &e) {
    std::cerr << "Runtime error while creating survey tree at " << survey_path << ": " << e.what() << std::endl;
    return 1;
  } catch (const std::exception &e) {
    std::cerr << "Error while creating survey tree at " << survey_path << ": " << e.what() << std::endl;
    return 1;
  }
  survey->scan(); // Scan the directory to populate the tree
  if (small_test) {
    stations_to_create = {"small test"};
    station_runs = {2};
    for (const auto &station_name : stations_to_create) {
      auto station = survey->add_child(station_name);
      if (station) {
        std::cout << "Station added: " << station->get_name() << std::endl;
        // Add runs to the station
        for (const auto &run_no : station_runs) {
          station->add_child(run_no);
          std::cout << "Run " << run_no << " added to station: " << station->get_name() << std::endl;
        }
      } else {
        std::cerr << "Failed to add station: " << station_name << std::endl;
      }
    }

  } else {
    for (const auto &station_name : stations_to_create) {
      pool->detach_task([&survey, &station_name]() {
        create_inside_thread(survey, station_name, station_runs);
      });
    }
    pool->wait();
  }
  survey->list_children(); // List all children in the survey tree
  survey.reset();          // Clear the survey tree to release resources
  // ***********************************************************************
  std::cout << "test 2" << std::endl;
  survey_path = "/home/bfr/tmp/Eastern_Mining/";
  survey = std::make_shared<survey_tree>(survey_path.string());
  survey->scan();                    // Scan the directory to populate the tree
  survey->list_children_recursive(); // List all children in the survey tree

  // try {
  //   tree.set_root(survey_path);
  //   std::cout << "Survey tree initialized with root: " << tree.root() << std::endl;
  //   // Example of getting all stations
  //   auto stations = tree.get_stations();
  //   std::cout << "Number of stations: " << stations.size() << std::endl;
  //   for (const auto &station : stations) {
  //     std::cout << "Station: " << station->name() << ", Path: " << station->path() << std::endl;
  //     // Example of getting runs for each station
  //     auto runs = station->get_runs();
  //     std::cout << "Number of runs in station " << station->name() << ": " << runs.size() << std::endl;
  //     for (const auto &run : runs) {
  //       std::cout << "  Run: " << run->path() << std::endl;
  //       //  run->show_filenames(); // Show channel filenames in the run
  //     }

  //     // // Example of adding a station
  //     // tree.add_child("Sarıçam");
  //     // auto station = tree.get_station("Sarıçam");
  //     // if (station) {
  //     //   std::cout << "Station added: " << station->name() << std::endl;
  //     //   // Add runs to the station
  //     //   station->add_child(1);
  //     //   station->add_child(2);
  //     //   std::cout << "Runs added to station: " << station->name() << std::endl;
  //     // } else {
  //     //   std::cerr << "Failed to add station." << std::endl;
  //   }
  // } catch (const std::exception &e) {
  //   std::cerr << "Error: " << e.what() << std::endl;
  // }
  // // Example of clearing the survey tree
  // try {
  //   tree.clear(); // IMPORTANT! otherwise YOU GET MIXED UP with older / newer instances!!!
  //   std::cout << "Survey tree cleared." << std::endl;
  // } catch (const std::exception &e) {
  //   std::cerr << "Error while clearing survey tree: " << e.what() << std::endl;
  // }

  // // *************************************** ***************************************
  // // Create a survey tree with multiple stations and runs, we using thread-pool to handle

  // std::filesystem::path temp_path = std::filesystem::temp_directory_path();
  // std::cout << "System temporary directory: " << temp_path << std::endl;
  // // survey_tree is implemented as a singleton, so you cannot create a new instance directly.
  // // The only way to access the instance is via survey_tree::instance().
  // std::filesystem::path test_create(temp_path / "survey_test");
  // try {
  //   tree.create_root(test_create);
  // } catch (const std::exception &e) {
  //   std::cerr << "Error while creating survey root: " << e.what() << std::endl;
  //   return 1;
  // }

  // // Create a survey tree with multiple stations and runs, we using thread-pool to handle the creation of runs in parallel
  // // and check the thread-safety of the survey_tree class
  // /*for (const auto &station_name : stations_to_create) {
  //   try {
  //     tree.add_child(station_name);
  //     auto station = tree.get_station(station_name);
  //     if (station) {
  //       std::cout << "Station added: " << station->name() << std::endl;
  //       // Add runs to the station
  //       for (const auto &run_no : station_runs) {
  //         station->add_child(run_no);
  //         std::cout << "Run " << run_no << " added to station: " << station->name() << std::endl;
  //       }
  //     } else {
  //       std::cerr << "Failed to add station: " << station_name << std::endl;
  //     }
  //   } catch (const std::exception &e) {
  //     std::cerr << "Error while adding station " << station_name << ": " << e.what() << std::endl;
  //   }
  // }
  // */
  // for (const auto &station_name : stations_to_create) {
  //   pool->detach_task([&tree, station_name]() {
  //     std::vector<size_t> runs = station_runs; // Copy runs for each station
  //     try {
  //       create_inside_thread(tree, station_name, runs);
  //     } catch (const std::exception &e) {
  //       std::cerr << "Error while adding station " << station_name << ": " << e.what() << std::endl;
  //     } catch (...) {
  //       std::cerr << "Unknown error while adding station " << station_name << std::endl;
  //     }
  //   });
  // }
  // pool->wait();
  return 0;
}