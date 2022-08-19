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

#include "xml_from_ats.h"

#include "atsheader_def.h"
#include "atsheader.h"
#include "cal_base.h"
#include "../cal/read_cal/read_cal.h"
#include "bthread.h"
#include "mt_base.h"

#include "pt_cat.h"
#include "pt_tojson.h"
#include "adu06_fname.h"

// run tests
// -outdir /tmp -cat /home/bfr/devel/ats_data/cat_ats_data/NGRI/meas_2019-11-20_06-52-49/*ats /home/bfr/devel/ats_data/cat_ats_data/NGRI/meas_2019-11-22_06-22-30/*ats
// -outdir /tmp -chats /home/bfr/devel/ats_data/zero6/site0199/*ats
// -tojson -clone -outdir /tmp/aa  /survey-master/Eastern_Mining

/*
   string path = "/";

for (const auto & file : directory_iterator(path))
    cout << file.path() << endl;
*/


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
    std::vector<std::shared_ptr<calibration>> calibs;       //!< all JSON style calibrations
    fs::path clone_dir;                                     //!> directory to clone into JSON
    std::multimap<std::string, fs::path> xmls_and_files;    //!< create a multimap which ats files belong to the same XML

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


    if (!clone) {
        l = 1;
        while (argc > 1 && (l < unsigned(argc))) {
            std::string marg(argv[l]);
            if ( (marg.compare(marg.size()-4, 4, ".ats") == 0) || (marg.compare(marg.size()-4, 4, ".ATS") == 0) ) {

                atsheaders.emplace_back(std::make_shared<atsheader>(fs::path(marg)));
            }
            ++l;
        }
    }
    else {
        clone_dir = std::string(argv[argc-1]);
        if (!fs::exists(clone_dir)) {
            std::cerr << "clone directory does not exist! : " << clone_dir << std::endl;
            return EXIT_FAILURE;
        }
        if (clone_dir.empty()) {
            std::cerr << "clone needs a survey directoy as last argument" << std::endl;
            return EXIT_FAILURE;
        }
        clone_dir = fs::canonical(clone_dir);

        if (!fs::is_directory(clone_dir)) {
            std::cerr << "clone needs a survey directoy as last argument" << std::endl;
            return EXIT_FAILURE;
        }

        std::string extension_lower = ".ats";
        std::string extension_upper = ".ATS";
        for (auto const& dir_entry : fs::recursive_directory_iterator(clone_dir))
        {

            // std::cout << dir_entry << '\n';
            if( dir_entry.is_regular_file() && ((dir_entry.path().extension() == extension_lower) || (dir_entry.path().extension() == extension_upper)) ) {
                // file_names.push_back( entry.path().string() ) ;
                atsheaders.emplace_back(std::make_shared<atsheader>(dir_entry.path()));
            }

        }
    }


    if (!atsheaders.size()) {
        std::cout << "no ats files found" << std::endl;
        return EXIT_FAILURE;
    }

    for (const auto& ats : atsheaders ) {
        std::cout <<  ats->path() <<  std::endl;
    }

    if (cat|| chats) {
        // first we order by start time
        std::sort(atsheaders.begin(), atsheaders.end(), compare_ats_start);
        // secondly we sort by channel type
        ats_channel_sort(atsheaders);
    }


    // example functions
    //    std::find_if(atsheaders.begin(), atsheaders.end(), Greater(5, "lsb"));
    //    auto result = *std::find_if(atsheaders.begin()+1, atsheaders.end(), comp_if_equal<double>(atsheaders.front(), "lsbval"));



    // ************************************************************************ C A T *******************************************************************************************

    if (cat) {
        if (outdir.empty()) {
            std::cout << "please supply -outdir name" << std::endl;
            return EXIT_FAILURE;
        }
        if (atsheaders.size() < 2) {
            std::cout << "cat you need two files or more" << std::endl;
            return EXIT_FAILURE;
        }

        if (!std::filesystem::exists(outdir)) {
            std::filesystem::create_directory(outdir);
            if(!std::filesystem::exists(outdir)) {
                std::cout << "can not create outdir" << std::endl;
                return EXIT_FAILURE;
            }
            std::cout << outdir << " created" << std::endl;
        }

        // e.g. a vector of Ex, a vector of Ey ... a vector of Hz
        std::vector<std::vector<std::shared_ptr<atsheader>>> cat_ats;
        do {
            if (atsheaders.size() > 1) {
                // create a new vector<of_not_yet_vector>
                cat_ats.push_back(std::vector<std::shared_ptr<atsheader>>());
                // now the new vector becomes a vector<vector>
                // take the lates created ( back() ) and add an element ( push_back() )
                cat_ats.back().push_back(atsheaders.at(0));
                // and remove the first which we pushed back
                atsheaders.erase(atsheaders.begin());

                for (auto p = atsheaders.begin(); p < atsheaders.end();) {
                    // latest vector (back() ) first element (front() )
                    // as long we find same channel, same sampling frequency and later start time we push back in the same sub-vector
                    if (ats_can_simple_cat(cat_ats.back().front(), *(p))) {
                        cat_ats.back().push_back(*p);
                        p = atsheaders.erase(p);
                    }
                    else {
                        ++p;
                    }
                }
                std::cout << "cat" << std::endl;
                for (const auto& ats : cat_ats.back() ) {
                    std::cout <<  ats->path().filename() <<  " -> "  << ats->header.start <<  std::endl;
                }
            }
        } while (atsheaders.size());
        atsheaders.clear();

        // for cat each vector must have at least two time series
        // reverse iterate check and remove in case

        for (auto it = cat_ats.begin(); it!= cat_ats.end();) {
            if ((*it).size() < 2) it = cat_ats.erase(it);
            else ++it;
        }

        if (!cat_ats.size()) return EXIT_FAILURE;

        try {
            std::vector<std::jthread> threads;
            std::mutex mtx_dir;
            std::mutex mtx_xml;
            std::mutex mtx_xml_files;

            int i = 0;
            // now print out the action
            std::cout << "start cat thread ";
            for (auto& ats : cat_ats) {
                std::cout << " " << i++;
                threads.emplace_back(std::jthread (cat_ats_files, std::ref(ats), std::ref(outdir), std::ref(xmls_and_files), std::ref(xml_files), std::ref(mtx_dir),
                                                  std::ref(mtx_xml), std::ref(mtx_xml_files)));
                //cat_ats_files(ats, outdir, xmls_and_files, xml_files, mtx);
            }
            std::cout << std::endl << "wait please ... " << std::endl;
        }

        catch (const std::string &error) {
            std::cerr << error << std::endl;
            std::cerr << "could not concat ats files" << std::endl;
            return EXIT_FAILURE;
        }
        catch(...) {
            std::cerr << "could not concat ats files" << std::endl;
            return EXIT_FAILURE;

        }

        read_cal rcal;
        for (const auto& xmlfile : xml_files) {
            std::cout << "scanning XML for calibration:" << xmlfile << std::endl;
            auto cals = rcal.read_std_xml(xmlfile);
            calibs.insert(calibs.end(), cals.begin(), cals.end());
        }

        std::cout << "cat done" << std::endl;
        remove_cal_duplicates(calibs);

        // this function will sort the ats channel files in order to get C00 ... C99
        xml_from_ats(xmls_and_files, calibs);

    }

    // ************************************************************************ C H A T S *******************************************************************************************


    if (chats) {
        if (!sizeof(outdir)) {
            std::cout << "please supply -outdir name" << std::endl;
            return EXIT_FAILURE;
        }
        if (!atsheaders.size()) {
            std::cout << "cat you need one file(s) or more" << std::endl;
            return EXIT_FAILURE;
        }

        if (!std::filesystem::exists(outdir)) {
            std::filesystem::create_directory(outdir);
            if(!std::filesystem::exists(outdir)) {
                std::cout << "can not create outdir" << std::endl;
                return EXIT_FAILURE;
            }
            std::cout << outdir << " created" << std::endl;
        }

        for (const auto &atsh : atsheaders) {

            auto adu06 = std::make_shared<adu06_fname>(atsh->path());
            auto atsj = std::make_shared<ats_header_json>( atsh->header,  atsh->path());
            atsj->get_ats_header();
            // the old software does not reliably set the chopper in the ats header - must guess!
            if (atsj->header["sample_rate"] < 513) atsj->header["chopper"] = 1;
            std::cout << adu06->channel_type << " " <<  atsj->header["sample_rate"] << " chopper: " << atsj->header["chopper"] << std::endl;
            std::cout <<  atsj->header["x1"] << " " << atsj->header["y1"] << std::endl;


        }

    }

    // ************************************************************************ T O J S O N || C L O N E *******************************************************************


    if (tojson || clone) {

        if (outdir.empty()) {
            std::cout << "please supply -outdir name" << std::endl;
            return EXIT_FAILURE;
        }
        if (!atsheaders.size()) {
            std::cout << "cat you need one file(s) or more" << std::endl;
            return EXIT_FAILURE;
        }

        if (clone) {
            if (clone_dir.empty()) {
                std::cout << "please supply last argument as directory to clone" << std::endl;
                return EXIT_FAILURE;
            }
            std::cout << clone_dir.parent_path().filename() << std::endl;
           // << fuck das geht nich
            try {
                //std::filesystem::create_directory(outdir);
                outdir /= clone_dir.filename();
                std::filesystem::create_directory(outdir);
                create_survey_dirs(outdir, survey_dirs());
                outdir /= "ts"; // put the data in the time series directory

            }
            catch (std::error_code& ec) {
                std::string err_str = std::string("atstools->") + __func__;
                std::cerr << ec.message();
                std::cerr << err_str << " " << outdir << std::endl;
            }


        }

        // ask HW and split threads
        std::vector<size_t> execs = mk_mini_threads(0, atsheaders.size());

        size_t thread_index = 0;
        std::mutex dirlock;

        //
        // serialized test
                for (auto &atsh : atsheaders) {
                    std::cout << atsh->path() << std::endl;
                    ats2json(atsh, outdir, dirlock, create_measdir, create_sitedir, default_e_sensor, default_h_sensor);
                }
        //

//        for (const auto &ex : execs) {
//            try {
//                std::vector<std::jthread> threads;
//                std::cout << "starting: " << ex << std::endl;
//                for (size_t j = 0; j < ex; ++j) {
//                    threads.emplace_back(std::jthread (ats2json, std::ref(atsheaders.at(thread_index++)), std::ref(outdir),
//                                                      std::ref(dirlock), std::ref(create_measdir), std::ref(create_sitedir),
//                                                      std::ref(default_e_sensor), std::ref(default_h_sensor)) );
//                }
//            }
//            catch (...) {
//                std::cerr << "could not execute all threads" << std::endl;
//                return EXIT_FAILURE;
//            }
//        }


    }


    return EXIT_SUCCESS;
}
