#ifndef ATSHEADER_XML
#define ATSHEADER_XML

#include "atsheader.h"
#include "atsheader_def.h"
#include "tinyxmlwriter.h"

// code snippets for generating xml files from ats

void recording_ats_sub(std::shared_ptr<tinyxmlwriter> &tix, const std::shared_ptr<ats_header_json> &atsh) {
  tix->element("start_time", atsh->start_time());
  tix->element("start_date", atsh->start_date());
  tix->element("stop_time", atsh->stop_time());
  tix->element("stop_date", atsh->stop_date());
  tix->element("target_directory", "/mtdata/data");
  tix->element("CyclicEvent", "0");
  tix->element("Cycle", "60");
  tix->element("Granularity", "seconds");
}

void global_config_ats(std::shared_ptr<tinyxmlwriter> &tix, const std::shared_ptr<ats_header_json> &atsh, const uint32_t nchannels) {

  tix->push("global_config");

  tix->element("meas_channels", nchannels);
  tix->element("sample_freq", atsh->header["sample_rate"]);
  tix->element("buffer", "1024");
  tix->element("start_stop_mode", " ");
  tix->element("calon", "0");
  tix->element("atton", "0");
  tix->element("calref", "0");
  tix->element("calint", "0");
  tix->element("calfreq", "0");
  tix->element("short_circuit", "0");
  tix->element("decimation", "0");
  tix->element("flush_fill", "64");
  tix->element("ovl_fill", "1024");
  tix->element("start_stop_fill", "512");

  tix->pop("global_config");
}

std::string HWNode_ats_str(const std::shared_ptr<ats_header_json> &atsh) {
  std::stringstream ss;
  // the operator << of json contains "" so : << name is WITH QUOTES "ADU-08e" and not ADU-08e
  ss << "Type=\"" << atsh->header["TypeNo"] << "\" GMS=\"" << atsh->header["GMSno"] << "\" Name=" << atsh->header["SystemType"];
  return ss.str();
}

void Hardware_channel_config_xml(std::shared_ptr<tinyxmlwriter> &tix, const std::shared_ptr<ats_header_json> &atsh) {

  tix->push("channel", "id", atsh->header["channel_number"]);

  tix->element("gain_stage1", atsh->header["gain_stage1"]);
  tix->element("gain_stage2", atsh->header["gain_stage2"]);
  tix->element("ext_gain", atsh->header["external_gain"]);
  // @todo
  // tix->element("filter_type", atsh->header["fil);
  // @todo hchopper vs echopper never used ?
  tix->element("echopper", atsh->header["chopper"]);
  tix->element("hchopper", atsh->header["chopper"]);
  tix->element("dac_val", atsh->header["DC_offset_voltage_mV"]);
  tix->element("dac_on", atsh->header["DCOffsetCorrOn"]);
  // ats does not know which part of the connector board was used
  tix->element("input", 0);
  tix->element("input_divider", atsh->header["InputDivOn"]);

  tix->pop("channel");
}

void ATSWriter_channel_xml(std::shared_ptr<tinyxmlwriter> &tix, const std::shared_ptr<ats_header_json> &atsh) {

  tix->push("channel", "id", static_cast<int>(atsh->header["channel_number"]));

  tix->element("start_time", atsh->start_time());
  tix->element("start_date", atsh->start_date());
  tix->element("sample_freq", atsh->header["sample_rate"]);
  tix->element("num_samples", atsh->header["samples"]);
  tix->element("ats_data_file", atsh->header["ats_data_file"].get<std::string>());
  tix->element("ts_lsb", atsh->header["lsbval"]);
  tix->element("channel_type", atsh->header["channel_type"].get<std::string>());
  tix->element("sensor_sernum", atsh->header["sensor_serial_number"]);
  tix->element("sensor_type", atsh->header["sensor_type"].get<std::string>());
  tix->element("sensor_cal_file", atsh->header["sensor_cal_filename"].get<std::string>());
  tix->element("pos_x1", atsh->header["x1"]);
  tix->element("pos_y1", atsh->header["y1"]);
  tix->element("pos_z1", atsh->header["z1"]);
  tix->element("pos_x2", atsh->header["x2"]);
  tix->element("pos_y2", atsh->header["y2"]);
  tix->element("pos_z2", atsh->header["z2"]);

  tix->pop("channel");
}

void ATSWriter_comments_xml(std::shared_ptr<tinyxmlwriter> &tix, const std::shared_ptr<ats_header_json> &atsh) {

  tix->push("comments");

  tix->element("site_name", atsh->header["SiteName"].get<std::string>());
  tix->element("site_name_rr", atsh->header["SiteNameRR"].get<std::string>());
  tix->element("site_name_emap", atsh->header["SiteNameEMAP"].get<std::string>());
  tix->element("client", atsh->header["Client"].get<std::string>());
  tix->element("contractor", atsh->header["Contractor"].get<std::string>());
  tix->element("area", atsh->header["Area"].get<std::string>());
  tix->element("survey_id", atsh->header["SurveyID"].get<std::string>());
  tix->element("operator", atsh->header["Operator"].get<std::string>());
  tix->element("weather", "");
  tix->element("general_comments", atsh->header["Comments"].get<std::string>());

  tix->pop("comments");
}

#endif // ATSHEADER_XML
