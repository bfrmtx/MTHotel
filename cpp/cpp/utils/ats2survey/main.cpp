#include "BS_thread_pool.hpp"
#include "ats2atss.hpp"
#include "survey_tree.hpp"

int main(int argc, char **argv) {
  // Your code here
  command_line_options_ats2atss options(argc, argv);

  std::cout << "Starting ATS to ATSS conversion..." << std::endl;
  if (options.help) {
    return EXIT_SUCCESS; // help was requested, exit
  }
  // first we check if we make a simple directory conversion
  if (options.in_directory != "" && options.out_directory != "") {
    std::cout << "Performing simple directory conversion from " << options.in_directory << " to " << options.out_directory << std::endl;
    // we do a simple directory conversion
    try {
      // we go for bare metal here and make a vector of ats2atss objects for each ats file in the in_directory
      std::filesystem::path in_dir(options.in_directory);
      std::filesystem::path out_dir(options.out_directory);
      // check that in_dir exists
      if (!std::filesystem::exists(in_dir) || !std::filesystem::is_directory(in_dir)) {
        std::cerr << "Input directory does not exist or is not a directory: " << in_dir << std::endl;
        return EXIT_FAILURE;
      }
      // create output directory if it does not exist
      if (!std::filesystem::exists(out_dir)) {
        std::filesystem::create_directories(out_dir);
      }
      // append the lowest level directory name from in_dir to out_dir to create a new subdirectory
      out_dir /= in_dir.filename();
      ;
      if (!std::filesystem::exists(out_dir)) {
        std::filesystem::create_directories(out_dir);
      }

      std::vector<std::shared_ptr<ats2atss>> conversions; // and look for .ats files
      std::vector<std::shared_ptr<channel>> channels;
      for (const auto &entry : std::filesystem::directory_iterator(in_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".ats") {
          std::cout << "Found ATS file: " << entry.path() << std::endl;
          auto conversion = std::make_shared<ats2atss>(entry.path());
          auto chan = conversion->convert_header();
          if (options.no_e && chan->is_type("E")) {
            std::cout << "Skipping E channel as per command line option." << std::endl;
            continue; // skip E channels if no_e is set
          }
          conversions.push_back(conversion);
        }
      }
      std::cout << "Found " << conversions.size() << " ATS files to convert." << std::endl;
      // write headers into out_dir
      for (size_t i = 0; i < conversions.size(); ++i) {
        auto &conversion = conversions[i];
        auto chan = conversion->convert_header();
        chan->set_dir(out_dir);
        chan->write_header(); // no_e = false
        channels.push_back(chan);
        conversion->set_can_convert_data(true); // we can convert data now
      }
      // now convert data using a thread pool
      auto pool = std::make_shared<BS::thread_pool<BS::tp::none>>();
      for (const auto &conversion : conversions) {
        pool->detach_task([conversion, no_e = options.no_e]() {
          conversion->convert_data(no_e);
        });
      }
      // for (size_t i = 0; i < conversions.size(); ++i) {
      //   auto &conversion = conversions[i];
      //   conversion->convert_data(); // no_e = false
      // }
      //   std::cout << "Converting data for ATS file: " << conversion->ats_file->get_ats_path() << std::endl;
      //   pool->enqueue([conversion]() {
      //     conversion->convert_data(nullptr, false); // no_e = false
      //   });
      // }
      // Wait for all tasks to finish
      std::cout << "Waiting for all conversion tasks to finish..." << std::endl;
      pool->wait(); // Wait for all tasks to finish
      std::cout << "All conversion tasks finished." << std::endl;
    } catch (const std::exception &e) {
      std::cerr << "Error during simple directory conversion: " << e.what() << std::endl;
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }
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
  std::shared_ptr<survey_ats> ats_survey;
  try {
    ats_survey = std::make_shared<survey_ats>(survey_path_ats);
  } catch (const std::exception &e) {
    std::cerr << "Error initializing ATS survey: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
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
