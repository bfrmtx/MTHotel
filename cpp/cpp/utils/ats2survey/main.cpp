#include "BS_thread_pool.hpp"
#include "ats2atss.hpp"
#include "survey_tree.hpp"

int main(int argc, char **argv) {
  // Your code here
  command_line_options_ats2atss options(argc, argv);

  std::cout << "Starting ATS to ATSS conversion..." << std::endl;
  std::filesystem::path survey_path_ats = options.survey_path_ats;
  std::filesystem::path survey_tree_path = options.survey_tree_path;
  bool verbose = options.verbose;
  bool no_e = options.no_e;
  size_t start_run = options.start_run;
  std::vector<std::string> stations = options.stations;
  std::cout << "Scanning ATSS target survey directory: " << survey_tree_path << std::endl;
  auto survey = std::make_shared<survey_tree>(survey_tree_path);
  survey->scan(); // Scan the directory to populate the tree
  if (verbose) {
    survey->list_children_recursive(); // List all children in the survey tree
  }
  auto ats_survey = std::make_shared<survey_ats>(survey_path_ats);
  std::cout << "Scanning ATS survey directory: " << survey_path_ats << std::endl;
  ats_survey->scan(); // Scan the ATS survey directory to populate the stations
  if (verbose) {
    ats_survey->ls(); // List the stations in the survey
  }
  std::cout << "Survey scanned, found " << ats_survey->stations.size() << " stations." << std::endl;
  // we first add the stations, we can use the public map of ats_survey to add the stations to the survey_tree
  for (const auto &[station_name, station] : ats_survey->stations) {
    try {
      // if stations is empty, we add all stations, else we only add the station in the stations vector
      if (!stations.empty() && std::find(stations.begin(), stations.end(), station_name) == stations.end()) {
        std::cout << "Skipping station: " << station_name << " as it is not in the list of stations to process." << std::endl;
        continue; // skip this station
      }
      std::cout << "Adding station: " << station_name << std::endl;
      auto station_tree = survey->add_child(station_name); // Add the station to the survey tree
      // add the runs from station to station_tree
      for (const auto &[run_path, run] : station->runs) {
        std::cout << "Adding run: " << run_path.filename() << " to station: " << station_name << std::endl;
        auto run_tree = station_tree->addAutoRun(start_run, true); // Add a run to the station tree
        run->set_output_dir(run_tree->get_path());                 // Set the output directory for the run
        run->write_headers(no_e);                                  // Write the headers for the run
      }
    } catch (const std::exception &e) {
      std::cerr << "Error adding station " << station_name << ": " << e.what() << std::endl;
    }
  }

  auto pool = std::make_shared<BS::thread_pool<BS::tp::none>>();
  // convert_data for each run in the survey
  for (const auto &[station_name, station] : ats_survey->stations) {
    // if stations is empty, we process all stations, else we only process stations in the vector
    if (!stations.empty() && std::find(stations.begin(), stations.end(), station_name) == stations.end()) {
      std::cout << "Skipping conversion for station: " << station_name << std::endl;
      continue; // skip this station
    }
    for (const auto &[run_path, run] : station->runs) {
      std::cout << "Converting data for run: " << run_path.filename() << std::endl;
      run->convert_data(pool, no_e); // Convert the data for each run
    }
  }
  // Wait for all tasks to finish
  std::cout << "Waiting for all conversion tasks to finish..." << std::endl;

  pool->wait();
  std::cout << "All conversion tasks finished." << std::endl;
  return EXIT_SUCCESS;
}
