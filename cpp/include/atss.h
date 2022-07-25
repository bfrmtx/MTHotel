#ifndef ATSS_H
#define ATSS_H

#include <iostream>
#include <vector>
#include <string>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include "json.h"
#include "strings_etc.h"
#include <filesystem>


using jsn = nlohmann::ordered_json;


class p_timer {

public:
    p_timer(const std::string &datetime, const double &sample_rate, const double& fracs = 0.0) {
        this->tt = mstr::time_t_iso_8601_str(datetime);
        this->sample_rate = sample_rate;
        this->fracs = fracs;
    }

    time_t tt = 0;              //!< seconds since 1970, can be negative in some implementaions
    double fracs = 0.0;         //!< fractions of seconds from 0 ...0.9999...
    double sample_rate = 0.0;   //!< in Hz always
    size_t samples = 0;         //!< total samples of the file
    size_t spos = 0;            //!< sample pos

};

/*!
 * \brief The atss_file class is the FILENAME part of the atss format consisting of binary .atss, JSON .json
 *  the tags of the filename are NOT repeated in the JSON
 */
class atss_file {

public:
    /*!
     * \brief atss_file most common mimimum constructor
     * \param channel_type like Ex, Ex, ... Hx ..
     * \param sample_rate in Hz always
     */
    atss_file(const std::string &channel_type = "", const double& sample_rate = 0) {
        this->set_channel_type(channel_type);
        this->set_sample_rate(sample_rate);
    }

    template <typename T> void set_base_file(const std::string& system = "", const T& ser = 0,
                       const T& channel_no = 0, const T& run = 0) {
        this->set_system(system);
        this->set_serial(ser);
        this->set_channel_no(channel_no);
        this->set_run(run);
    }

    template <typename T> void set_serial(const T& ser) {
        this->serial = size_t(std::abs(ser));
    }

    void set_system(const std::string& system) {
        this->system = mstr::simplify(system, true);
    }

    void set_channel_type(const std::string& channel_type) {
        this->channel_type = mstr::simplify(channel_type, true);
    }

    template <typename T> void set_run(const T& run) {
        this->run = size_t(std::abs(run));
    }

    template <typename T> void set_channel_no(const T& channel_no) {
        this->channel_no = size_t(std::abs(channel_no));
    }

    double set_sample_rate(const double& sample_rate) {

        if (sample_rate < treat_as_null ) {
            std::string err_str = std::string("atss_file->") + __func__;
            err_str += ":: you can not have NULL negative frequencies";
            throw err_str;
        }
        else this->sample_rate = sample_rate;
        return this->sample_rate;
    }

    double get_sample_rate() const {
        return this->sample_rate;
    }

    std::string filename(const std::string& extension_wo_dot = "") const {

        if (this->sample_rate < this->treat_as_null) {
            std::string err_str = std::string("atss_file->") + __func__;
            err_str += "::you can not have NULL or negative frequencies";
            throw err_str;
            return std::string();
        }
        std::string str;
        str += mstr::zero_fill_field(this->serial, 3) + "_";
        str += this->system + "_";
        str += "C" + mstr::zero_fill_field(this->channel_no, 3) + "_";
        str += "R" + mstr::zero_fill_field(this->run, 3) + "_";
        str += "T" + this->channel_type + "_";
        double f_or_s;
        std::string unit;
        double diff_s = mstr::sample_rate_to_str(this->sample_rate, f_or_s, unit, true);
        // can we make an int?
        if ( ((floor(f_or_s) - f_or_s) < this->treat_as_null) && (diff_s < this->treat_as_null)) str += std::to_string(int(f_or_s)) + unit;
        else str += std::to_string(f_or_s) + unit;
        if (extension_wo_dot.size()) str += "." + extension_wo_dot;
        return str;
    }

private:
    // not content of JSON -  these are the elements of the filename, generated on the fly
    std::size_t serial = 0;             //!< such as 1234 (no negative numbers please) for the system
    std::string system = "";            //!< such as ADU-08e, XYZ (a manufacturer is not needed because the system indicates it)
    std::size_t channel_no = 0;         //!< channel number - you can integrate EAMP stations as channels if the have the SAME!! timings
    std::size_t run = 0;                //!< counts for same frequency at the same place - or a sclice
    std::string channel_type = "";      //!< type such as Ex, Ey, Hx, Hy, Hz or currents like Jx, Jy, Jz or Pitch, Roll, Yaw or x, y, z or T for temperature
    /*!
     * sample_rate contains sample_rate. Unit: Hz (samples per second) - "Hz" or "s" will be appended while writing in real time
     * the FILENAME contains a UNIT for better readability; you MUST have 256Hz (sample rate 256) OR 4s (sample rate 0.25);
     * a "." in the FILENAME is possible on modern systems, 16.6666Hz is possible
     */
    double sample_rate = 0.0;

protected:
    double treat_as_null = 1E-32;

};

class atss_header
{

public:

    /*!
     * \brief atss_header constructor
     * \param datetime ISO8601 like 1970-01-01T00:00:00 or 1970-01-01T00:00:00.234
     * \param fracs - helper in cas you have 1970-01-01T00:00:00 AND! 0.234
     */

    atss_header(const std::string &datetime = "1970-01-01T00:00:00", const double& fracs = 0.0) {
        this->set_date_time(datetime, fracs);

    }

    void set_date_time(const std::string &datetime = "1970-01-01T00:00:00", const double& fracs = 0.0) {
        if (fracs == 0.0) this->datetime = datetime;
        else this->datetime = mstr::iso_8601_str_d(datetime, fracs);
    }



