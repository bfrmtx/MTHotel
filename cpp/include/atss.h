#ifndef ATSS_H
#define ATSS_H

#include <iostream>
#include <vector>
#include <string>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include "json.h"
#include "cal_base.h"
#include "strings_etc.h"
#include <filesystem>


using jsn = nlohmann::ordered_json;





/*!
 * \brief The p_timer class allows date functions with fractions of seconds
 * remind: 12:00:00.0001 is maybe a bad start time! when ever possible, shift your samples so that you get full seconds
 */

class p_timer {

public:

    /*!
     * \brief p_timer empty constructor
     */
    p_timer() {;}

    /*!
     * \brief p_timer default constructor
     * \param datetime
     * \param sample_rate
     * \param fracs
     */
    p_timer(const std::string &datetime, const double &sample_rate, const double& fracs = 0.0) {
        this->set_datetime_sample_rate_fracs(datetime, sample_rate, fracs);
    }

    /*!
     * \brief set_datetime_sample_rate_fracs
     * \param datetime ISO string like 2009-08-20T13:22:01 NOT 2009-08-20T13:22:01.0034
     * \param sample_rate in Hz
     * \param fracs like 0.0034
     */
    void set_datetime_sample_rate_fracs(const std::string &datetime, const double &sample_rate, const double& fracs = 0.0) {
        this->tt = mstr::time_t_iso_8601_str(datetime);
        this->sample_rate = sample_rate;
        this->fracs = fracs;
        if (this->fracs < mstr::zero_frac) this->fracs = 0.0;
    }

    /*!
     * \brief p_timer copy constructor
     * \param rhs p_timer
     */
    p_timer(const p_timer &rhs) {
        this->tt = rhs.tt;
        this->fracs = rhs.fracs;
        this->sample_rate = rhs.sample_rate;
        this->samples = rhs.samples;
        this->spos = rhs.spos;
    }

    /*!
     * \brief p_timer copy constructor from a shared pointer
     * \param rhs
     */
    p_timer(const std::shared_ptr<p_timer> &rhs) {
        this->tt = rhs->tt;
        this->fracs = rhs->fracs;
        this->sample_rate = rhs->sample_rate;
        this->samples = rhs->samples;
        this->spos = rhs->spos;
    }

    void clear() {
        this->tt = 0;
        this->fracs = 0.0;
        this->sample_rate = 0.0;
        this->samples = 0;
        this->spos = 0;
    }


    p_timer operator = (const p_timer& rhs) {
        if (&rhs == this) return *this;
        this->tt = rhs.tt;
        this->fracs = rhs.fracs;
        this->sample_rate = rhs.sample_rate;
        this->samples = rhs.samples;
        this->spos = rhs.spos;
        return *this;
    }

    bool operator < (const p_timer& rhs) const {

        if (this->tt < rhs.tt) return true;      // second diff
        if (this->tt > rhs.tt) return false;     // second diff
        return (this->fracs < rhs.fracs);        // else compare sub samples

    }


    bool operator > (const p_timer& rhs) const {

        if (this->tt > rhs.tt) return true;      // second diff
        if (this->tt < rhs.tt) return false;     // second diff
        return (this->fracs > rhs.fracs);        // else compare sub samples

    }

    p_timer operator + (const p_timer &rhs) const {
        p_timer nt(*this);
        nt.tt += rhs.tt;
        double f = nt.fracs + rhs.fracs;
        if (f < 1.0) nt.fracs = f;
        else {
            double fullsecs;
            nt.fracs = modf (f, &fullsecs);
            nt.tt += (uint64_t) fullsecs;
        }
        return nt;
    }

    p_timer operator += (const p_timer &rhs)  {
        this->tt += rhs.tt;
        double f = this->fracs + rhs.fracs;
        if (f < 1.0) this->fracs = f;
        else {
            double fullsecs;
            this->fracs = modf (f, &fullsecs);
            this->tt += (uint64_t) fullsecs;
        }
        return *this;
    }


