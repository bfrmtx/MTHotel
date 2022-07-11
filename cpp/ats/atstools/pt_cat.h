#ifndef PT_CAT_H
#define PT_CAT_H

#include <iostream>
#include <vector>
#include <list>
#include <memory>
#include <filesystem>
#include <string>
#include <map>
#include <mutex>
#include <shared_mutex>

#include "xml_from_ats.h"

#include "atsheader_def.h"
#include "atsheader.h"
#include "atmheader.h"
#include "atsheader_xml.h"
#include "cal_base.h"

void cat_ats_files(const std::vector<std::shared_ptr<atsheader>> &ats, const std::filesystem::path &outdir_base, std::multimap<std::string, std::filesystem::path> &xmls_and_files, std::vector<std::filesystem::path> &xml_files,
                   std::mutex &mtx_dir, std::mutex &mtx_xml, std::mutex &mtx_xml_files) {

    if (!ats.size()) return;
    if (outdir_base.empty()) {
        std::string err_str = __func__;
        err_str += "::no -outdir supplied or";
        throw err_str;
        return;
    }

    if (!std::filesystem::exists(outdir_base)) {
        std::string err_str = __func__;
        err_str += "::top outdir does not exists!, want to create outdir/meas_dir!";
        throw err_str;
        return;
    }

    //std::unique_lock<std::mutex> lck (mtx, std::defer_lock); // don't lock the mutex on construction
    std::filesystem::path outdir(outdir_base);

    for (size_t i = 0; i < ats.size()-1; ++i) {

       // int64_t dt = diff_time(ats.at(i), ats.at(i+1));
       // std::cout <<  ats.at(i)->path().filename() << " add samples: " << dt << " " <<  ats.at(i+1)->path().filename() << std::endl;


        try {
            size_t chunk_size = 524288;
            std::vector<std::int32_t> ints(chunk_size);
            size_t samples_read = 0;
            ats[0]->read();
            auto atsj = std::make_shared<ats_header_json>( ats[0]->header,  ats[0]->path());
            atsj->get_ats_header();
            auto out = std::make_shared<atsheader>(ats.at(0));
            outdir /= atsj->measdir();
            // fetch xml file from atsheader
            try {
                std::lock_guard<std::mutex> lck (mtx_dir);
                if(std::find(xml_files.begin(), xml_files.end(), atsj->xml_path()) == xml_files.end()) {
                    xml_files.push_back(atsj->xml_path());
                }
                if (!std::filesystem::exists(outdir)) {
                    std::filesystem::create_directory(outdir);
                    std::cout << "creating " << outdir << std::endl;
                    if (!std::filesystem::exists(outdir)) {
                        std::cout << "creating " << outdir << " " << "failed!" <<  std::endl;
                    }
                }
            }
            catch (...) {
                std::cerr << "execption cought in find::xml_files" << std::endl;
                std::cerr << "std::filesystem::create_directory" << std::endl;
                return;


            }
            atsj.reset();

            out->change_dir(outdir);
            out->write(); // write header and keep file open
           // std::cout << out->path() << std::endl;
            auto atm = std::make_shared<atmfile>();
            atm->get_atsfile_name(out->path());

            size_t j = 0;

            for (j = 0; j < ats.size(); ++j) {
                if (j) {
                    ats[j - 1]->close();
                    ats[j]->read();
                    auto atsj = std::make_shared<ats_header_json>( ats[j]->header,  ats[j]->path());
                    atsj->get_ats_header();
                    // fetch xml file from atsheader

                    try {
                        std::lock_guard<std::mutex> lck (mtx_xml);
                        if(std::find(xml_files.begin(), xml_files.end(), atsj->xml_path()) == xml_files.end()) {
                            xml_files.push_back(atsj->xml_path());
                        }
                    }
                    catch (...) {
                        std::cerr << "execption cought in find::xml_files" << std::endl;
                        return;

                    }

                    atsj.reset();
                    ints.resize(chunk_size);
                }
                while (ats[j]->ats_read_ints_doubles(ints)) {
                   // std::cout << ".";
                    out->ats_write_ints_doubles(out->header.lsbval, ints);
                    atm->add_unselected(ints.size());
                    samples_read += ints.size();
                }
                // std::cout << std::endl;
                if (j < ats.size() - 1) {
                    if (ats_can_simple_cat(ats.at(j), ats.at(j + 1))) {
                        int64_t dt = diff_time(ats.at(j), ats.at(j + 1));
                       // std::cout << "samples to add: " << dt << " after sample: " << samples_read << " (write count " << out->write_count << ")" << std::endl;

                        if (dt >= 0) {
                            out->ats_zero_ints(dt);
                            atm->add_selected(dt);
                        }
                    }
                }
            }

            out->close();
            atm->close(true);
            out->header.samples = std::uint32_t(out->write_count);
           // std::cout << "total samples: " << out->header.samples << " " << atm->header.samples << std::endl;
            // update header with new samples AND new xml file
            out->re_write();
            // pupolate a multimap with the NEW ats file and NEW XML file


            try {
                std::lock_guard<std::mutex> lck (mtx_xml_files);
                xmls_and_files.emplace(out->gen_xmlfilename(), out->path());

            }
            catch (...) {
                std::cerr << "execption cought in xmls_and_files.emplace" << std::endl;
                return;

            }

            out.reset();
        }
        catch (const std::string &error) {

            std::cerr << error << std::endl;
        }

    }

}



#endif // PT_CAT_H
