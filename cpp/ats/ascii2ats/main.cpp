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

#include "single_colon_ascii.h"

// using std::filesystem::directory_iterator;
namespace fs = std::filesystem;


#include "atsheader_def.h"
#include "atsheader.h"



// run tests
// -toasccii -outdir /tmp/aa  /survey-master/Northern_Mining
// -outdir /tmp/aa -channel_type Ex
// -channel_type Ex -sample_rate 0.125  -outdir /home/bfr/devel/ats_data/ascii/ascii_out/ /home/bfr/devel/ats_data/ascii/ascii_in/084_V01_C00_R001_TEx_BL_8S.tsdata
/*
   string path = "/";

for (const auto & file : directory_iterator(path))
    cout << file.path() << endl;
*/


int main(int argc, char* argv[])
{


    auto atsh = std::make_shared<atsheader>();
    auto atsj = std::make_shared<ats_header_json>(atsh->header, "");

    bool create_measdir = false;
    fs::path outdir;
    fs::path infile;
    size_t run = 99;

    std::string channel_type = "";
    // load the according template

    // scan the rest
    unsigned l = 1;
    while (argc > 1 && (l < unsigned(argc))) {
        std::string marg(argv[l]);
        if (marg.compare("-channel_type") == 0) {
            channel_type = std::string(argv[++l]);
        }
        ++l;
    }
    atsj->create_default_header(channel_type);

    l = 1;
    while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
        std::string marg(argv[l]);

        if (marg.compare("-x1") == 0) {
            atsj->header["x1"] = atof(argv[++l]);
        }
        if (marg.compare("-x2") == 0) {
            atsj->header["x2"] = atof(argv[++l]);
        }
        if (marg.compare("-y1") == 0) {
            atsj->header["y1"] = atof(argv[++l]);
        }
        if (marg.compare("-y2") == 0) {
            atsj->header["y2"] = atof(argv[++l]);
        }
        if (marg.compare("-z1") == 0) {
            atsj->header["z1"] = atof(argv[++l]);
        }
        if (marg.compare("-z2") == 0) {
            atsj->header["z2"] = atof(argv[++l]);
        }
        if (marg.compare("-sample_rate") == 0) {
            atsj->header["sample_rate"] = atof(argv[++l]);
        }
        if (marg.compare("-start") == 0) {
            atsj->header["start"] = atoll(argv[++l]);
        }
        if (marg.compare("-run") == 0) {
            run = atol(argv[++l]);
        }
        // repeated in order to fetch this argumnent here in the loop
        if (marg.compare("-channel_type") == 0) {
            channel_type = std::string(argv[++l]);
        }






        if (marg.compare("-create_measdir") == 0) {
            create_measdir = true;
        }

        else if (marg.compare("-outdir") == 0) {
            outdir = std::string(argv[++l]);
            if (!fs::exists(outdir)) fs::create_directory(outdir);
            outdir = fs::canonical(outdir);
        }


        else if (marg.compare("-") == 0) {
            std::cerr << "\nunrecognized option " << argv[l] << std::endl;
            return EXIT_FAILURE;
        }




        ++l;
    }
    infile = argv[argc -1];
    if (!fs::exists(infile)) {
        std::cerr << "\nin file does not exist " << infile.string() << std::endl;
        return EXIT_FAILURE;

    }



    atsj->filename = outdir;
    atsj->set_ats_header();


    atsh->header = atsj->atsh;

    //std::cout << atsh->gen_xmlfilename() << "  " << atsh->get_ats_filename(run) << std::endl;
std::cout <<  atsh->get_ats_filename(run) << std::endl;

    atsh->set_new_filename("/tmp/aa/test.ats");
    //atsh->write(true);  // CHANGE






    return EXIT_SUCCESS;
}