    p_timer operator - (const p_timer &rhs) const {
        p_timer nt(*this);
        nt.tt -= rhs.tt;
        double f = nt.fracs - rhs.fracs;
        if (f > 0.0) nt.fracs = f;
        else {
            double fullsecs;
            nt.fracs = modf (f, &fullsecs);
            nt.tt -= (uint64_t) fullsecs;
        }
        return nt;
    }


    p_timer operator -= (const p_timer &rhs)  {
        this->tt -= rhs.tt;
        double f = this->fracs - rhs.fracs;
        if (f > 0.0) this->fracs = f;
        else {
            double fullsecs;
            this->fracs = modf (f, &fullsecs);
            this->tt -= (uint64_t) fullsecs;
        }
        return *this;
    }


    bool operator == (const p_timer& rhs) const {
        return ((this->tt == rhs.tt) && (this->fracs == rhs.fracs) );
    }

    p_timer sample_time(const size_t nth_sample) const {
        p_timer nt(*this);
        if (nth_sample <= this->samples) nt.spos = nth_sample;
        else return p_timer();

        double fullsecs, tsseg = (double(nt.spos)) / this->sample_rate;
        nt.fracs = modf (tsseg, &fullsecs);
        nt.tt += (uint64_t) fullsecs;
        return nt;
    }


    p_timer p_start() const {
        return this->sample_time(0);
    }

    p_timer p_stop() const {
        return this->sample_time(this->samples);
    }

    void time_t_iso_8601_str_fracs(const std::string& datetime) {
        this->tt = mstr::time_t_iso_8601_str_fracs(datetime, this->fracs);
        if (this->fracs < mstr::zero_frac) this->fracs = 0.0;
    }


    std::string sample_datetime(const size_t &sample, const int iso_0_date_1_time_2 = 0) const {
        auto nt = this->sample_time(sample);
        return mstr::iso8601_time_t(nt.tt, iso_0_date_1_time_2, nt.fracs);
    }

    std::string stop_datetime(const int iso_0_date_1_time_2 = 0) const {
        auto nt = this->sample_time(this->samples);
        return mstr::iso8601_time_t(nt.tt, iso_0_date_1_time_2, nt.fracs);
    }

    std::string start_datetime(const int iso_0_date_1_time_2 = 0) const {
        auto nt = this->sample_time(0);
        return mstr::iso8601_time_t(nt.tt, iso_0_date_1_time_2, nt.fracs);
    }

    std::string brief() const {
        std::stringstream ss;
        ss << this->start_datetime() << " <-> " << this->stop_datetime() << " @" <<mstr::sample_rate_to_str_simple(this->sample_rate);
        auto dur = this->p_stop() - this->p_start();
        std::chrono::seconds sd(dur.tt);
        const auto days = duration_cast<std::chrono::days>(sd);
        const auto hrs = duration_cast<std::chrono::hours>(sd - days);
        const auto mins = duration_cast<std::chrono::minutes>(sd - days - hrs);
        const auto secs = duration_cast<std::chrono::seconds>(sd - days - hrs - mins);

        ss << " " << days.count() << "days " << hrs.count() << "hrs " << mins.count() <<  "mins " << secs.count() << "secs ";

        return ss.str();
    }

    /*!
     * \brief set_max_samples convinence function for tests ONLY, where you don't have a file or scanning
     * \param max_samples
     */
    void set_max_samples(const size_t max_samples) {
        this->samples = max_samples;
    }

    // 2007-12-24T18:21:00.01234 is probably not supported by C/C++/Python/PHP and others COMPLETELY, eg. fraction of seconds is a problem
    //std::string datetime = "1970-01-01T00:00:00"; //!< ISO 8601 in UTC - other timezones are not supported; convert!
    time_t tt = 0;              //!< seconds since 1970, can be negative in some implementaions
    double fracs = 0.0;         //!< fractions of seconds from 0 ...0.9999...
    double sample_rate = 0.0;   //!< in Hz always
    size_t samples = 0;         //!< total samples of the file
    size_t spos = 0;            //!< sample pos

};

// *****************************************************  C H A N N E L *********************************************************

