#ifndef PT_TOJSON_H
#define PT_TOJSON_H

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

#include "atsheader.h"
#include "atsheader_def.h"
#include "atss.h"
#include "cal_base.h"
#include "read_cal.h"
#include "survey.h"

void collect_atsheaders(const std::shared_ptr<atsheader> &ats, std::unique_ptr<survey_d> &survey, const int64_t &shift_start_time = 0) {

  ats->read(); // get the binary data from the header; keep file open
  auto atsj = std::make_shared<ats_header_json>(ats->header, ats->path());
  atsj->get_ats_header(); // fill the json
  // atsj->header["sensor_type"].get<std::string>())
  // that seems not to be thread safe - the date int -> string -> int ... may be a problem
  // auto chan = std::make_shared<channel>(atsj->header["channel_type"], atsj->header["sample_rate"], atsj->start_datetime(), 0.0);
  //
  if (shift_start_time) {
    atsj->header["start"] = int64_t(atsj->header["start"]) + shift_start_time;
  }
  auto chan = std::make_shared<channel>();
  chan->from_ats(atsj->header["channel_type"], atsj->header["sample_rate"], atsj->secs_since_1970(), 0.0);
  chan->set_base_file(atsj->header["SystemType"], atsj->header["serial_number"], atsj->header["channel_number"]);
  chan->set_lat_lon_elev(atsj->get_lat(), atsj->get_lon(), atsj->get_elev());
  chan->angle = atsj->pos2angle();
  chan->tilt = atsj->pos2tilt();
  chan->resistance = atsj->header["rho_probe_ohm"]; // contact resistance
  chan->filter = atsj->get_filter("ADB-");
  auto rcal = std::make_shared<read_cal>();
  auto cal = std::make_shared<calibration>();
  cal->set_format(CalibrationType::mtx, false);
  cal->units_amplitude = "mV"; // may be changed later when try to scale in case E is assumed
  // fill with vals rom ats header as far as possible
  cal->sensor = rcal->get_sensor_name(atsj->header["sensor_type"]);
  if (cal->sensor == "")
    cal->sensor = atsj->header["sensor_type"];
  cal->serial = atsj->header["sensor_serial_number"];
  cal->chopper = atsj->get_chopper();

  std::string messages;
  int64_t ssi = atsj->header["channel_number"];
  std::string ssh = std::to_string(ssi);

  std::vector<std::shared_ptr<calibration>> cals;
  // read XML calibration xmlcal
  try {
    cals = rcal->read_std_xml(atsj->xml_path(), messages);
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    std::cerr << "--> continued" << std::endl;
    cals.clear();
  }
  // make sure to have correct sensor names
  for (auto &ncal : cals) {
    ncal->sensor = rcal->get_sensor_name(ncal->sensor);
  }

  chan->tmp_station = ats->site_name();
  chan->tmp_orgin = ats->path();

  double lsb = atsj->header["lsbval"];
  if (atsj->can_and_want_scale()) {
    lsb *= 1000. / atsj->pos2length();
    chan->units = "mV/km";
    chan->tmp_lsb = lsb;
  } else {
    chan->tmp_lsb = lsb;
    chan->units = "mV";
  } // no scaling
  bool has_cal = false;
  for (auto &ncal : cals) {
    if (compare_sensor_and_chopper(cal, ncal)) {
      ncal->old_to_newformat();
      chan->set_cal(ncal);
      has_cal = true;
      auto all_messages = mstr::split(messages, ';');
      for (const auto &str : all_messages) {
        if (mstr::contains(str, cal->sensor, false) && mstr::contains(str, cal->chopper2string(), false) && mstr::contains(str, cal->serial2string(), false) && mstr::contains(str, ssh, false)) {
          std::cout << str << std::endl;
          break;
        }
      }
    }
  }
  if (!has_cal) {
    chan->set_cal(cal);
    auto all_messages = mstr::split(messages, ';');
    for (const auto &str : all_messages) {
      if (mstr::contains(str, cal->sensor, false) && mstr::contains(str, ssh, false)) {
        std::cout << str << std::endl;
        break;
      }
    }
  }
  survey->collect(chan);
}

void fill_survey_tree(const std::unique_ptr<survey_d> &survey, const size_t &index) {

  auto chan = survey->get_channel_from_all(index);
  if (chan->get_atss_filepath().empty())
    return;
  auto ats = std::make_shared<atsheader>(chan->tmp_orgin, false);
  size_t samples = static_cast<uint64_t>(ats->header.samples);
  if (!samples)
    return;
  double lsb = chan->tmp_lsb;
  std::filesystem::path meta_run;
  try {
    chan->write_header();
    meta_run = fdirs::meta_dir(chan->get_filepath_wo_ext().parent_path());
    if (!std::filesystem::exists(meta_run))
      std::filesystem::create_directory(meta_run);

  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    return;
  }

  // now read the data, convert to double and scale E in case
  size_t chunk_size = 524288;
  std::vector<int32_t> ints;
  std::vector<double> dbls;
  if (samples < chunk_size) {
    dbls.resize(samples);
    ints.resize(samples);
  } else {
    dbls.resize(chunk_size);
    ints.resize(chunk_size);
  }

  size_t samples_read = 0;
  bool check = false;

  try {
    do {
      if (!samples_read) {
        std::cout << "reading " << chan->get_atss_filepath() << " LSB : " << lsb << std::endl;
        if (chan->channel_type == "Hz") {
          std::cout << "E channel found" << std::endl;
          check = true;
        }
      }
      dbls.resize(ats->ats_read_ints_doubles(ints));
      samples_read += dbls.size();
      std::transform(ints.begin(), ints.end(), dbls.begin(), [lsb](double v) { return (lsb * v); });
      if (check) {
        check = false;
        for (size_t nx = 0; nx < 5; nx++) {
          std::cout << ints[nx] << " <-> " << dbls[nx] << " "
                    << "lsb:" << lsb << std::endl;
        }
      }

      chan->write_data(dbls);
    } while (dbls.size() && chan->outfile_is_good());
    chan->close_outfile();

    std::cout << chan->filename(".json") << "  " << samples_read << " <-> " << chan->samples() << std::endl;
  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
    return;
  }

  if (std::filesystem::exists(meta_run)) {
    auto atsj = std::make_shared<ats_header_json>(ats->header, ats->path());
    atsj->get_ats_header(); // fill the json
    auto meta = atsj->write_meta(meta_run, chan->filename(".json"));

    if (std::filesystem::exists(atsj->xml_path())) {
      auto new_xml = meta_run /= atsj->xml_path().filename();
      // std::cout << meta << " written" << std::endl;

      std::filesystem::copy_file(atsj->xml_path(), new_xml, std::filesystem::copy_options::skip_existing);
    }
  }
}

#endif // PT_TOJSON_H
