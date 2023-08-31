#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <vector>
// using std::filesystem::directory_iterator;
namespace fs = std::filesystem;

#include "../xml/tinyxmlwriter/tinyxmlwriter.h"
#include "atmheader.h"
#include "atsheader.h"
#include "atsheader_def.h"
#include "atsheader_xml.h"

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

  // needs ONE ats file name as parameter for testing
  std::vector<std::shared_ptr<atsheader>> atsheaders;

  unsigned l = 1;

  l = 1;
  while (argc > 1 && (l < unsigned(argc))) {
    std::string marg(argv[l]);
    if ((marg.compare(marg.size() - 4, 4, ".ats") == 0) || (marg.compare(marg.size() - 4, 4, ".ATS") == 0)) {

      atsheaders.emplace_back(std::make_shared<atsheader>(fs::path(marg)));
    }
    ++l;
  }

  if (!atsheaders.size()) {
    std::cout << "no ats files found / selected" << std::endl;
    exit(0);
  }

  for (const auto &ats : atsheaders) {
    std::cout << ats->path() << ", samples: " << ats->header.samples << std::endl;
  }

  try {
    size_t chunk_size = 300000;
    std::vector<int32_t> ints(chunk_size);
    size_t samples_read = 0;
    atsheaders[0]->read();
    auto out = std::make_shared<atsheader>(atsheaders.at(0));
    out->change_dir("/tmp");
    out->write(); // write header and keep file open
    std::cout << out->path() << std::endl;
    auto atm = std::make_shared<atmfile>();
    atm->get_atsfile_name(out->path());

    size_t i = 0, j = 0;

    for (j = 0; j < atsheaders.size(); ++j) {
      if (j) {
        atsheaders[j - 1]->close();
        atsheaders[j]->read();
        ints.resize(chunk_size);
      }
      while (atsheaders[j]->ats_read_ints_doubles(ints)) {
        std::cout << i++ << std::endl;
        // chuncks.push_back(ints);
        out->ats_write_ints_doubles(out->header.lsbval, ints);
        atm->add_unselected(ints.size());
        samples_read += ints.size();
      }
      if (j < atsheaders.size() - 1) {
        if (ats_can_simple_cat(atsheaders.at(j), atsheaders.at(j + 1))) {
          int64_t dt = diff_time(atsheaders.at(j), atsheaders.at(j + 1));
          std::cout << "samples to add: " << dt << " after sample: " << samples_read << " " << out->write_count << std::endl;

          if (dt >= 0) {
            out->ats_zero_ints(dt);
            atm->add_selected(dt);
          }
        }
      }
    }

    out->close();
    atm->close(true);
    out->header.samples = uint32_t(out->write_count);
    std::cout << "total samples: " << out->header.samples << " " << atm->header.samples << std::endl;
    out->re_write();
    // out->write_ascii();

    auto atsj = std::make_shared<ats_header_json>(out->header, out->path());
    atsj->get_ats_header();

    auto tix = std::make_shared<tinyxmlwriter>(true, "/tmp/measdoc.xml");
    tix->push("measurement");
    tix->push("recording");
    recording_ats_sub(tix, atsj);
    tix->push("input");
    tix->push("Hardware", HWNode_ats_str(atsj));
    global_config_ats(tix, atsj, 1);
    Hardware_channel_config_xml(tix, atsj);
    tix->pop("Hardware");
    tix->pop("input");

    tix->push("output");
    tix->push("ProcessingTree", "id", 0);
    tix->push("configuration");
    tix->element("processings", "mt_auto");
    tix->pop("configuration");
    tix->push("output");
    tix->push("ATSWriter");
    tix->push("configuration");
    ATSWriter_channel_xml(tix, atsj);
    tix->pop("configuration");
    ATSWriter_comments_xml(tix, atsj);
    tix->push("output_file");
    tix->element("line_num", "");
    tix->element("site_num", "");
    tix->element("run_num", "");
    tix->element("ats_file_size", "");
    tix->pop("output_file");
    tix->pop("ATSWriter");
    tix->pop("output");
    tix->pop("ProcessingTree");
    tix->pop("output");
    tix->pop("recording");

    std::cout << out->header.comments.XmlHeader << std::endl;
    tix->snatch_cal_from_xml(atsheaders.at(0)->path().parent_path(), mstr::clean_b_str(out->header.comments.XmlHeader));

    tix->pop("measurement");
    tix->write_file();

    std::cout << "done" << std::endl;

    
    std::cout << std::setw(2) << atsj->header << std::endl;

  } catch (const std::string &error) {

    std::cerr << error << std::endl;
  }

  std::cout << "run finished - w/o exception" << std::endl;
  return 0;
}