/*!
 * \brief The atss_file class is the FILENAME part of the atss format consisting of binary .atss, JSON .json
 *  the tags of the filename are NOT repeated in the JSON
 */
class channel {

public:
    /*!
     * \brief channel most common mimimum constructor
     * \param channel_type like Ex, Ex, ... Hx ..
     * \param sample_rate in Hz always
     */
    channel(const std::string &channel_type, const double& sample_rate, const std::string &datetime = "1970-01-01T00:00:00", const double& fracs = 0.0) {
        this->set_channel_type(channel_type);
        this->set_sample_rate(sample_rate);
        this->set_datetime(datetime, fracs);
    }

    channel(const std::shared_ptr<channel> &rhs) {
        this->pt = rhs->pt;
        this->filepath_wo_ext = rhs->filepath_wo_ext;

        this->cal = std::make_shared<calibration>(rhs->cal);

        this->latitude = rhs->latitude;
        this->longitude = rhs->longitude;
        this->elevation = rhs->elevation;
        this->angle = rhs->angle;
        this->dip = rhs->dip;
        this->units = rhs->units;
        this->source = rhs->source;

        this->serial = rhs->serial;
        this->system = rhs->system;
        this->channel_no = rhs->channel_no;
        this->run = rhs->run;
        this->channel_type = rhs->channel_type;
    }

    /*!
     * \brief channel
     * \param in_json_file
     */
    channel(const std::filesystem::path &in_json_file) {

        std::filesystem::path json_file(in_json_file);
        if (!json_file.has_extension()) json_file.replace_extension(".json");

        if (this->parse_json_filename(json_file)) {
            std::ifstream file;
            file.open (json_file, std::fstream::in);
            if (!file.is_open()) {
                file.close();
                return;
            }
            nlohmann::ordered_json head = nlohmann::ordered_json::parse(file);
            file.close();
            if (head.contains("latitude") && head.contains("longitude") && head.contains("elevation")) {
                this->set_lat_lon_elev(head["latitude"], head["longitude"], head["elevation"]);
            }
            this->pt.time_t_iso_8601_str_fracs(head["datetime"]);
            this->filepath_wo_ext = json_file;
            this->filepath_wo_ext.replace_extension("");
            this->samples(this->filepath_wo_ext);


            if (head.contains("angle")) this->angle = head["angle"];
            if (head.contains("dip")) this->dip = head["dip"];
            if (head.contains("units")) this->units = head["units"];
            if (head.contains("source")) this->source = head["source"];

            if(this->cal != nullptr) this->cal.reset();
            this->cal = std::make_shared<calibration>();

            this->cal->parse_head(head, json_file);

        }

    }


    void set_datetime(const std::string &datetime = "1970-01-01T00:00:00", const double& fracs = 0.0) {

        this->pt.tt = mstr::time_t_iso_8601_str(datetime);
        this->pt.fracs = fracs;
    }

    std::string get_datetime() const {
        return this->pt.start_datetime();
    }

    std::string stop_datetime(const int iso_0_date_1_time_2 = 0) const {
        return this->pt.stop_datetime(iso_0_date_1_time_2);
    }

    std::string start_datetime(const int iso_0_date_1_time_2 = 0) const {
        return  this->pt.start_datetime(iso_0_date_1_time_2);
    }



    void set_base_file(const std::string& system = "", const int64_t& ser = 0,
                       const int64_t& channel_no = 0, const int64_t& run = 0) {
        this->set_system(system);
        this->set_serial(ser);
        this->set_channel_no(channel_no);
        this->set_run(run);
    }
    /*!
     * \brief parse_json_filename - put it in a try block
     * \param in_json_file
     * \return
     */
    bool parse_json_filename(const std::filesystem::path &in_json_file) {

        if (!std::filesystem::exists(in_json_file)) {
            std::string err_str = std::string("channel::") + __func__;
            err_str += "::file not exists " + in_json_file.string();
            throw err_str;
            return false;
        }

        std::stringstream ss(in_json_file.stem().string());
        std::string item;
        size_t i = 0;
        while(std::getline(ss, item, '_')) {
            if (i == 0) this->serial = size_t(std::abs(std::stol(item)));
            else if (i == 1) this->system = item;
            else if (i == 2) this->channel_no = size_t(std::abs(std::stol(item.substr(1))));
            else if (i == 3) this->run = size_t(std::abs(std::stol(item.substr(1))));
            else if (i == 4) this->channel_type = item.substr(1);
            else if (i == 5) this->pt.sample_rate = mstr::str_to_sample_rate(item);
            ++i;
        }

        if (i != 6) {
            std::string err_str = std::string("channel::") + __func__;
            err_str += "::error parsing filename " + in_json_file.string();
            this->pt.clear();
            throw err_str;
            return false;
        }


        this->filepath_wo_ext = in_json_file;
        this->filepath_wo_ext.replace_extension("");

        return true;

    }