    void set_lat_lon_elev(const double &lat, const double &lon, const double elev = 0.) {
        this->latitude = lat;
        if (lon > 180.0) this->longitude = lon - 360.0;
        else if (lon < -180.0) this->longitude = lon + 360.0;
        else  this->longitude = lon;
        this->elevation = elev;
    }


    // 2007-12-24T18:21:00.01234 is probably not supported by C/C++/Python/PHP and others COMPLETELY, eg. fraction of seconds is a problem
    std::string datetime = "1970-01-01T00:00:00"; //!< ISO 8601 in UTC - other timezones are not supported; convert!
    double latitude =  0.0;             //!< decimal degree such as 52.2443, ISO 6709, +/- 90
    double longitude =  0.0;            //!< decimal degree such as 10.5594, ISO 6709, +/- 180
    double elevation =  0.0;            //!< elevation in meter
    double angle =  0.0;                //!< orientaion from North to East (90 = East, -90 or 270 = West, 180 South, 0 North)
    double dip =  0.0;                  //!< angle positive down 90 = down, 0 = horizontal - in case it had been measured
    std::string units =  "mV";          //!< for ADUs it will be mV H or whatever or scaled E mV/km
    std::string source =  "";           //!< empty or indicate as, ns, ca, cp, tx or what ever

};


template <class T>
class channel: public atss_file, public atss_header {

public:
    channel(const std::string &channel_type = "", const double& sample_rate = 0,
            const std::string &datetime = "1970-01-01T00:00:00", const double& fracs = 0.0)
        : atss_file(channel_type, sample_rate), atss_header(datetime, fracs) {

        if (this->pt != nullptr) pt.reset();

        pt = std::make_shared<p_timer>(this->datetime, this->get_sample_rate(), fracs);
    }

    std::shared_ptr<p_timer> pt;

    void set_sample_rate(const double& sample_rate) {

        this->pt->sample_rate = atss_file::set_sample_rate(sample_rate);
    }


    /*!
     * \brief write_header
     * \param path_only full path including the measdir
     * \param jsn_cal the calibration part
     */
    std::filesystem::path write_header(const std::filesystem::path &path_only, const jsn &jsn_cal) const {
        jsn head;
        head["datetime"] = this->datetime;
        head["latitude"] =  this->latitude;
        head["longitude"] =  this->longitude;
        head["elevation"] =  this->elevation;
        head["angle"] = this->angle;
        head["dip"] =  this->dip;
        head["units"] = this->units;
        head["source"] = this->source;
        head.update(jsn_cal);

        std::filesystem::path filepath(path_only);
        filepath /= this->filename("json");
        std::ofstream file;
        file.open (filepath, std::fstream::out | std::fstream::trunc);

        if (!file.is_open()) {
            file.close();
            std::string err_str = __func__;
            err_str += "::can not write header, file not open ";
            err_str += filepath.string();
            throw err_str;
            return std::filesystem::path("");
        }

        file << std::setw(2) << head << std::endl;
        file.close();

        return filepath;

        //std::cout << std::setw(2) << head << std::endl;
    }

    bool prepare_write_atss(const std::filesystem::path &path_only, std::ofstream &file) {

        std::filesystem::path filepath(path_only);
        filepath /= this->filename("atss");
        file.open (filepath, std::ios::out | std::ios::trunc | std::ios::binary);

        if (!file.is_open()) {
            file.close();
            std::string err_str = __func__;
            err_str += "::file not open ";
            err_str += filepath.string();
            throw err_str;
            return false;
        }

        return file.is_open();
    }

    void write_bin(const std::vector<double> &data, std::ofstream &file) {

        for (auto dat : data) {
            file.write(static_cast<char *>(static_cast<void *>(&dat)), 8);
        }
    }

    bool prepare_read_atss(const std::filesystem::path &path_only, std::ifstream &file) {

        std::filesystem::path filepath(path_only);
        filepath /= this->filename("atss");
        bool ok = false;

        try {
            ok = std::filesystem::exists(filepath);
        }
        catch (std::filesystem::filesystem_error& e) {
            std::string err_str = __func__;
            std::cerr << err_str << " " << e.what() << std::endl;
            return false;
        }

        file.open(filepath, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            file.close();
            std::string err_str = __func__;
            err_str += "::can not open, file not open ";
            err_str += filepath.string();
            throw err_str;
            return false;
        }


        return ok;
    }

    size_t read_bin(std::vector<double> &data, std::ifstream &file, const bool read_last_chunk = false) {

        // too much checking ?
        if (file.peek() == EOF ) {
            data.resize(0);
            return 0;
        }

        size_t i = 0;
        while (!file.eof() && i < data.size()) {
            file.read(static_cast<char *>(static_cast<void *>(&data[i++])), 8);
        }

        // read last chunk in case
        if ((file.eof() ) && read_last_chunk && (i > 1) ) {
            --i;
            data.resize(i);  // keeps the elements; i was incremented BEFORE the while loop above terminated
        }
        else if ((i == 1) || !i) {
            data.resize(0);
        }

        return data.size();
    }




    size_t samples(const std::filesystem::path &path_only) const {

        uintmax_t xx;
        std::filesystem::path filepath(path_only);
        filepath /= this->filename("atss");

        try {
            xx = std::filesystem::file_size(filepath);
        }
        catch(std::filesystem::filesystem_error& e) {
            std::cerr << e.what() << std::endl;
            return 0;
        }

        this->pt->samples = size_t(xx/8);

        return size_t(xx/8);
    }




};





#endif // ATSS_H
