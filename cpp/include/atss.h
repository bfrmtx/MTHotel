#ifndef ATSS_H
#define ATSS_H

#include <iostream>
#include <vector>
#include <string>
#include <cstddef>
#include <cstdlib>
#include "json.h"
#include "strings_etc.h"

using jsn = nlohmann::ordered_json;

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

    void set_sample_rate(const double& sample_rate) {
        if (sample_rate < 0.0 ) {
            std::string err_str = std::string("atss_file->") + __func__;
            err_str += ":: you can not have NULL negative frequencies";
            throw err_str;
        }
        else this->sample_rate = sample_rate;
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

    atss_header(const std::string &date = "1970-01-01", const std::string &time = "00:00:00", const double& fracs = 0.0) {
        this->set_date_time(date, time, fracs);
    }

    void set_date_time(const std::string &date = "1970-01-01", const std::string &time = "00:00:00", const double& fracs = 0.0) {
        std::string iso_date = mstr::iso_8601_str_str(date, time, fracs);
        std::cout << iso_date << std::endl;
        //std::time_t tcmp = mstr::string_iso8601_time_t(iso_date, this->fracs);
        std::string tdate, ttime;
        std::string scmp = mstr::time_t_iso8601_utc(mstr::string_iso8601_time_t(iso_date, this->fracs), tdate, ttime, this->fracs);
        std::cout << scmp << std::endl;

        if (date.compare(tdate) || time.compare(ttime)) {
            this->fracs = 0.0;
            std::cerr << "failed to set start time" << std::endl;
            std::string err_str = std::string("atss_header->") + __func__;
            err_str += "::failed to set start time, in > " + date + " " + time + " out > " + tdate + " " + ttime;
            throw err_str;

        }
        this->date = date;
        this->time = time;

    }

    std::string get_date_time_str() const{
        return date + " " + time;
    }

    std::string get_date_time_str_iso() const{
        return mstr::iso_8601_str_str(this->date, this->time, this->fracs);
    }

private:
    // 2007-12-24T18:21:00.01234Z is NOT supported by C/C++/Python/PHP and others COMPLETELY
    std::string date =  "1970-01-01";   //!< ISO 8601 date 2021-05-19 UTC
    std::string time =  "00:00:00";     //!< ISO 8601 time in UTC
    double fracs =  0.0;                //!< factions of seconds, at your own risk. It is always the best to cut the data to full seconds
    double latitude =  0.0;             //!< decimal degree such as 52.2443
    double longitude =  0.0;            //!< decimal degree such as 10.5594
    double elevation =  0.0;            //!< elevation in meter
    double dipole_length =  0.0;        //!< length of dipole in meter
    double angle =  0.0;                //!< orientaion from North to East (90 = East, -90 or 270 = West, 180 South, 0 North)
    double dip =  0.0;                  //!< angle positive down - in case it had been measured
    std::string units =  "";            //!< for ADUs it will be mV (E or H) or scaled E mV/km
    std::string source =  "";           //!< empty or indicate as, ns, ca, cp, tx or what ever
    std::string site =  "";             //!< only use when you need it in your file name! leave empty!

};


template <class T>
class channel: public atss_file, public atss_header {

public:
    channel(const std::string &channel_type = "", const double& sample_rate = 0,
            const std::string &date = "1970-01-01", const std::string &time = "00:00:00", const double& frac = 0.0)
        : atss_file(channel_type, sample_rate), atss_header(date, time, frac) {

    }
    std::vector<T> d;   // share my data

    size_t size() const {
        return d.size();
    }


};





#endif // ATSS_H
