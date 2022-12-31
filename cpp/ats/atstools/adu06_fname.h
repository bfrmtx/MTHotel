#ifndef ADU06_FNAME_H
#define ADU06_FNAME_H

#include <string>
#include <filesystem>
#include <sstream>

class adu06_fname {
public:
    adu06_fname(const std::filesystem::path &name) {
        if (!std::filesystem::exists(name)) return;
        std::stringstream sstr;
        std::string tstr;
        std::string tname(name.filename());
        // nnn ser
        // c channel no A .. H -> 0 ..7
        // nn run
        // c channel type Ex .. Hz
        // 067A02AD.ats

        // serial number three digits
        sstr << tname.substr(0,3);
        sstr >> this->serial;
        sstr.clear();
        sstr.str("");

        // channel number one char
        tstr = tname.substr(3,1);
        for (auto & c: tstr) c = toupper(c);
        if (tstr == "A") this->channel_number =  0;
        if (tstr == "B") this->channel_number =  1;
        if (tstr == "C") this->channel_number =  2;
        if (tstr == "D") this->channel_number =  3;
        if (tstr == "E") this->channel_number =  4;
        if (tstr == "F") this->channel_number =  5;
        if (tstr == "G") this->channel_number =  6;
        if (tstr == "H") this->channel_number =  7;

        // run
        sstr << tname.substr(4,2);
        sstr >> this->run;
        sstr.clear();
        sstr.str("");

        // type
        tstr = tname.substr(6,1);
        for (auto & c: tstr) c = toupper(c);
        if (tstr == "A") this->channel_type =  "Ex";
        if (tstr == "B") this->channel_type =  "Ey";
        if (tstr == "X") this->channel_type =  "Hx";
        if (tstr == "Y") this->channel_type =  "Hy";
        if (tstr == "Z") this->channel_type =  "Hz";

        // band not needed - we take sample frequency from heade

        this->system = "ADU-06";


    }

    ~adu06_fname() {;}

    std::size_t serial = 0;             //!< such as 1234 (no negative numbers please) for the system
    std::string system = "";            //!< such as ADU-08e, XYZ (a manufacturer is not needed because the system indicates it)
    std::size_t channel_number = 0;     //!< channel number - you can integrate EAMP stations as channels if the have the SAME!! timings
    std::size_t run = 0;                //!< counts for same frequency at the same place - or a sclice
    std::string channel_type = "";      //!< type such as Ex, Ey, Hx, Hy, Hz or currents like Jx, Jy, Jz or Pitch, Roll, Yaw or x, y, z or T for temperature

};




#endif // ADU06_FNAME_H