    std::filesystem::path get_json_filepath() const {
        std::filesystem::path jsn(this->filepath_wo_ext.parent_path());
        return jsn /= this->filename(".json");
    }

    std::filesystem::path get_atss_filepath() const {
        std::filesystem::path atsf(this->filepath_wo_ext.parent_path());
        return atsf /= this->filename(".atss");
    }

    std::filesystem::path get_filepath_wo_ext() const {
        return this->filepath_wo_ext;
    }


    // simple set and get

    void set_serial(const std::integral auto& ser) {
        this->serial = size_t(std::abs(ser));
    }

    size_t get_serial() const {
        return this->serial;
    }

    void set_system(const std::string& system) {
        this->system = mstr::simplify(system, true);
    }

    std::string get_system() {
        return this->system;
    }


    void set_channel_type(const std::string& channel_type) {
        this->channel_type = mstr::simplify(channel_type, true);
    }

    std::string get_channel_type() const {
        return this->channel_type;
    }

    void set_run(const std::integral auto& run) {
        this->run = size_t(std::abs(run));
    }

    size_t get_run() const {
        return this->run;
    }

    void set_channel_no(const std::integral auto& channel_no) {
        this->channel_no = size_t(std::abs(channel_no));
    }

    size_t get_channel_no() const {
        return this->channel_no;
    }

    double set_sample_rate(const double& sample_rate) {

        if (sample_rate < treat_as_null ) {
            std::string err_str = std::string("channel->") + __func__;
            err_str += ":: you can not have NULL negative frequencies";
            throw err_str;
        }
        else this->pt.sample_rate = sample_rate;
        return this->pt.sample_rate;
    }

    double get_sample_rate() const {
        return this->pt.sample_rate;
    }

