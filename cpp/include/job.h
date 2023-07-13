#ifndef JOB_H
#define JOB_H


#include <vector>
#include <iostream>
#include <iomanip>

#include "strings_etc.h"
#include "atsheader_def.h"
#include "json.h"

enum class sys_type : std::uint8_t {
    none   = 0,
    adu07e = 1,
    adu08e = 2,
    adu09  = 3,
    adu10e = 4,
    adu11e = 5,
    adu12e = 6

};

struct adu_channel {

public:



    time_t tt = 0;              //!< generates the ISO 8601 in UTC , as time_t seconds since 1970, can be negative in some implementaions
    double sample_rate = 512.0; //!< in Hz always
    size_t samples = 0;         //!< samples to record


};


class adu_job {

public:

    adu_job(const std::string &Name) {
        this->TypeNo = ats_sys_types.at(Name);
        size_t chans = 0;
        if (this->TypeNo == 0) chans = 5;       // ADU-07e
        if (this->TypeNo == 1) chans = 6;       // ADU-08e
        if (this->TypeNo == 4) chans = 5;       // ADU-10e
        if (this->TypeNo == 5) chans = 8;       // ADU-11e
        if (this->TypeNo == 6) chans = 13;      // ADU-12e

        for (size_t i = 0; i < chans; ++i) {
            this->channels.emplace_back(std::make_shared<adu_channel>());
        }

    }
    bool set_start_time(const std::string &datetime) {


        return true;
    }

    bool set_stop_time(const std::string &datetime) {


        return true;
    }

    bool set_duration(const std::string &days = "", const std::string &hours = "", const std::string &minutes = "", const std::string &seconds = "" ) {


        return true;
    }

    bool set_sample_rate(const std::string &sample_rate) {

        return true;
    }




private:

    std::vector<std::shared_ptr<adu_channel>> channels;
    int TypeNo = 0;
};

#endif // JOB_H
