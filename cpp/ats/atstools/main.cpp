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
#include "bthread.h"
#include <optional>

// using std::filesystem::directory_iterator;
namespace fs = std::filesystem;

#include "xml_from_ats.h"

#include "atsheader_def.h"
#include "atsheader.h"
#include "cal_base.h"
#include "../cal/read_cal/read_cal.h"
#include "bthread.h"

#include "pt_cat.h"
#include "pt_tojson.h"
#include "adu06_fname.h"
#include "chats.h"

#include "BS_thread_pool.h"
#include "whereami.h"

// run tests
// -outdir /tmp -cat /home/bfr/devel/ats_data/cat_ats_data/NGRI/meas_2019-11-20_06-52-49/*ats /home/bfr/devel/ats_data/cat_ats_data/NGRI/meas_2019-11-22_06-22-30/*ats
// -outdir /tmp -chats /home/bfr/devel/ats_data/zero6/site0199/*ats
// -tojson -clone -outdir /tmp/aa  /survey-master/Eastern_Mining
// -tojson -clone -outdir /tmp/aa  /survey/Northern_Mining
// -chats -outdir /tmp/aa/06 -json_caldir /tmp  /home/bfr/devel/ats_data/adu06/
// -tojson -clone -outdir /tmp/bb -json_caldir /tmp /home/bfr/devel/ats_data/adu06/
// -split_channels -outdir /home/bfr/tmp /home/bfr/tmp/meas_2023-07-14_09-05-03/*ats
//  splits all to s1 and s2 in /home/bfr/tmp
/*
-split_channels  -outdir /home/bfr/tmp
/home/bfr/tmp/meas_2023-07-14_09-05-03/053_V01_C00_R000_TEx_BH_65536H.ats
/home/bfr/tmp/meas_2023-07-14_09-05-03/053_V01_C01_R000_TEy_BH_65536H.ats
/home/bfr/tmp/meas_2023-07-14_09-05-03/053_V01_C02_R000_THx_BH_65536H.ats
/home/bfr/tmp/meas_2023-07-14_09-05-03/053_V01_C03_R000_THy_BH_65536H.ats
/home/bfr/tmp/meas_2023-07-14_09-05-03/053_V01_C04_R000_THz_BH_65536H.ats
/home/bfr/tmp/meas_2023-07-14_09-05-03/053_V01_C07_R000_THx_BH_65536H.ats
/home/bfr/tmp/meas_2023-07-14_09-05-03/053_V01_C08_R000_THy_BH_65536H.ats
/home/bfr/tmp/meas_2023-07-14_09-05-03/053_V01_C09_R000_THz_BH_65536H.ats

 */

/*
   string path = "/";

for (const auto & file : directory_iterator(path))
    cout << file.path() << endl;
*/



