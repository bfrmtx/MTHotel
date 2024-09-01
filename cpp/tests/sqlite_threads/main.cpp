#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

#include "messages.h"
#include "sqlite_status_thread.h"
#include <chrono>

#include "json.h"
#include "ring_buf_micro.h"

using jsn = nlohmann::ordered_json;
int main() {

  std::exception_ptr ex_ptr;
  const size_t channels = 5;
  std::filesystem::path sqlfile_log(std::filesystem::temp_directory_path() / "log_test.sql3");
  std::filesystem::path sqlfile_status(std::filesystem::temp_directory_path() / "status.sql3");
  std::filesystem::path logdir(std::filesystem::temp_directory_path());

  std::filesystem::path http_dir("/srv/http");
  std::filesystem::path channel_file(http_dir / "adu/system/json/ADU-10e/status/channel.json");
  std::filesystem::path system_file(http_dir / "adu/system/json/ADU-10e/status/system.json");
  std::filesystem::path gps_file(http_dir / "adu/system/json/ADU-10e/status/gps.json");

  std::unique_ptr<sqlite_receiver>
      logger; // logs like battery voltage, gps position, adc values and so on

  try {
    // make sure to use the template type of the receiver
    logger = std::make_unique<sqlite_receiver>(sqlfile_status, sqlfile_log); // create logger receiver

  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    logger.reset();
    return EXIT_FAILURE;
  }

  std::cout << "logger and status created" << std::endl;

  std::unordered_map<std::string, std::unique_ptr<sqlite_sender>> status_senders;
  // create a sender using the logger
  // first sender will create the table and start the receiver thread

  status_senders.emplace("adu", logger->create_status_sender("adu", system_file));
  for (size_t i = 0; i < channels; ++i) {
    std::stringstream ss;
    ss << "ch" << i;
    status_senders.emplace(ss.str(), logger->create_status_sender(ss.str(), channel_file));
  }
  status_senders.emplace("gps", logger->create_status_sender("gps", gps_file));

  std::cout << "status senders created" << std::endl;
  // set some key values
  try {
    // status_senders["adu"]->log_only_message("logging starts", "2021-01-01");
    status_senders["ch0"]->set_key_value("channel", "Ex");
    status_senders["ch1"]->set_key_value("channel", "Ey");
    status_senders["ch0"]->set_key_value("gain", 1.0);
    status_senders["ch1"]->set_key_value("gain", 1.0);
    status_senders["ch0"]->set_key_value("angle", 0.0);
    status_senders["ch1"]->set_key_value("angle", 90.0);
    status_senders["ch0"]->set_key_value("tilt", 0.0);
    status_senders["ch1"]->set_key_value("tilt", 0.0);
    status_senders["ch0"]->set_key_value("sensor", "EFP-06");
    status_senders["ch1"]->set_key_value("sensor", "EFP-06");
    // status_senders["gps"]->set_key_value("tracked_sats", 4); force an error
    status_senders["gps"]->set_key_value("tracked", 4);
    status_senders["gps"]->set_key_value("in_use", 3);
    status_senders["gps"]->set_key_value("latitude", 39.0261966);
    status_senders["gps"]->set_key_value("longitude", 29.12395333);
    status_senders["gps"]->set_key_value("elevation", 340.0);
    status_senders["adu"]->set_key_value("bat_1", 12.5); // space in key works - but maybe a bad idea
    status_senders["adu"]->set_key_value("bat_2", 11.5);
  }
  // catch std::runtime_error
  catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    status_senders.clear();
    return EXIT_FAILURE;
  }
  // catch all
  catch (...) {
    std::cerr << "unknown error" << std::endl;
    status_senders.clear();
    return EXIT_FAILURE;
  }

  try {
    status_senders["adu"]->watchdog();
    status_senders["adu"]->run_double("bat 1", 11.5, 14.2, 1500);
    status_senders["adu"]->run_double("bat_2", 11.5, 14.2, 1510);
    status_senders["gps"]->run_int("tracked", 3, 8, 1000);

  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    status_senders.clear();
    return EXIT_FAILURE;
  } catch (const std::exception &e) {
    std::cerr << "in main" << std::endl;
    std::cerr << e.what() << '\n';
    status_senders.clear();
    return EXIT_FAILURE;
  }

  std::cout << "wait for key pressed" << std::endl;
  std::cin.get();

  // gracefully stop the sender threads
  status_senders.clear();

  std::cout << "finish" << std::endl;
  return EXIT_SUCCESS;
}
