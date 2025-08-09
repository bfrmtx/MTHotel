#include "channel.hpp"
#include "strings_etc.hpp"
#include <iostream>

int main() {
  // Example UTC time string
  std::string utc_time_str = "2009-08-20T13:22:00";
  uint64_t secs_since_epoch = 1250774520; // Example seconds since epoch

  std::cout << std::endl
            << "control values: " << utc_time_str << " " << secs_since_epoch << std::endl
            << std::endl;
  // check secs to string
  auto str_result = mstr::iso8601_time_t(secs_since_epoch);
  std::cout << "Formatted UTC time from secs since epoch: " << str_result << ", control: " << utc_time_str << std::endl;
  auto tt_from_string = mstr::iso8601_to_time_t(utc_time_str);
  std::cout << "time_t from ISO 8601 string: " << tt_from_string << " control: " << secs_since_epoch << std::endl;
  std::cout << "measdir: " << mstr::measdir_time(tt_from_string) << std::endl;
  // checks for the channel
  auto chanx = std::make_shared<channel>("Ex", 128, "2009-08-20T13:22:00", 0);
  std::cout << "channel: " << chanx->get_unix_timestamp() << " control: " << secs_since_epoch << std::endl;
  auto chany = std::make_shared<channel>();
  chany->from_ats("Ey", 128, secs_since_epoch, 0);
  std::cout << "channel2: " << chany->get_unix_timestamp() << " control: " << secs_since_epoch << std::endl;
  std::cout << "both: " << chanx->start_datetime() << " " << chany->start_datetime() << std::endl;

  return 0;
}