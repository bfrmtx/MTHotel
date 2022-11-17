#include <iostream>
#include <vector>
#include <list>
#include <memory>
#include <filesystem>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <shared_mutex>

// using std::filesystem::directory_iterator;
namespace fs = std::filesystem;


#include "atsheader_def.h"
#include "atsheader.h"

#include "bthread.h"
#include "mt_base.h"


// run tests
// -outdir /tmp -cat /home/bfr/devel/ats_data/cat_ats_data/NGRI/meas_2019-11-20_06-52-49/*ats /home/bfr/devel/ats_data/cat_ats_data/NGRI/meas_2019-11-22_06-22-30/*ats
// -outdir /tmp -chats /home/bfr/devel/ats_data/zero6/site0199/*ats
// -tojson -clone -outdir /tmp/aa  /survey-master/Eastern_Mining
// -tojson -clone -outdir /tmp/aa  /survey-master/Northern_Mining


/*
   string path = "/";

for (const auto & file : directory_iterator(path))
    cout << file.path() << endl;
*/
#include "sqlite_handler.h"

// SELECT printf("%.12E", value) AS value FROM mtx_8x;
int main(int argc, char* argv[])
{



    bool cat = false;                       //!< concatunate ats files, try read xml from atsheader & calibration from XML
    bool chats = false;                     //!< convert ADU-06 files to ADU-08e files
    bool tojson = false;                    //!< convert to JSON and binary - the new format
    bool create_measdir = false;
    bool create_sitedir = false;
    bool clone = false;
    fs::path outdir;

    // to be implemented
    std::string default_e_sensor;           //!< E Sensor is not detected by default use a string like EFP-06
    std::string default_h_sensor;           //!< H Sensor is detected by default; this is an rescue option; use a string like MFS-06


    std::vector<std::shared_ptr<atsheader>> atsheaders;     //!< all ats files or ats headers

    std::vector<fs::path> indirs;
    std::vector<fs::path> xml_files;                        //!< all xml files - either direct read or from ats/atss generated
    fs::path clone_dir;                                     //!> directory to clone into JSON




    std::filesystem::path sqlfile("/home/bfr/data.sql3");
    auto sql_data = std::make_unique<sqlite_handler>();
    std::vector<double> coeff;
    std::string sql_query = "SELECT value FROM mtx_9x";
    try {
        coeff = sql_data->sqlite_vector_double(sqlfile, sql_query);
    }
    catch (const std::string &error) {
        std::cerr << error << std::endl;
        sql_data.reset();
    }

    for (const auto &value : coeff) {
        std::cout << value << std::endl;
    }
    std::cout  << std::endl;

    // read orgin
    // estimate the filter
    // cascade down from origin



    unsigned l = 1;
    while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
        std::string marg(argv[l]);
        if (marg.compare("-cat") == 0) {
            cat = true;
        }
        if (marg.compare("-chats") == 0) {
            chats = true;
        }
        if (marg.compare("-create_measdir") == 0) {
            create_measdir = true;
        }
        if (marg.compare("-create_sitedir") == 0) {
            create_sitedir = true;
        }
        if (marg.compare("-tojson") == 0) {
            tojson = true;
        }
        if (marg.compare("-clone") == 0) {
            clone = true;
            create_sitedir = true;
            create_measdir = true;
        }

        else if (marg.compare("-outdir") == 0) {
            outdir = std::string(argv[++l]);
            if (!fs::exists(outdir)) fs::create_directory(outdir);
            outdir = fs::canonical(outdir);
        }

        else if (marg.compare("-default_e_sensor") == 0) {
            default_e_sensor = std::string(argv[++l]);
        }
        else if (marg.compare("-default_h_sensor") == 0) {
            default_h_sensor = std::string(argv[++l]);
        }


        else if (marg.compare("-") == 0) {
            std::cerr << "\nunrecognized option " << argv[l] << std::endl;
            return EXIT_FAILURE;
        }
        ++l;
    }




    return EXIT_SUCCESS;
}
