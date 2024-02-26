#include <cfloat>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <vector>
// using std::filesystem::directory_iterator;
namespace fs = std::filesystem;

#include "../xml/tinyxml2/tinyxml2.h"
namespace xml = tinyxml2;

#include "base_xml.h"
#include "cal_base.h"

/*
   string path = "/";

    for (const auto & file : directory_iterator(path))
        cout << file.path() << endl;
 */

int main(int argc, char *argv[]) {

  bool cat = false;  //!< concatunate ats files
  int run = -1;      //!< run number, greater equal 0
  double lsbval = 0; //!< lsb
  fs::path outdir;
  //   USERPROFILE=C:\Users\bfr
  //   HOMEDRIVE=C:
  //   HOMEPATH=\Users\bfr
  fs::path home_dir(getenv("HOME"));
  // pPath = getenv ("userdir");
  std::cout << "home is: " << home_dir << std::endl;
  std::string xmltestfile = "devel/ats_data/nm_tiny/meas_2009-08-20_13-23-36/084_2009-08-20_13-23-36_2009-08-21_06-58-32_R001_8S.xml";
  // std::string xmltestfile = "/nfs-server/ptgfz/2023-11-28/1064-257-1065/meas_2023-11-28_07-22-11/183_2023-11-28_07-22-11_2023-11-28_07-25-41_R000_65536H.xml";
  //  std::string xmltestfile = "devel/ats_data/cat_ats_data/NGRI/meas_2019-11-20_06-52-49/295_2019-11-20_06-52-49_2019-11-27_07-22-49_R000_4H.xml";
  fs::path xmlf;
  xmlf = home_dir;
  xmlf /= xmltestfile;
  std::cout << "using " << xmlf << std::endl;
  // needs ONE ats file name as parameter for testing
  // std::vector<std::shared_ptr<atsheader>> atsheaders;

  auto tir = std::make_shared<xml::XMLDocument>();

  std::vector<calibration> cal_entries;
  try {

    bool loaded = tir->LoadFile(xmlf.string().c_str());
    if (loaded) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << ":: error loading XML file " << xmlf;
      throw std::runtime_error(err_str.str());
    }
    auto proot = tir->RootElement(); // that is the envelope, mostly "measurement"
    if (proot == nullptr) {
      std::ostringstream err_str(__func__, std::ios_base::ate);
      err_str << "::Root Element XML_ERROR_FILE_READ_ERROR " << xmlf;
      throw std::runtime_error(err_str.str());
    }

    auto pscal_sens = open_node(proot, "calibration_sensors");
    auto pchan = open_node(pscal_sens, "channel");

    while (pchan) {
      int id = -1;
      int old_id = id;
      pchan->QueryIntAttribute("id", &id);
      if (old_id != id) {
        std::cout << "sensor for channel: " << id << std::endl;
        old_id = id;
        auto pca = open_node(pchan, "calibration");
        // check status for _closingType in tinyxml2.h
        if (pca->ClosingType() == xml::XMLElement::OPEN) {

          auto pci = open_node(pca, "calibrated_item");
          std::string sensor(xml_svalue(pci, "ci"));
          std::cout << sensor << " detected" << std::endl;
          int64_t serial = xml_ivalue(pci, "ci_serial_number");
          std::string cal_date(xml_svalue(pci, "ci_date"));
          std::string cal_time(xml_svalue(pci, "ci_time"));
          std::vector<double> f_on, f_off, a_on, a_off, p_on, p_off;
          auto f_unit = std::make_unique<std::string>();
          auto a_unit = std::make_unique<std::string>();
          auto p_unit = std::make_unique<std::string>();

          // cd will be NULL when there is no cal data, like e.g. for E
          auto cd = open_node(pca, "caldata", true);
          while (cd) {
            if (cd != nullptr) {
              std::string strchp("ukn");
              const char *cchopper = cd->Attribute("chopper");
              if (cchopper != nullptr) {
                strchp = std::string(cchopper);
              }
              double f = xml_dvalue(cd, "c1", f_unit.get(), "unit");
              double a = xml_dvalue(cd, "c2", a_unit.get(), "unit");
              double p = xml_dvalue(cd, "c3", p_unit.get(), "unit");
              if ((f != DBL_MAX && (a != DBL_MAX) && (p != DBL_MAX))) {
                if (strchp == "on") {
                  f_on.push_back(f);
                  a_on.push_back(a);
                  p_on.push_back(p);
                } else {
                  f_off.push_back(f);
                  a_off.push_back(a);
                  p_off.push_back(p);
                }
              }
            }
            cd = cd->NextSiblingElement("caldata");
          }
          if (f_on.size()) {
            std::cout << "on  size " << f_on.size() << std::endl;
            cal_entries.emplace_back(calibration());
            if ((*a_unit.get() == "V/(nT*Hz)") && (*f_unit.get() == "Hz") && (*p_unit.get() == "deg")) {
              cal_entries.back().set_format(CalibrationType::mtx_old);
              cal_entries.back().chopper = ChopperStatus::on;
              cal_entries.back().f = f_on;
              cal_entries.back().a = a_on;
              cal_entries.back().p = p_on;
              cal_entries.back().sensor = sensor;
              if (serial != INT64_MAX)
                cal_entries.back().serial = serial;
              if (cal_date.size())
                cal_entries.back().datetime = cal_date;
              if (cal_time.size())
                cal_entries.back().datetime += "T" + cal_time;
              else
                cal_entries.back().datetime += "T00:00:00";
              // cal_entries.back().write_file("/tmp");
            }
          }
          if (f_off.size()) {
            std::cout << "off size " << f_off.size() << std::endl;
            if ((*a_unit.get() == "V/(nT*Hz)") && (*f_unit.get() == "Hz") && (*p_unit.get() == "deg")) {
              cal_entries.emplace_back(calibration());
              cal_entries.back().set_format(CalibrationType::mtx_old);
              cal_entries.back().chopper = ChopperStatus::off;
              cal_entries.back().f = f_off;
              cal_entries.back().a = a_off;
              cal_entries.back().p = p_off;
              cal_entries.back().sensor = sensor;
              if (serial != INT64_MAX)
                cal_entries.back().serial = serial;
              if (cal_date.size())
                cal_entries.back().datetime = cal_date;
              if (cal_time.size())
                cal_entries.back().datetime += "T" + cal_time;
              else
                cal_entries.back().datetime += "T00:00:00";

              // cal_entries.back().write_file("/tmp");
            }
          }
          // likely electrodes or old sensors
          if (!f_on.size() && !f_off.size()) {
            cal_entries.emplace_back(calibration());
            cal_entries.back().sensor = sensor;
            if (serial != INT64_MAX)
              cal_entries.back().serial = serial;
            if (cal_date.size())
              cal_entries.back().datetime = cal_date;
            if (cal_time.size())
              cal_entries.back().datetime += "T" + cal_time;
            else
              cal_entries.back().datetime += "T00:00:00";
            cal_entries.back().write_file("/tmp");
          }
        }
      }
      pchan = pchan->NextSiblingElement("channel");
    }

  } catch (const std::runtime_error &error) {
    std::cerr << error.what() << std::endl;
  }

  for (auto &cal : cal_entries) {
    std::cout << cal.sensor << " : " << cal.serial << " " << cal.chopper2string() << " (" << cal.f.size() << ")" << std::endl;
    cal.old_to_newformat();
  }

  return 0;
}