int main(int argc, char* argv[])
{

    auto exec_path = get_exec_dir();

    std::cout << exec_path << std::endl;


    bool cat = false;                       //!< concatunate ats files, try read xml from atsheader & calibration from XML
    bool chats = false;                     //!< convert ADU-06 files to ADU-08e files
    bool tojson = false;                    //!< convert to JSON and binary - the new format
    bool create_old_tree = false;           //!< create an old 07/08 survey tree to enable conversion manually
    bool create_tree = false;               //!< create a survey empty tree
    bool split_channels = false;            //!< split a a multichannel recoring into 5 channels
    ChopperStatus chopper = ChopperStatus::off;
    bool change_chopper = true;
    size_t adu06_shift_samples_hf = 0;
    size_t adu06_shift_samples_lf = 0;
    int64_t shift_start_time = 0;

    bool clone = false;
    fs::path outdir;

    // to be implemented
    std::string default_e_sensor;           //!< E Sensor is not detected by default use a string like EFP-06
    std::string default_h_sensor;           //!< H Sensor is detected by default; this is an rescue option; use a string like MFS-06


    std::vector<std::shared_ptr<atsheader>> atsheaders;     //!< all ats files or ats headers

    std::vector<fs::path> indirs;
    std::vector<fs::path> xml_files;                        //!< all xml files - either direct read or from ats/atss generated
    std::vector<std::shared_ptr<calibration>> calibs;       //!< all JSON style calibrations
    fs::path json_caldir;                                   //!< find directory with JSON cal files
    fs::path clone_dir;                                     //!> directory to clone into JSON
    std::multimap<std::string, fs::path> xmls_and_files;    //!< create a multimap which ats files belong to the same XML

    unsigned l = 1;
    while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
        std::string marg(argv[l]);

        if (marg.compare("-cat") == 0) {
            cat = true;
        }
        if (marg.compare("-split_channels") == 0) {
            split_channels = true;
        }
        if (marg.compare("-chats") == 0) {
            chats = true;
            clone = true;
        }

        if (marg.compare("-json_caldir") == 0) {
            json_caldir = std::string(argv[++l]);
            if (!fs::is_directory(json_caldir)) {
                std::cerr << "-json_caldir needs a directoy with JSON cal files inside" << std::endl;
                return EXIT_FAILURE;
            }
            json_caldir = fs::canonical(json_caldir);
        }

        if (marg.compare("-chats-chopper") == 0) {
            auto str_chopper = std::string(argv[++l]);
            if (mstr::contains(str_chopper, "on", false)) chopper = ChopperStatus::on;
            if (mstr::contains(str_chopper, "off", false)) chopper = ChopperStatus::off;
            change_chopper = false;
        }

        if (marg.compare("-chats_shift_samples_hf") == 0) {
            adu06_shift_samples_hf = atoi(argv[++l]);
        }

        if (marg.compare("-chats_shift_samples_lf") == 0) {
            adu06_shift_samples_lf = atoi(argv[++l]);
        }

        if (marg.compare("-shift_start_time") == 0) {
            shift_start_time = atoi(argv[++l]);
        }
        if (marg.compare("-create_old_tree") == 0) {
            create_old_tree = true;
        }

        if (marg.compare("-create_tree") == 0) {
            create_tree = true;
        }

        if (marg.compare("-tojson") == 0) {
            tojson = true;
        }
        if (marg.compare("-clone") == 0) {
            clone = true;
        }

        if (marg.compare("-outdir") == 0) {
            outdir = std::string(argv[++l]);
            try {
                if (!fs::exists(outdir)) fs::create_directories(outdir);
            }
            catch(...) {
                std::cerr << "could not create outdir " << argv[l-1] << std::endl;
                return EXIT_FAILURE;
            }
            outdir = fs::canonical(outdir);
        }

        if (marg.compare("-default_e_sensor") == 0) {
            default_e_sensor = std::string(argv[++l]);
        }

        if (marg.compare("-default_h_sensor") == 0) {
            default_h_sensor = std::string(argv[++l]);
        }
        // LF board 31 samples cut, no date shift - old source code 30
        // HF board 32 - old source 32
        if (marg.compare("-h") == 0 || marg.compare("-help") == 0 || marg.compare("--help")  == 0) {
            std::cout << " " << std::endl;
            std::cout << "-outdir /tmp/mysurvey " << std::endl;
            std::cout << "  location where the new survey tree is created " << std::endl;
            std::cout << "  example: " << std::endl;
            std::cout << "  -tojson -clone -outdir /tmp/Northern_Mining  /survey-master/Northern_Mining" << std::endl;
            std::cout << "-default_h_sensor MFS-06 " << std::endl;
            std::cout << "  sets MFS-06 for all H channels as sensor type; do this in case a sensor could not be found" << std::endl;
            std::cout << "-default_e_sensor EFP-06 [Bufferd St.Rod] " << std::endl;
            std::cout << "  sets EFP-06 for all E channels as sensor type; do this in case a sensor could not be found" << std::endl;
            std::cout << "-chats_shift_samples_hf 32 " << std::endl;
            std::cout << "  removes 32 samples at the beginning WITHOUT shifting start time: my be used for old ADU-06 data, HF board" << std::endl;
            std::cout << "-chats_shift_samples_lf 30 [31] " << std::endl;
            std::cout << "  removes 30 (or 31) samples at the beginning WITHOUT shifting start time: my be used for old ADU-06 data, LF board" << std::endl;
            std::cout << "  ATTENTION: do NOT use for down filtered data!, It is a RUNTIME correction for old boards" << std::endl;
            std::cout << "-shift_start_time -1 [3599, 3600, -3600, ...] " << std::endl;
            std::cout << "  changes the start time by -1 seconds; old ADU-06 data can be affected by untracked leap seconds, summer/winter time " << std::endl;

            std::cout << "-create_old_tree survey_dir [site_1 site_2, ...] like: iron_mountain L1_S3 L2_S24" << std::endl;
            std::cout << "  create an old 07/08 survey tree; you may need it for conversion" << std::endl;

            std::cout << "-create_tree survey_dir [station_1 station_2, ...]: like iron_mountain L1_S3 L2_S24" << std::endl;
            std::cout << "  create survey tree" << std::endl;

            std::cout << " " << std::endl;
            std::cout << " " << std::endl;

        }

        if (marg.compare("-") == 0) {
            std::cerr << "\nunrecognized option " << argv[l] << std::endl;
            return EXIT_FAILURE;
        }
        ++l;
    }

    if (create_old_tree || create_tree) {

        std::unique_ptr<survey_d> survey;
        std::filesystem::path survey_dir;
        std::vector<std::string> stations;

        while ( (l < unsigned(argc))) {
            std::string marg(argv[l++]);
            if (survey_dir.empty()) {
                survey_dir = std::filesystem::path(marg);
            }
            else {
                stations.emplace_back(marg);
            }
        }

        if (create_old_tree) {
            // do not use a canonical path for recursive creation
            create_survey_dirs(survey_dir, survey_dirs_old(), stations);

        }
        else {
            try {
                survey = std::make_unique<survey_d>(survey_dir, false);
                std::filesystem::path spath;
                for (const auto &station : stations) {
                    spath = survey->create_station(station);
                }
                if (!spath.empty()) {
                    std::cout << "directories created in " << survey_dir << std::endl;
                }
            }
            catch (const std::string &error ) {
                std::cerr << error <<std::endl;

            }
            catch (...) {
                std::cerr << "could not execute create survey" << std::endl;
                return EXIT_FAILURE;
            }

            // if you would like to work the survey - delete pointer, we are in creation mode
            // and use  survey = std::make_unique<survey_d>(survey_dir);

        }

        return EXIT_SUCCESS;

    }

    auto pool = std::make_shared<BS::thread_pool>();


    if (!clone) {
        l = 1;
        while (argc > 1 && (l < unsigned(argc))) {
            std::string marg(argv[l]);
            if ( mstr::ends_with(marg, ".ats") || mstr::ends_with(marg, ".ATS")) {
                if ( (marg.compare(marg.size()-4, 4, ".ats") == 0) || (marg.compare(marg.size()-4, 4, ".ATS") == 0) ) {
                    atsheaders.emplace_back(std::make_shared<atsheader>(fs::path(marg)));
                }
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

    if (cat || chats) {
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
        std::string messages;

        for (const auto& xmlfile : xml_files) {
            std::cout << "scanning XML for calibration:" << xmlfile << std::endl;
            auto cals = rcal.read_std_xml(xmlfile, messages);
            calibs.insert(calibs.end(), cals.begin(), cals.end());
        }
        // correct sensor name in case from MFS06e to MFS-06e
        for (auto &cal : calibs) {
            cal->sensor = rcal.get_sensor_name(cal->sensor);
        }

        std::cout << "cat done" << std::endl;
        remove_cal_duplicates(calibs);

        // this function will sort the ats channel files in order to get C00 ... C99
        xml_from_ats(exec_path, xmls_and_files, calibs);
        return EXIT_SUCCESS;

    }

    // ************************************************************************ S P L I T *******************************************************************************************


    if (split_channels && std::filesystem::exists(outdir)) {
        std::vector<std::shared_ptr<atsheader>> split_ats_1, split_ats_2;
        auto s1dir = outdir / "s1";  // 0, 1, 2, 3, 4
        auto s2dir = outdir / "s2";  // 0, 1, 7, 8, 9
        fs::path xmlfile;
        size_t i = 0;
        for (auto &atsh : atsheaders) {
            auto atsj = std::make_shared<ats_header_json>( atsh->header,  atsh->path());
            atsj->get_ats_header();
            if (!i) {
                xmlfile =  atsh->path().parent_path() / fs::path(atsj->header["XmlHeader"].get<std::string>());
                s1dir /= atsj->measdir();
                s2dir /= atsj->measdir();
            }
            ++i;
            if (atsj->header["channel_number"] < 2 ) {
                split_ats_1.push_back(atsh);
                split_ats_2.push_back(atsh);
            }
            else if (atsj->header["channel_number"] < 5) {
                split_ats_1.push_back(atsh);
            }
            else {
                split_ats_2.push_back(atsh);
            }

            try {

                if (!fs::exists(s1dir)) fs::create_directories(s1dir);
                if (!fs::exists(s2dir)) fs::create_directories(s2dir);

            }
            catch (...) {
                std::cerr << "could not create outdir s1 or s2 " << std::endl;
                return EXIT_FAILURE;
            }


        }
        std::cout << "splitting" << std::endl;

        for (auto &atsh : split_ats_1) {
            std::filesystem::copy(atsh->path(), s1dir, fs::copy_options::update_existing);
        }
        for (auto &atsh : split_ats_2) {
            std::filesystem::copy(atsh->path(), s2dir, fs::copy_options::update_existing);
        }

        try {
            read_cal rcal;

            std::string messages;
            std::cout << "scanning XML for calibration:" << xmlfile << std::endl;
            auto cals = rcal.read_std_xml(xmlfile, messages);
            calibs.insert(calibs.end(), cals.begin(), cals.end());

            // correct sensor name in case from MFS06e to MFS-06e
            for (auto &cal : calibs) {
                cal->sensor = rcal.get_sensor_name(cal->sensor);
            }
            remove_cal_duplicates(calibs);


            //xml_from_ats(exec_path, xmls_and_files, calibs);

        }
        catch (const std::string &error ) {
            std::cerr << error <<std::endl;
            return EXIT_FAILURE;
        }
        catch (...) {

        }

        // get all atsfiles from s1
        std::cout << "scanning XML for calibration:" << xmlfile << std::endl;
        try {
            xmls_and_files.clear();
            for (const auto & file: fs::directory_iterator(s1dir)) {
                std::cout << file.path() << std::endl;
                auto atsh = std::make_shared<atsheader>(file.path());
                xmls_and_files.emplace(atsh->gen_xmlfilename(), atsh->path());
            }
            xml_from_ats(exec_path, xmls_and_files, calibs);

            xmls_and_files.clear();
            for (const auto & file: fs::directory_iterator(s2dir)) {
                std::cout << file.path() << std::endl;
                int i = 0;
                auto atsh = std::make_shared<atsheader>(file.path());
                atsh->read();
                std::string ren_in(file.path().filename());
                std::string ren_out;
                if (mstr::contains(ren_in, "_TEx_")) {
                    ren_out = mstr::string_replace(ren_in, "_C05_", "_C00_");
                    i = 0;
                }
                if (mstr::contains(ren_in, "_TEy_")) {
                    ren_out = mstr::string_replace(ren_in, "_C06_", "_C01_");
                    i = 1;
                    }
                if (mstr::contains(ren_in, "_THx_")) {
                    i = 2;
                    ren_out = mstr::string_replace(ren_in, "_C07_", "_C02_");
                }
                if (mstr::contains(ren_in, "_THy_")) {
                    i = 3;
                    ren_out = mstr::string_replace(ren_in, "_C08_", "_C03_");
                }
                if (mstr::contains(ren_in, "_THz_")) {
                    i = 4;
                    ren_out = mstr::string_replace(ren_in, "_C09_", "_C04_");
                }
                if (ren_in != ren_out) {
                    atsh->header.channel_number = i;
                    atsh->re_write();
                    fs::rename(file.path(), fs::path(file.path()).replace_filename(ren_out));
                }



            }
            for (const auto & file: fs::directory_iterator(s2dir)) {
                std::cout << file.path() << std::endl;
                auto atsh = std::make_shared<atsheader>(file.path());
                xmls_and_files.emplace(atsh->gen_xmlfilename(), atsh->path());
            }
            xml_from_ats(exec_path, xmls_and_files, calibs);

        }
        catch (const std::string &error ) {
            std::cerr << error <<std::endl;
            return EXIT_FAILURE;
        }
        catch (...) {

        }




        return EXIT_SUCCESS;

    }

    // ************************************************************************ C H A T S *******************************************************************************************


    if (chats) {
        if (outdir.empty()) {
            std::cout << "please supply -outdir name" << std::endl;
            return EXIT_FAILURE;
        }
        if (!atsheaders.size()) {
            std::cout << "chats you need one file(s) or more" << std::endl;
            return EXIT_FAILURE;
        }

        std::vector<std::shared_ptr<atsheader>> adu08s;
        std::vector<std::shared_ptr<ats_header_json>> atsjs;


        for (auto &atsh : atsheaders) {

            auto adu06 = std::make_shared<adu06_fname>(atsh->path());
            auto atsj = std::make_shared<ats_header_json>( atsh->header,  atsh->path());
            atsj->get_ats_header();
            // the old software does not reliably set the chopper in the ats header - must guess!
            if (change_chopper) {
                if (atsj->header["sample_rate"] < 513) atsj->header["chopper"] = ChopperStatus::on;
            }
            else atsj->header["chopper"] = chopper;

            atsj->header["channel_number"] = adu06->channel_number;

            std::cout << atsj->header["channel_type"].get<std::string>() << " " <<  atsj->header["sample_rate"] << " chopper: " << atsj->header["chopper"] << std::endl;
            std::cout << atsj->header["x1"] << " " << atsj->header["y1"] << std::endl;
            std::cout << atsj->filename << " " << atsj->header["ADB_board_type"].get<std::string>() << std::endl;
            std::cout << atsj->header["sensor_type"].get<std::string>() << " " << atsj->header["sensor_serial_number"] << std::endl;

            if (default_e_sensor.size() && atsj->header["channel_type"].get<std::string>().starts_with("E")) {
                atsj->header["sensor_type"] = default_e_sensor;
                std::cout << "sensor_type changed to " << atsj->header["sensor_type"].get<std::string>() << std::endl;

            }

            if (default_h_sensor.size() && atsj->header["channel_type"].get<std::string>().starts_with("H")) {
                atsj->header["sensor_type"] = default_h_sensor;
                std::cout << "sensor_type changed to " << atsj->header["sensor_type"].get<std::string>() << std::endl;
            }

            // we have the old MFS06 instead MFS-06 format, use read_cal to convert
            auto rd = std::make_shared<read_cal>();
            atsj->header["sensor_type"] = rd->get_sensor_name(atsj->header["sensor_type"]);
            calibs.emplace_back(std::make_shared<calibration>(rd->get_sensor_name(atsj->header["sensor_type"]), atsj->header["sensor_serial_number"], CalibrationType::mtx ));

            auto calfilename = calibs.back()->gen_json_filename_from_blank(ChopperStatus(atsj->header["chopper"]));
            std::cout << fs::path(json_caldir / calfilename) << std::endl;
            if (fs::exists(fs::path(json_caldir / calfilename))) {
                calibs.back()->read_file(json_caldir / calfilename);
            }
            else calibs.pop_back();
            if (shift_start_time) {
                atsj->header["start"] = int64_t(atsj->header["start"]) + shift_start_time;
            }
            atsj->set_ats_header();                                 // apply changes to binaray
            auto adu08 = std::make_shared<atsheader>();             // make a new file
            adu08->header = atsj->atsh;
            fs::create_directory(outdir / fs::path(atsj->measdir()));
            auto meas =   fs::path(atsj->measdir()) / fs::path(atsh->get_ats_filename(adu06->run));
            adu08->set_new_filename(outdir / meas); // set a new output

            adu08s.emplace_back(adu08);
            atsjs.emplace_back(atsj);

            xmls_and_files.emplace(adu08->gen_xmlfilename(), adu08->path());

        }
        // ask HW and split threads
        std::vector<size_t> execs = mk_mini_threads(0, atsheaders.size());
        size_t thread_index = 0;
        for (const auto &ex : execs) {
            try {
                std::vector<std::jthread> threads;
                std::cout << "start chats thread " << std::endl;
                //size_t i = 0;
                for (size_t j = 0; j < ex; ++j) {

                    threads.emplace_back(std::jthread (chats_files, std::ref(atsheaders[thread_index]), std::ref(adu08s[thread_index]), std::ref(atsjs[thread_index]), std::ref(adu06_shift_samples_hf), std::ref(adu06_shift_samples_lf)));
                    //chats_files(atsh, adu08, atsj, std::ref(adu06_shift_samples_hf), std::ref(adu06_shift_samples_lf));
                    ++thread_index;
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
        }



        ///////////////   check format ! JSON has mV and is non normalized

        std::vector<std::shared_ptr<calibration>> xxcal;
        for (const auto &calib : calibs) {
            xxcal.emplace_back(std::make_shared<calibration>(calib));
            if (xxcal.back()->cal_type() == CalibrationType::mtx) xxcal.back()->tasks_todo(false, false, false, false, true);
        }
        remove_cal_duplicates(xxcal);

        xml_from_ats(exec_path, xmls_and_files, xxcal);
        return EXIT_SUCCESS;

    }

    // ************************************************************************ T O J S O N || C L O N E *******************************************************************


    if (tojson || clone) {

        std::unique_ptr<survey_d> survey;

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
            try {
                survey = std::make_unique<survey_d>(outdir / clone_dir.filename(), false, atsheaders.size());
            }

            catch (std::filesystem::filesystem_error& e) {
                std::cerr <<  e.what() << std::endl;
                return EXIT_FAILURE;
            }
            catch( const std::string &error ) {
                std::cerr << error <<std::endl;
                return EXIT_FAILURE;
            }
            catch (...) {
                std::cerr << "create SURVEY" << std::endl;
                return EXIT_FAILURE;
            }
        }

        try {
            for (const auto &atsh : atsheaders) {
                //pool->push_task(collect_atsheaders, std::ref(atsh), std::ref(survey), std::ref(shift_start_time));
                collect_atsheaders(atsh, survey, shift_start_time);

            }
            // pool->wait_for_tasks();

        }
        catch( const std::string &error ) {
            std::cerr << error <<std::endl;
            return EXIT_FAILURE;
        }
        catch(std::filesystem::filesystem_error& e) {
            std::cerr <<  e.what() << std::endl;
            return EXIT_FAILURE;
        }
        catch (...) {
            std::cerr << "could not execute all threads" << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "done collecting" << std::endl;

        auto vch = survey->get_all_channels();

        for (const auto &ch : vch) {
            if (ch->cal->is_empty() && fs::exists(json_caldir)) {
                auto read_name = ch->cal->gen_json_filename_from_blank(ch->cal->chopper);
                std::cout << "empty calibration, try to read: " << read_name << std::endl;
                if (fs::exists(json_caldir / read_name)) {
                    ch->cal->read_file((json_caldir / read_name), false);
                }
            }
        }

        std::sort(vch.begin(), vch.end(), compare_channel_start_lt);



        try {
            survey->mk_tree();
        }
        catch( const std::string &error ) {
            std::cerr << error <<std::endl;
            return EXIT_FAILURE;
        }
        catch(std::filesystem::filesystem_error& e) {
            std::cerr <<  e.what() << std::endl;
            return EXIT_FAILURE;
        }
        catch (...) {
            std::cerr << "could not execute all threads" << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "done make tree" << std::endl;

        try {
            for (size_t i = 0; i < vch.size(); ++i) {
                //fill_survey_tree(survey, i);
                pool->push_task(fill_survey_tree, std::ref(survey), i);
            }
            pool->wait_for_tasks();
        }
        catch( const std::string &error ) {
            std::cerr << error <<std::endl;
            return EXIT_FAILURE;
        }
        catch(std::filesystem::filesystem_error& e) {
            std::cerr <<  e.what() << std::endl;
        }
        catch (...) {
            std::cerr << "could not execute all threads" << std::endl;
            return EXIT_FAILURE;
        }

        try  {
            if (std::filesystem::exists((clone_dir / "doc"))) {
                std::filesystem::copy((clone_dir / "doc"), (survey->survey_path() / "reports"), std::filesystem::copy_options::skip_existing | std::filesystem::copy_options::recursive);
            }
            if (std::filesystem::exists((clone_dir / "shell"))) {
                std::filesystem::copy((clone_dir / "shell"), (survey->survey_path() / "shell"), std::filesystem::copy_options::skip_existing | std::filesystem::copy_options::recursive);
            }
        }
        catch (std::exception& e) {
            std::cerr << e.what();
        }
        for (const auto &ch : vch) {
            std::cout << ch->brief() << std::endl;
        }
        std::cout << "done" << std::endl;

        return EXIT_SUCCESS;
    }



    return EXIT_SUCCESS;
}
