#ifndef PT_TOJSON_H
#define PT_TOJSON_H

#include <iostream>
#include <vector>
#include <list>
#include <memory>
#include <filesystem>
#include <string>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <algorithm>

#include "xml_from_ats.h"

#include "atsheader_def.h"
#include "atsheader.h"
#include "atss.h"
#include "cal_base.h"
#include "read_cal.h"

void ats2json(std::shared_ptr<atsheader> &ats, const std::filesystem::path &outdir_base) {

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

    std::filesystem::path outdir(outdir_base);


    try {
        size_t chunk_size = 524288;
        std::vector<double> dbls(chunk_size);
        size_t samples_read = 0;
        ats->read();
        auto atsj = std::make_shared<ats_header_json>(ats->header,  ats->path());
        atsj->get_ats_header(); // fill the json
        // atsj->header["sensor_type"].get<std::string>())
        auto chan = std::make_shared<channel<double>>(atsj->header["channel_type"], atsj->header["sample_rate"], atsj->start_date(), atsj->start_time(), 0.0);
        chan->set_base_file<int64_t>(atsj->header["SystemType"], atsj->header["serial_number"], atsj->header["channel_number"], atsj->get_run());
        chan->set_lat_lon_elev(atsj->get_lat(), atsj->get_lon(), atsj->get_elev());
        chan->angle = atsj->pos2angle();
        chan->dip = atsj->pos2dip();
        auto rcal = std::make_shared<read_cal>();
        auto cal = std::make_shared<calibration>();
        cal->set_format(CalibrationType::mtx, false);
        cal->units_amplitude = "mV";
        // fill with vals rom ats header as far as possible
        cal->sensor = rcal->get_sensor_name(atsj->header["sensor_type"]);
        if (cal->sensor == "") cal->sensor = atsj->header["sensor_type"];
        cal->serial = atsj->header["sensor_serial_number"];
        cal->chopper = atsj->get_chopper();
        auto cals = rcal->read_std_xml(atsj->xml_path());

        size_t found_in_xml_etc = 0;
        for (auto &ncal : cals) {
            if (compare_sensor_and_chopper(cal, ncal) ) {
                ncal->old_to_newformat();
                ++found_in_xml_etc;
                try {
                    chan->write_header(outdir, ncal->toJson_embedd());
                }
                catch (const std::string &error) {
                    std::cerr << error << std::endl;
                    cals.clear();
                    return;
                }
            }
        }
        // we have no cal, so write an empty section with sensor name but no cal data
        if (!found_in_xml_etc) {
            try {
                chan->write_header(outdir, cal->toJson_embedd());
            }
            catch (const std::string &error) {
                std::cerr << error << std::endl;
                cals.clear();
                return;
            }
        }

        std::cout << chan->filename() << std::endl;
        outdir /= chan->filename("json");
        std::cout << outdir << std::endl;
        //chan->write_header(cal->toJson_embedd());


    }

    catch (...) {
        std::cerr << "std::filesystem::create_directory" << std::endl;
        return;


    }
}

#endif // PT_TOJSON_H