    /*!
     * \brief filename can be used to create files
     * \param extension_wo_dot
     * \return
     */
    std::string filename(const std::string& extension_with_dot = "") const {

        if (this->pt.sample_rate < this->treat_as_null) {
            std::string err_str = std::string("channel::") + __func__;
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
        double diff_s = mstr::sample_rate_to_str(this->pt.sample_rate, f_or_s, unit, true);
        // can we make an int?
        if ( ((floor(f_or_s) - f_or_s) < this->treat_as_null) && (diff_s < this->treat_as_null)) str += std::to_string(int(f_or_s)) + unit;
        else str += std::to_string(f_or_s) + unit;
        if (extension_with_dot.size()) str += extension_with_dot;
        return str;
    }


    std::filesystem::path filepath_wo_ext;
    p_timer pt;
    std::shared_ptr<calibration> cal;

    // JSON values + datetime from p_timer
    double latitude =  0.0;             //!< decimal degree such as 52.2443, ISO 6709, +/- 90
    double longitude =  0.0;            //!< decimal degree such as 10.5594, ISO 6709, +/- 180
    double elevation =  0.0;            //!< elevation in meter
    double angle =  0.0;                //!< orientaion from North to East (90 = East, -90 or 270 = West, 180 South, 0 North)
    double dip =  0.0;                  //!< angle positive down 90 = down, 0 = horizontal - in case it had been measured
    std::string units =  "mV";          //!< for ADUs it will be mV H or whatever or scaled E mV/km
    std::string source =  "";           //!< empty or indicate as, ns, ca, cp, tx or what ever

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

    double treat_as_null = 1E-32;


    /*!
     * \brief set_lat_lon_elev according to ISO 6709, +/- 90, +/- 180, elevation in meter
     * \param lat
     * \param lon
     * \param elev
     */
    void set_lat_lon_elev(const double &lat, const double &lon, const double elev = 0.) {
        this->latitude = lat;
        if (lon > 180.0) this->longitude = lon - 360.0;
        else if (lon < -180.0) this->longitude = lon + 360.0;
        else  this->longitude = lon;
        this->elevation = elev;
    }



    /*!
     * \brief write_header
     * \param directory_path_only full path including the measdir
     * \param jsn_cal the calibration part
     */
    std::filesystem::path write_header(const std::filesystem::path &directory_path_only, const jsn &jsn_cal) const {
        jsn head;
        head["datetime"] = this->start_datetime();
        head["latitude"] =  this->latitude;
        head["longitude"] =  this->longitude;
        head["elevation"] =  this->elevation;
        head["angle"] = this->angle;
        head["dip"] =  this->dip;
        head["units"] = this->units;
        head["source"] = this->source;
        head.update(jsn_cal);

        std::filesystem::path filepath(std::filesystem::canonical(directory_path_only));
        filepath /= this->filename(".json");
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

    bool prepare_write_atss(const std::filesystem::path &directory_path_only, std::ofstream &file) {

        std::filesystem::path filepath(std::filesystem::canonical(directory_path_only));
        filepath /= this->filename(".atss");
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

    bool prepare_read_atss(std::ifstream &file) {

        bool ok = false;

        try {
            ok = std::filesystem::exists(this->get_atss_filepath());
        }
        catch (std::filesystem::filesystem_error& e) {
            std::string err_str = __func__;
            std::cerr << err_str << " " << e.what() << std::endl;
            return false;
        }

        file.open(this->get_atss_filepath(), std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            file.close();
            std::string err_str = __func__;
            err_str += "::can not open, file not open ";
            err_str += this->get_atss_filepath().string();
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

    size_t samples(const std::filesystem::path &filepath_wo_ext = "")  {

        if (this->filepath_wo_ext.empty() && filepath_wo_ext.empty()) {
            std::string err_str = __func__;
            err_str += "::empty class file AND no argument given ";
            throw err_str;
        }
        std::filesystem::path filepath;
        if (!filepath_wo_ext.empty()) filepath = filepath_wo_ext;
        else filepath = this->filepath_wo_ext;
        filepath.replace_extension(".atss");

        uintmax_t xx =  0;


        try {
            xx = std::filesystem::file_size(filepath);
        }
        catch(std::filesystem::filesystem_error& e) {
            std::cerr << e.what() << std::endl;
            return 0;
        }

        this->pt.samples = size_t(xx/8);

        return this->pt.samples;
    }

    std::string brief() const {
        std::stringstream  ss;
        ss << this->pt.brief();
        if (this->cal != nullptr) ss << " " << cal->brief();

        return ss.str();
    }

};  // end channel class



auto compare_channel_start_lt = [](const std::shared_ptr<channel> &lhs, const std::shared_ptr<channel> &rhs) -> bool {
    return lhs->pt < rhs->pt;
};

auto compare_channel_start_eq = [](const std::shared_ptr<channel> &lhs, const std::shared_ptr<channel> &rhs) -> bool {
    // operator for pointer, * invoces the < operator wich is defined for a pointer rhs
    return lhs->pt == rhs->pt;
};

#endif // ATSS_H

/*
 *

bool operator < (const std::shared_ptr<p_timer>& rhs) const {

    if (this->tt < rhs->tt) return true;      // second diff
    if (this->tt > rhs->tt) return false;     // second diff
    return (this->fracs < rhs->fracs);        // else compare sub samples

}

auto compare_channel_start_lt = [](const std::shared_ptr<channel> &lhs, const std::shared_ptr<channel> &rhs) -> bool {
    // operator for pointer, * invoces the < operator wich is defined for a pointer rhs
    return *lhs->pt < rhs->pt;
};




*/
