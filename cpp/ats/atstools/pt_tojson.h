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

/*!
 * \brief ats2json
 * \param ats
 * \param outdir_base something like /survey/France/Lile IN CASE OF and COMPLETE SURVEY it must be /survey/France/Lile/ts where to create the site & measdir
 * \param dirlock
 * \param create_measdir
 * \param create_sitedir
 */
void ats2json(std::shared_ptr<atsheader> &ats, const std::filesystem::path &outdir_base, std::mutex &dirlock, const bool &create_measdir = false, const bool &create_sitedir = false) {

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

    std::filesystem::path outdir(outdir_base);   // in cse of clone it ends with /ts
    std::filesystem::path metadir((outdir_base.parent_path()) /= "meta");
    size_t samples_read = 0;
    bool my_create_measdir = false;

    try {
        ats->read();            // get the binary data from the header; keep file open
        auto atsj = std::make_shared<ats_header_json>(ats->header,  ats->path());
        atsj->get_ats_header(); // fill the json
        // atsj->header["sensor_type"].get<std::string>())
        auto chan = std::make_shared<channel<double>>(atsj->header["channel_type"], atsj->header["sample_rate"], atsj->start_datetime(), 0.0);
        chan->set_base_file<int64_t>(atsj->header["SystemType"], atsj->header["serial_number"], atsj->header["channel_number"], atsj->get_run());
        chan->set_lat_lon_elev(atsj->get_lat(), atsj->get_lon(), atsj->get_elev());
        chan->angle = atsj->pos2angle();
        chan->dip = atsj->pos2dip();
        auto rcal = std::make_shared<read_cal>();
        auto cal = std::make_shared<calibration>();
        cal->set_format(CalibrationType::mtx, false);
        cal->units_amplitude = "mV"; // may be changed later when try to scale in case E is assumed
        // fill with vals rom ats header as far as possible
        cal->sensor = rcal->get_sensor_name(atsj->header["sensor_type"]);
        if (cal->sensor == "") cal->sensor = atsj->header["sensor_type"];
        cal->serial = atsj->header["sensor_serial_number"];
        cal->chopper = atsj->get_chopper();
        std::vector<std::shared_ptr<calibration>> cals;
        try{
            cals = rcal->read_std_xml(atsj->xml_path());
        }
        catch (const std::string &error) {
            std::cerr << error << std::endl;
            std::cerr << "continued" << std::endl;
            cals.clear();
        }

        if (create_sitedir) {
            auto rp = ats->path().root_path();
            auto up = ats->path().parent_path(); // measxxx
            if(!up.empty()) {
                up = up.parent_path();  // should be the site name
                if ((up != rp) && !up.empty()) {
                    // std::cout << up << " " << up.filename() << std::endl;
                    if (up.has_filename()) {
                        auto str = up.filename();
                        outdir /= up.filename();
                        metadir /= up.filename();
                        my_create_measdir = true;
                    }
                }

            }
        }

        if (my_create_measdir || create_measdir) {
            auto meas = atsj->measdir();

            try {
                std::lock_guard<std::mutex> lck (dirlock);
                //std::filesystem::create_directory(outdir);
                outdir /= meas;
                metadir /= meas;
                std::filesystem::create_directories(outdir);
                std::filesystem::create_directories(metadir);

            }
            catch (std::error_code& ec) {
                std::string err_str = std::string("atstools->") + __func__;
                std::cerr << ec.message();
                std::cerr << err_str << " " << outdir << std::endl;
            }


        }


        double lsb = atsj->header["lsbval"];
        if (atsj->can_and_want_scale()) {
            lsb *= 1000. / atsj->pos2length();
            chan->units = "mV/km";
        }
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

        // now read the data, convert to double and scale E in case
        size_t chunk_size = 524288;
        std::vector<std::int32_t> ints;
        std::vector<double> dbls;
        if (atsj->header["samples"] < chunk_size) {
            dbls.resize(atsj->header["samples"]);
            ints.resize(atsj->header["samples"]);
        }
        else {
            dbls.resize(chunk_size);
            ints.resize(chunk_size);
        }


        std::ofstream file;
        try {
            chan->prepare_write_atss(outdir, file);
        }
        catch (const std::string &error) {

            std::cerr << error << std::endl;
            return;
        }

        do {
            dbls.resize(ats->ats_read_ints_doubles(ints));
            samples_read += dbls.size();
            std::transform(ints.begin(), ints.end(), dbls.begin(), [lsb](double v) { return (lsb * v);} );
            chan->write_bin(dbls, file);

        } while (dbls.size() && file.good());
        file.close();


        std::cout << chan->filename() << std::endl;
        std::cout << chan->filename("json") << "  " << samples_read << " <-> " << chan->samples(outdir) <<  std::endl;

        if (std::filesystem::exists(metadir)) {
            auto meta = atsj->write_meta(metadir, chan->filename("json"));
            auto new_xml = metadir /= atsj->xml_path().filename();
            std::cout << meta << " written" << std::endl;
            try {
                std::lock_guard<std::mutex> lck (dirlock);
                std::filesystem::copy_file(atsj->xml_path(), new_xml, std::filesystem::copy_options::skip_existing);
            } catch(fs::filesystem_error& e) {
                std::cout << "Could not copy XML file: " <<  atsj->xml_path() << " -> " << e.what() << '\n';
            }


        }

        // read test
        //        std::ifstream ifile;
        //        chan->prepare_read_atss(outdir, ifile);
        //        size_t chunks = 8192;
        //        dbls.resize(chunks);

        //        do {
        //            chan->read_bin(dbls, ifile, true);
        //            if (dbls.size()) std::cout << "read " << dbls.size() << std::endl;
        //        } while (dbls.size());
        //        ifile.close();

    }

    catch (...) {
        std::string err_str = __func__;
        err_str += "::failed!";
        throw err_str;
        return;

    }



    return;
}

#endif // PT_TOJSON_H
