#include "read_cal.h"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <memory>

int main(int argc, char **argv)
{
    using namespace std;
    namespace fs = std::filesystem;

    bool tojson = false;
    bool toxml = false;
    bool tomtx = false;
    bool ampl_div_f = false;
    bool ampl_mul_f = false;
    bool ampl_mul_by_1000 = false;
    bool old_to_new = false;
    bool new_to_old = false;
    bool force_measdoc = false;
    bool force_single = false;


    fs::path outdir;

    unsigned l = 1;
    while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
        std::string marg(argv[l]);

        if (marg.compare("-tojson") == 0) {
            tojson = true;
        }
        if (marg.compare("-toxml") == 0) {
            toxml = true;
        }
        if (marg.compare("-tomtx") == 0) {
            tomtx = true;
        }
        if (marg.compare("-ampl_div_f") == 0) {
            ampl_div_f = true;
        }
        if (marg.compare("-ampl_mul_f") == 0) {
            ampl_mul_f = true;
        }
        if (marg.compare("-ampl_mul_by_1000") == 0) {
            ampl_mul_by_1000 = true;
        }
        if (marg.compare("-old_to_new") == 0) {
            old_to_new = true;
        }
        if (marg.compare("-new_to_old") == 0) {
            new_to_old = true;
        }
        if (marg.compare("-force_measdoc") == 0) {
            force_measdoc = true;
        }
        if (marg.compare("-force_single") == 0) {
            force_single = true;
        }




        if (marg.compare("-outdir") == 0) {
            outdir = std::string(argv[++l]);
        }
        if ((marg.compare("-help") == 0) || (marg.compare("--help") == 0)) {
            std::cout << "-toxml file.txt" << std::endl;
            std::cout << "-toxml *.txt "<< std::endl;

            return EXIT_FAILURE;
        }
        else if (marg.compare("-") == 0) {
            std::cerr << "\nunrecognized option " << argv[l] << std::endl;
            return EXIT_FAILURE;
        }
        ++l;
    }

    if (!outdir.empty()) {
        if (!std::filesystem::exists(outdir)) {
            std::filesystem::create_directory(outdir);
            if(!std::filesystem::exists(outdir)) {
                std::cout << "can not create outdir" << std::endl;
                return EXIT_FAILURE;
            }
            std::cout << outdir << " created" << std::endl;
        }
    }

    std::vector<std::shared_ptr<calibration>> cals;
    std::multimap<fs::path, size_t> mtxfiles_and_cals;

    l = 1;
    while (argc > 1 && (l < unsigned(argc))) {
        std::string marg(argv[l]);
        if ( (marg.compare(marg.size()-4, 4, ".txt") == 0) || (marg.compare(marg.size()-4, 4, ".TXT") == 0) ) {

            std::shared_ptr<read_cal> mtx_cal_file = std::make_shared<read_cal>();

            try {

                cals.emplace_back(mtx_cal_file->read_std_mtx_txt(marg, ChopperStatus::off));
                if (!cals.back()->f.size()) cals.pop_back();
                else if (cals.size()) mtxfiles_and_cals.emplace(fs::path(marg),cals.size()-1);
                cals.emplace_back(mtx_cal_file->read_std_mtx_txt(marg, ChopperStatus::on));
                if (!cals.back()->f.size()) cals.pop_back();
                else if (cals.size()) mtxfiles_and_cals.emplace(fs::path(marg),cals.size()-1);


            }
            catch (const std::string &error) {

                std::cerr << error << std::endl;
                cals.clear();

                // we could also reset mtx_cal_file.reset()
                // depending on what we want to do
                // in a loop of many files we keep mtx_cal_file alive
            }

        }


        if ( (marg.compare(marg.size()-4, 4, ".xml") == 0) || (marg.compare(marg.size()-4, 4, ".XML") == 0) ) {
            std::shared_ptr<read_cal> mtx_cal_file = std::make_shared<read_cal>();
            try {
                std::vector<std::shared_ptr<calibration>> xcals;
                if ((isdigit(marg.at(0)) || force_measdoc) && !force_single)  xcals = mtx_cal_file->read_std_xml(marg);
                else xcals = mtx_cal_file->read_std_xml_single(marg);

                for (auto &xcal : xcals) {
                    if (!xcal->is_empty()) {
                        cals.push_back(xcal);
                    }
                }

            }
            catch (const std::string &error) {

                std::cerr << error << std::endl;
                cals.clear();

                // we could also reset mtx_cal_file.reset()
                // depending on what we want to do
                // in a loop of many files we keep mtx_cal_file alive
            }

        }

        if ( (marg.compare(marg.size()-5, 5, ".json") == 0) || (marg.compare(marg.size()-5, 5, ".JSON") == 0) ) {
            try {
                auto cal = std::make_shared<calibration>();
                cal->read_file(marg, true);
                cals.emplace_back(cal);
            }
            catch (const std::string &error) {

                std::cerr << error << std::endl;
                cals.clear();

            }

        }
        ++l;
    }

    if( !cals.size() ) {
        std::cout << "no calibrations found / loaded" << std::endl;
        return EXIT_FAILURE;
    }


    // in case operations are forced - do it here
    for (auto &cal : cals) {
        cal->tasks_todo(ampl_div_f, ampl_mul_f, ampl_mul_by_1000, old_to_new, new_to_old);
    }

    if (toxml) {

        fs::path y;
        for(const auto &x: mtxfiles_and_cals) {

            if (y != x.first) {
                std::pair <std::multimap<fs::path, size_t>::iterator, std::multimap<fs::path, size_t>::iterator> ret;
                fs::path outxmlfile;
                if (!outdir.empty()) {
                    outxmlfile = outdir;
                    outxmlfile /= x.first.filename();
                }
                else outxmlfile = x.first;
                outxmlfile.replace_extension("xml");
                fs::path outxmlfile_plot(outxmlfile);
                outxmlfile_plot.replace_filename(outxmlfile.stem() += "_plot.xml");

                auto tix = std::make_shared<tinyxmlwriter>(true, outxmlfile);
                auto tixplot = std::make_shared<tinyxmlwriter>(true, outxmlfile_plot);

                ret = mtxfiles_and_cals.equal_range(x.first);
                std::cout << x.first << " -> " << outxmlfile << " =>";
                int segments = 0;
                for (std::multimap<fs::path, size_t>::iterator it=ret.first; it!=ret.second; ++it) {
                    if (!segments) {
                        if (it->second < cals.size()) {
                            cals.at(it->second)->add_to_xml_1_of_3(tix);
                            cals.at(it->second)->add_to_xml_2_of_3(tix);
                            tixplot->push("calibration_sensors");
                            tixplot->push("channel", "id", it->second);
                            cals.at(it->second)->add_to_xml_1_of_3(tixplot);
                            cals.at(it->second)->add_to_xml_2_of_3(tixplot);
                        }
                    }
                    else {
                        if (it->second < cals.size()) {
                            cals.at(it->second)->add_to_xml_2_of_3(tix);
                            cals.at(it->second)->add_to_xml_2_of_3(tixplot);
                        }
                    }
                    ++segments;
                    std::cout << ' ' << it->second;
                }
                // below add_to_xml_3_of_3(tix);
                tix->pop("calibration");
                tix->write_file();
                tixplot->pop("calibration");
                tixplot->pop("channel");
                tixplot->pop("calibration_sensors");
                tixplot->write_file();

                std::cout << '\n';
            }
            y = x.first;
        }

        //        for (const auto &cal : cals ) {
        //            fs::path outxmlfile("/home/bfr/xxx.xml");
        //            auto tix = std::make_shared<tinyxmlwriter>(true, outxmlfile);
        //            cal->add_to_xml_1_of_3(tix);
        //            cal->add_to_xml_2_of_3(tix);
        //            cal->add_to_xml_3_of_3(tix);

        //            if (compare_same_senor(cal, cal)) {
        //                std::cout << "same" << std::endl;
        //                cal->add_to_xml_2_of_3(tix);
        //            }

        //            tix->write_file();

        //        }
    }

    if (tojson) {
        //        if (cal != nullptr) {
        //            cal->write_file("/home/bfr");
        //        }
    }

    if (tomtx) {

    }




    std::cout  << endl << "finish write " << endl;



    return 0;
}
