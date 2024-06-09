#ifndef ATSS_TO_ATS_H
#define ATSS_TO_ATS_H

#include "atsheader.h"
#include "atsheader_def.h"
// new format
#include "atss.h"
#include "survey.h"

namespace fs = std::filesystem;

void copy_json_to_ats(const std::shared_ptr<channel> &chan, std::shared_ptr<ats_header_json> &atsj) {
  atsj->create_default_header(chan->get_channel_type());
  atsj->header["sensor_type"] = chan->cal->sensor;
  atsj->header["sensor_serial_number"] = chan->cal->serial;
  atsj->header["sample_rate"] = chan->get_sample_rate();
  atsj->header["orig_sample_rate"] = chan->get_sample_rate();
  atsj->header["start"] = chan->get_unix_timestamp();
  if (chan->channel_type.starts_with("E")) {
    atsj->dip2pos(1000.0, chan->angle); // atss E is always scaled to mV/km
  }
  atsj->header["channel_number"] = chan->get_channel_no();
  atsj->header["rho_probe_ohm"] = chan->resistance;
  atsj->header["SystemType"] = chan->get_system();
  atsj->header["serial_number"] = chan->get_serial();
  atsj->header["chopper"] = chan->cal->get_int_chopper();
  atsj->header["iElev_cm"] = int32_t(chan->elevation * 100.0);
  atsj->header["iLat_ms"] = int32_t(chan->latitude * 3600000.0);
  atsj->header["iLong_ms"] = int32_t(chan->longitude * 3600000.0);
}

class atss_to_ats {

public:
  atss_to_ats(std::shared_ptr<survey_d> &survey, fs::path &outdir) : survey(survey), outdir(outdir) {
    if (!fs::exists(outdir))
      fs::create_directory(outdir);
    outdir = fs::canonical(outdir);
    if (!survey) {
      std::cerr << "No survey object passed to atss_to_ats" << std::endl;
      exit(1);
    }
    if (!fs::exists(outdir)) {
      std::cerr << "Output directory does not exist" << std::endl;
      exit(1);
    }
  }

  ~atss_to_ats() = default;

  void convert_run(const std::string &station, const size_t &run_number) {
    auto ptr_station = this->survey->get_station(station);
    auto run = ptr_station->get_run(run_number);
    auto channels = run->get_channels();
    for (const auto &chan : channels) {
      auto atsh = std::make_shared<atsheader>();
      auto atsj = std::make_shared<ats_header_json>(atsh->header, "");
      copy_json_to_ats(chan, atsj);
      atsj->set_ats_header();    // set the ats header in the pointer
      atsh->header = atsj->atsh; // do I need this?
      // read the data into the channel
      // chan->read_data(true);
      auto data = chan->read_all_at_once();
      // both time series are in mV

      //
      atsh->header.samples = data.size();
      auto ats_outdir = this->outdir / station / atsj->measdir();
      if (!fs::exists(ats_outdir))
        fs::create_directories(ats_outdir);
      ats_outdir = fs::canonical(ats_outdir);
      atsh->set_new_filename(ats_outdir / atsh->get_ats_filename(run_number));
      atsh->calc_lsb_from_dbl_vec_mV(data);
      atsh->write(false);
      atsh->ats_write_ints_doubles(atsh->header.lsbval, data, true);
    }
  }

private:
  std::shared_ptr<survey_d> survey; // the survey
  fs::path outdir;                  // the output directory
  std::vector<std::shared_ptr<atsheader>> atshs;
  std::vector<std::shared_ptr<ats_header_json>> atsjs; // the json will push into ats
};

#endif // ATSS_TO_ATS_H
