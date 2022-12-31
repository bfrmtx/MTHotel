#ifndef XML_FROM_ATS_H
#define XML_FROM_ATS_H

#include <iostream>
#include <vector>
#include <list>
#include <memory>
#include <filesystem>
#include <string>
#include <map>

#include "about_system.h"
#include "atsheader_def.h"
#include "atsheader.h"
#include "atsheader_xml.h"
#include "cal_base.h"
#include "../xml/tinyxmlwriter/tinyxmlwriter.h"
#include "../cal/read_cal/read_cal.h"

// std::vector<std::shared_ptr<calibration>> calibs;       //!< all JSON style calibrations
// std::multimap<std::string, fs::path> xmls_and_files;    //!< create a multimap which ats files belong to the same XML

void xml_from_ats (std::multimap<std::string, fs::path> &xmls_and_files, const std::vector<std::shared_ptr<calibration>> &calibs) {
    std::cout << "try to generate new XML files" << std::endl;

    sort_xml_and_files(xmls_and_files);

    std::vector<std::string> new_xml_files;
    std::vector<fs::path> new_xml_paths;

    for(const auto &x: xmls_and_files) {
        // std::cout<< x.first <<":"<< x.second << std::endl;
        if (!new_xml_files.size()) {

            new_xml_files.push_back(x.first);
            new_xml_paths.push_back(x.second);
        }
        else if (new_xml_files.back() != x.first) {
            new_xml_files.push_back(x.first);
            new_xml_paths.push_back(x.second);

        }
    }

    std::cout << std::endl;
    auto rd = std::make_shared<read_cal>();
    std::string old_sensor_type;
    for (size_t i = 0; i < new_xml_files.size(); ++i) {
        try {
            std::cout << "set " << i << std::endl;

            std::vector<std::shared_ptr<ats_header_json>> json_into_xml;

            auto sets = xmls_and_files.equal_range(new_xml_files.at(i));
            std::cout << new_xml_files.at(i) << " => ";
            for (auto set = sets.first; set !=sets.second; ++set) {
                std::cout << ' ' << set->second;
                // create a atsheader and read the header
                auto atsx = std::make_shared<atsheader>(set->second);
                json_into_xml.emplace_back(std::make_shared<ats_header_json>(atsx->header, atsx->path()));
                json_into_xml.back()->get_ats_header();
                old_sensor_type = json_into_xml.back()->header["sensor_type"];
                json_into_xml.back()->header["sensor_type"] = rd->get_sensor_name(json_into_xml.back()->header["sensor_type"]);

            }
            std::cout << std::endl;


            fs::path outxmlfile(new_xml_paths.at(i).parent_path());
            outxmlfile += "/";
            outxmlfile += new_xml_files.at(i);


            auto tix = std::make_shared<tinyxmlwriter>(true, outxmlfile);
            tix->push("measurement");
            tix->push("recording");
            // for (const auto &atsj : json_into_xml) {
            recording_ats_sub(tix, json_into_xml.at(0));
            // }
            tix->push("input");
            //for (const auto &atsj : json_into_xml) {
            tix->push("Hardware", HWNode_ats_str(json_into_xml.at(0)));
            //}
            global_config_ats(tix, json_into_xml.at(0), json_into_xml.size());
            for (const auto &atsj : json_into_xml) {
                Hardware_channel_config_xml(tix, atsj);
            }
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
            for (const auto &atsj : json_into_xml) {
                ATSWriter_channel_xml(tix, atsj);
            }
            tix->pop("configuration");
            ATSWriter_comments_xml(tix, json_into_xml.at(0));

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

            tix->push("calibration_sensors");
            bool cal_found = false;
            bool cal_head_added = false;
            for (const auto &atsj : json_into_xml) {
                tix->push("channel", "id", static_cast<int>(atsj->header["channel_number"]));


                for (const auto &cal : calibs) {
                    if (((cal->sensor == atsj->header["sensor_type"].get<std::string>()) || cal->sensor == old_sensor_type) && (cal->serial == atsj->header["sensor_serial_number"]) ) {
                        cal_found = true;
                        if (!cal_head_added) cal->add_to_xml_1_of_3(tix);
                        cal_head_added = true;
                        // xml has chopper on AND chopper off
                        cal->add_to_xml_2_of_3(tix);

                    }
                }
                cal_head_added = false;
                if (cal_found) {
                    // if found we had at least one entry
                    calibs.at(0)->add_to_xml_3_of_3(tix);
                }
                if (!cal_found) {
                    tix->push("calibrated_item");

                    tix->element("ci", atsj->header["sensor_type"].get<std::string>());
                    tix->element("ci_serial_number", atsj->header["sensor_serial_number"]);
                    tix->pop("calibrated_item");

                }
                tix->pop("channel");
            }

            tix->pop("calibration_sensors");

            tix->pop("measurement");
            tix->write_file();
        }

        catch (const std::string &error) {

            std::cerr << error << std::endl;
        }
    }
}


#endif // XML_FROM_ATS_H
