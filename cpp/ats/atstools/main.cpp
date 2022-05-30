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
#include "atmheader.h"
#include "atsheader_xml.h"
#include "cal_base.h"
#include "../cal/read_cal/read_cal.h"
#include "../xml/tinyxmlwriter/tinyxmlwriter.h"

#include "pt_cat.h"
#include "adu06_fname.h"

// run tests
// -outdir /tmp -cat /home/bfr/devel/ats_data/cat_ats_data/NGRI/meas_2019-11-20_06-52-49/*ats /home/bfr/devel/ats_data/cat_ats_data/NGRI/meas_2019-11-22_06-22-30/*ats
// -outdir /tmp -chats /home/bfr/devel/ats_data/zero6/site0199/*ats


/*
   string path = "/";

for (const auto & file : directory_iterator(path))
    cout << file.path() << endl;
*/


    int main(int argc, char* argv[])
{



    bool cat = false;                       //!< concatunate ats files, try read xml from atsheader & calibration from XML
    int run = -1;                           //!< run number, greater equal 0
    double lsbval = 0;                      //!< lsb
    bool chats = false;                     //!< convert ADU-06 files to ADU-08e files
    fs::path outdir;


    std::vector<std::shared_ptr<atsheader>> atsheaders;     //!< all ats files or ats headers

    std::vector<fs::path> indirs;
    std::vector<fs::path> xml_files;                        //!< all xml files - either direct read or from ats/atss generated
    std::vector<std::shared_ptr<calibration>> calibs;       //!< all JSON style calibrations

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
        //        else if (marg.compare("-lsbval") == 0) {
        //            lsbval = atof(argv[++l]);
        //        }
        else if (marg.compare("-run") == 0) {
            run = atoi(argv[++l]);
        }
        else if (marg.compare("-outdir") == 0) {
            outdir = std::string(argv[++l]);
        }

        else if (marg.compare("-") == 0) {
            std::cerr << "\nunrecognized option " << argv[l] << std::endl;
            return EXIT_FAILURE;
        }
        ++l;
    }

    l = 1;
    while (argc > 1 && (l < unsigned(argc))) {
        std::string marg(argv[l]);
        if ( (marg.compare(marg.size()-4, 4, ".ats") == 0) || (marg.compare(marg.size()-4, 4, ".ATS") == 0) ) {

            atsheaders.emplace_back(std::make_shared<atsheader>(fs::path(marg)));
        }
        ++l;
    }

    if (!atsheaders.size()) {
        std::cout << "no ats files found" << std::endl;
        return EXIT_FAILURE;
    }

    for (const auto& ats : atsheaders ) {
        std::cout <<  ats->path() <<  std::endl;
    }

    // first we order by start time
    std::sort(atsheaders.begin(), atsheaders.end(), compare_ats_start);
    // secondly we sort by channel type
    ats_channel_sort(atsheaders);


    //    std::find_if(atsheaders.begin(), atsheaders.end(), Greater(5, "lsb"));
    //    auto result = *std::find_if(atsheaders.begin()+1, atsheaders.end(), comp_if_equal<double>(atsheaders.front(), "lsbval"));

    //    std::cout << "test comp" << std::endl;
    //    std::cout << atsheaders.front()->path() << std::endl;
    //    std::cout << result->path() << std::endl;

    //    std::cout << "test comp end" << std::endl;

    //    for (const auto& ats : atsheaders ) {
    //        std::cout <<  ats->path().filename() << " x" <<  std::endl;
    //    }



    //        for (const auto& ats : atsheaders ) {
    //        std::cout <<  ats->path().filename() <<  " -> "  << ats->header.start <<  std::endl;
    //    }


    // ************************************************************************ C A T *******************************************************************************************

    if (cat) {
        if (!sizeof(outdir)) {
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
                threads.emplace_back(std::jthread (cat_ats_files, std::ref(ats), std::ref(outdir), std::ref(xmls_and_files), std::ref(xml_files), std::ref(mtx_dir), std::ref(mtx_xml), std::ref(mtx_xml_files)));
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


    return EXIT_SUCCESS;
}
