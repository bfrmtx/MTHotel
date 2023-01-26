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
    bool keep_name = false;


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

        if (marg.compare("-keep_name") == 0) {
            keep_name = true;
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
        if ((marg.compare("-help") == 0) || (marg.compare("--help") == 0)) {
            std::cout << "Sensor and Serial are derived from filename ONLY!" << std::endl;
            std::cout << "-toxml file.txt" << std::endl;
            std::cout << "-toxml *.txt "<< std::endl;
            std::cout << "-new_to_old -tojson file.txt" << std::endl;
            std::cout << "-new_to_old -tojson *.txt "<< std::endl;
            std::cout << "you may want to call "<< std::endl;
            std::cout << "-outdir /home/newcal -old_to_new -tojson *.txt "<< std::endl;
            std::cout << " use -outdir [options] *txt in order to place the results at a different place" << std::endl;
            std::cout << "-keep_name should be active for txt -> xml when using script files ancient style" << std::endl;
            std::cout << "otherwise file name would be " << std::endl;


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

    if (tojson && !old_to_new) {
        std::cout << "  ***************************************************************   "<< std::endl;
        std::cout << "    "<< std::endl;
        std::cout << "  YOU MAY WANTED TO CALL  "<< std::endl;
        std::cout << "-outdir /home/newcal -old_to_new -tojson *.txt "<< std::endl;
        std::cout << "-old_to_new -tojson file.txt" << std::endl;
        std::cout << "    "<< std::endl;
        std::cout << "  ***************************************************************   "<< std::endl;

    }

    std::vector<std::shared_ptr<calibration>> cals;
    std::multimap<fs::path, size_t> mtxfiles_and_cals;

    l = 1;
    while (argc > 1 && (l < unsigned(argc))) {

        fs::path check_ext(argv[l]);
        if (check_ext.has_extension()) {
            if ((check_ext.extension() == ".txt") ||  (check_ext.extension() == ".TXT")) {
                try {
                    std::shared_ptr<read_cal> mtx_cal_file_on = std::make_shared<read_cal>();
                    std::shared_ptr<read_cal> mtx_cal_file_off = std::make_shared<read_cal>();
                    cals.emplace_back(mtx_cal_file_off->read_std_mtx_txt(check_ext, ChopperStatus::off));
                    if (cals.back()->is_empty()) cals.pop_back();
                    else if (cals.size()) mtxfiles_and_cals.emplace(fs::path(check_ext), cals.size()-1);
                    cals.emplace_back(mtx_cal_file_on->read_std_mtx_txt(check_ext, ChopperStatus::on));
                    if (cals.back()->is_empty()) cals.pop_back();
                    else if (cals.size()) mtxfiles_and_cals.emplace(fs::path(check_ext), cals.size()-1);

                }
                catch (const std::string &error) {
                    std::cerr << error << std::endl;
                    cals.clear();
                }
            }

            if ((check_ext.extension() == ".xml") ||  (check_ext.extension() == ".XML")) {
                try {

                    std::shared_ptr<read_cal> mtx_cal_file = std::make_shared<read_cal>();
                    std::vector<std::shared_ptr<calibration>> xcals;
                    std::string fdig(check_ext.stem().string());
                    if ((isdigit(fdig.at(0)) || force_measdoc) && !force_single)  xcals = mtx_cal_file->read_std_xml(check_ext);
                    else xcals = mtx_cal_file->read_std_xml_single(check_ext);

                    for (auto &xcal : xcals) {
                        if (!xcal->is_empty()) {
                            cals.emplace_back(xcal);
                        }
                    }

                }
                catch (const std::string &error) {
                    std::cerr << error << std::endl;
                    cals.clear();
                }

            }
            if ((check_ext.extension() == ".json") ||  (check_ext.extension() == ".JSON")) {

                try {
                    auto cal = std::make_shared<calibration>();
                    cal->read_file(check_ext, true);
                    if (!cal->is_empty()) cals.emplace_back(cal);
                }
                catch (const std::string &error) {
                    std::cerr << error << std::endl;
                    cals.clear();

                }

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

    l = 0;
    // keep name should be active for txt -> xml when using script files ancient style
    if ( !keep_name  || (cals.size() != mtxfiles_and_cals.size())) {
        mtxfiles_and_cals.clear();
        for (auto &cal : cals) {
            mtxfiles_and_cals.emplace(cal->mtx_cal_head(outdir, true), l++);
        }
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

    }

    if (tojson) {
        for (auto &cal : cals) {
            cal->write_file(outdir);
        }
    }

    if (tomtx) {

        fs::path y;
        for(const auto &x: mtxfiles_and_cals) {

            if (y != x.first) {
                std::pair <std::multimap<fs::path, size_t>::iterator, std::multimap<fs::path, size_t>::iterator> ret;

                ret = mtxfiles_and_cals.equal_range(x.first);
                std::cout << x.first <<  " =>";
                int segments = 0;
                std::filesystem::path full_name;
                for (std::multimap<fs::path, size_t>::iterator it=ret.first; it!=ret.second; ++it) {
                    if (!segments) {
                        if (it->second < cals.size()) {
                            full_name = cals.at(it->second)->mtx_cal_head(outdir, false);
                            cals.at(it->second)->mtx_cal_body(full_name);
                        }
                    }
                    else {
                        if (it->second < cals.size()) {
                            cals.at(it->second)->mtx_cal_body(full_name);
                        }
                    }
                    ++segments;
                    std::cout << ' ' << it->second;
                }
                std::cout << '\n';
            }
            y = x.first;
        }


    }



    if (tojson && !old_to_new) {
        std::cout << "  ***************************************************************   "<< std::endl;
        std::cout << "    "<< std::endl;
        std::cout << "  YOU MAY WANTED TO CALL  "<< std::endl;
        std::cout << "-outdir /home/newcal -old_to_new -tojson *.txt "<< std::endl;
        std::cout << "-old_to_new -tojson file.txt" << std::endl;
        std::cout << "    "<< std::endl;
        std::cout << "  ***************************************************************   "<< std::endl;

    }

    std::cout  << endl << "finish write " << endl;



    return EXIT_SUCCESS;
}
