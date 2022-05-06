#ifndef CAL_BASE_H
#define CAL_BASE_H


#include <cstdint>
#include <climits>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <filesystem>
//
// that is now in cmake as add_compile_definitions( _USE_MATH_DEFINES _msvc )
// #define _USE_MATH_DEFINES // for C++ and MSVC
//
#include <cmath>
#include "mt_base.h"
#include "strings_etc.h"
#include "json.h"
#include "../xml/tinyxmlwriter/tinyxmlwriter.h"


struct calibration
{
    std::string sensor;
    std::uint64_t serial = 0;
    ChopperStatus chopper = ChopperStatus::off;
    std::string units_amplitude;
    std::string units_frequency;
    std::string units_phase;
    std::string date;
    std::string time;
    std::string Operator;           //!< the one made the calibration; UPPERCASE because operator is keyword in C++

    std::vector<double> f;
    std::vector<double> a;
    std::vector<double> p;
    CalibrationType ct = CalibrationType::nn;

    calibration() {
        this->clear();
    }

    calibration(const std::shared_ptr<calibration> &rhs) {

        this->sensor = rhs->sensor;
        this->serial = rhs->serial;
        this->chopper = rhs->chopper;
        this->units_amplitude = rhs->units_amplitude;
        this->units_frequency = rhs->units_frequency;
        this->units_phase = rhs->units_phase;
        this->date = rhs->date;
        this->time = rhs->time;
        this->Operator = rhs->Operator;

        this->f = rhs->f;
        this->a = rhs->a;
        this->p = rhs->p;
        this->ct = rhs->ct;
    }

    void clear() {
        this->sensor.clear();
        this->serial = 0;
        this->units_amplitude.clear();
        this->units_frequency.clear();
        this->units_frequency.clear();
        this->date.clear();
        this->time.clear();
        this->Operator.clear();
        this->chopper = ChopperStatus::off;
    }

    void set_format(const CalibrationType ct) {
        if (ct == CalibrationType::mtx_old) {
            this->units_amplitude = "V/(nT*Hz)";
            this->units_frequency = "Hz";
            this->units_phase = "degrees";
            this->date = "1970-01-01";
            this->time = "00:00:00";
            this->Operator = "mtx";
            this->ct = CalibrationType::mtx;
        }
        else if (ct == CalibrationType::mtx) {
            this->units_amplitude = "mV/nT";
            this->units_frequency = "Hz";
            this->units_phase = "degrees";
            this->date = "1970-01-01";
            this->time = "00:00:00";
            this->Operator = "mtx";
            this->ct = CalibrationType::mtx;

        }
    }

    bool is_empty() const {
        if (this->f.size()) return false;
        return true;
    }

    std::string chopper2string() const {
        if (this->chopper == ChopperStatus::on) return std::string("on");
        return std::string("off");
    }

    CalibrationType cal_type() const {
        if (this->units_amplitude == "mV/nT") return CalibrationType::mtx;
        else if (this->units_amplitude == "V/(nT*Hz)") return CalibrationType::mtx_old;
        return CalibrationType::nn;
    }

    /*!
     * \brief toJson_embedd
     * \return returns a nlohmann::ordered_json for embedding; do NOT use for files! For files sensor, serial and chopper are part of file name
     */
    nlohmann::ordered_json toJson_embedd() const {
        nlohmann::ordered_json head;                // use ordered because of readability (vectors last)
        // use other.update(head); to join
        head["sensor_calibration"]["sensor"] = this->sensor;
        head["sensor_calibration"]["serial"] = this->serial;
        head["sensor_calibration"]["chopper"] = int(this->chopper);
        head["sensor_calibration"]["units_amplitude"] = this->units_amplitude;
        head["sensor_calibration"]["units_frequency"] = this->units_frequency;
        head["sensor_calibration"]["units_phase"] = this->units_phase;
        head["sensor_calibration"]["date"] = this->date;
        head["sensor_calibration"]["time"] = this->time;
        head["sensor_calibration"]["Operator"] = this->Operator;

        head["sensor_calibration"]["f"] = f;
        head["sensor_calibration"]["a"] = a;
        head["sensor_calibration"]["p"] = p;

        return head;
    }

    /*!
     * \brief write_file write a json file with sensor, serial and chopper are part of file name; CONTENT hast NOT! these parts
     * hence that if e.g. sensor would be included inside the file, filename AND content must be altered in case - that is stupid
     * \param path_only
     * \return site of calibration frequencies
     */
    size_t write_file(const std::filesystem::path &path_only) const {

        std::filesystem::path filepath(path_only);
        std::string fname(this->sensor);
        fname += "_" + std::to_string(this->serial);
        if (this->chopper == ChopperStatus::on) fname += "_chopper_on.json";
        else if (this->chopper == ChopperStatus::off) fname += "_chopper_off.json";
        else fname += ".json";
        filepath /= fname;
        std::ofstream file;
        file.open (filepath, std::fstream::out | std::fstream::trunc);
        nlohmann::ordered_json head;                // use ordered because of readability (vectors last)
        // use other.update(head); to join


        head["sensor_calibration"]["chopper"] = int(this->chopper);
        head["sensor_calibration"]["units_amplitude"] = this->units_amplitude;
        head["sensor_calibration"]["units_frequency"] = this->units_frequency;
        head["sensor_calibration"]["units_phase"] = this->units_phase;
        head["sensor_calibration"]["date"] = this->date;
        head["sensor_calibration"]["time"] = this->time;
        head["sensor_calibration"]["Operator"] = this->Operator;

        head["sensor_calibration"]["f"] = f;
        head["sensor_calibration"]["a"] = a;
        head["sensor_calibration"]["p"] = p;


        // std::cout << std::setw(2) << head << std::endl; // debug only

        if (!file.is_open()) {
            file.close();
            return 0;
        }

        file << std::setw(2) << head << std::endl;
        file.close();

        return this->f.size();
    }

    /*!
     * \brief extract_from_filename .. does NOT call clear (again)
     * \param filepath
     * \return
     */
    int extract_from_filename(const std::filesystem::path &filepath) {
        std::filesystem::path name(filepath.filename());
        name.replace_extension("");
        std::string calfilename = name.string();
        std::string tmpcalfilename(calfilename);

        std::transform(tmpcalfilename.begin(), tmpcalfilename.end(), tmpcalfilename.begin(), ::tolower);
        bool has_chopname = false;
        if (mstr::ends_with(tmpcalfilename, "_chopper_off")) {
            has_chopname = true;
            this->chopper = ChopperStatus::off;
        }
        else if (mstr::ends_with(tmpcalfilename, "_chopper_on")) {
            has_chopname = true;
            this->chopper = ChopperStatus::on;
        }
        auto items = mstr::split(calfilename, '_');
        if ( !items.size() ) return 0;

        if (has_chopname) {
            if (items.size() >= 4) {
                items.resize(items.size() - 2);
            }
        }

        this->serial = std::stoi(items.back());
        items.pop_back();
        for (auto const &str : items) {
            this->sensor += str + "_";
        }
        this->sensor.pop_back();

        return 1;
    }

    /*!
     * \brief read_file JSON format
     * \param filepath from the extracted filename we generate type sensor serial chopper
     * \return
     */
    size_t read_file(const std::filesystem::path &filepath, const bool auto_convert = true) {
        this->clear();
        if (!std::filesystem::exists(filepath)) {
            std::string err_str = __func__;
            err_str += ":: file not found ->";
            err_str += std::filesystem::absolute(filepath).string();
            throw err_str;
            return 0;
        }

        if (!this->extract_from_filename(filepath)) {
            std::string err_str = __func__;
            err_str += ":: filename can not be parsed! ->";
            err_str += std::filesystem::absolute(filepath).string();
            throw err_str;
            return 0;
        }

        std::ifstream file;
        file.open (filepath, std::fstream::in);
        if (!file.is_open()) {
            file.close();
            return 0;
        }
        nlohmann::ordered_json head = nlohmann::ordered_json::parse(file);
        file.close();
        // std::cout << std::setw(2) << head << std::endl; // debug only

        this->units_amplitude = std::string(head["sensor_calibration"]["units_amplitude"]);
        this->units_frequency = std::string(head["sensor_calibration"]["units_frequency"]);
        this->units_phase = std::string(head["sensor_calibration"]["units_phase"]);
        this->date = std::string(head["sensor_calibration"]["date"]);
        this->time = std::string(head["sensor_calibration"]["time"]);
        this->Operator = std::string(head["sensor_calibration"]["Operator"]);

        this->f = std::vector<double>(head["sensor_calibration"]["f"]);
        this->a = std::vector<double>(head["sensor_calibration"]["a"]);
        this->p = std::vector<double>(head["sensor_calibration"]["p"]);

        if ((f.size() != a.size()) || (f.size() != p.size())) {
            this->clear();
            std::string err_str = __func__;
            err_str += ":: calibration vecors f,a, are inconsistent ->";
            err_str += std::filesystem::absolute(filepath).string();
            throw err_str;
            return 0;
        }

        bool go_mtx_f = false;
        bool go_mtx_p = false;
        if (this->units_frequency == "Hz" ) go_mtx_f = true;
        if (this->units_phase == "degrees" ) go_mtx_p = true;


        if ( (this->units_amplitude == "V/(nT*Hz)") && go_mtx_f && go_mtx_p ) {

            this->ct = CalibrationType::mtx_old;
        }

        if ( (this->units_amplitude == "mV/nT") && go_mtx_f && go_mtx_p ) {

            this->ct = CalibrationType::mtx;
        }

        if (auto_convert) {
            if (this->ct == CalibrationType::mtx_old) {
                auto fi = this->f.cbegin();
                for (auto &v : this->a) {
                    v *= 1000.0 * *fi++;
                }
            }
        }

        return this->f.size();

    }

    void add_to_xml_1_of_3(std::shared_ptr<tinyxmlwriter> &tix) const {
        tix->push("calibration");

        tix->element("cal_version", "1.0");
        tix->element("creator", "cal_base");
        tix->element("user", "metronix");

        tix->push("calibrated_item");

        tix->element("ci", this->sensor);
        tix->element("ci_serial_number", this->serial);
        tix->element_empty("ci_revision");
        tix->element("ci_date", this->date);
        tix->element("ci_time", this->time);
        tix->element_empty("ci_calibration_valid_until");
        tix->element_empty("ci_next_calibration");
        tix->element_empty("ci_tag");
        tix->element_empty("ci_owner");
        tix->element_empty("ci_owners_address");
        tix->element("ci_manufacturer", "metronix");
        tix->element("ci_manufacturers_address", "Kocher Str. 3, 38120 Braunschweig, Germany");


        tix->pop("calibrated_item");

    }


    void add_to_xml_2_of_3(std::shared_ptr<tinyxmlwriter> &tix) const {


        for (size_t i = 0; i < this->f.size(); ++i) {
            auto chop = this->chopper2string();
            tix->push_caldata(static_cast<int>(this->chopper), 0, 0, 0, std::complex<double>(0,0));
            tix->element_attr("c0", "unit", "V", 0.0, 6, true);
            tix->element_attr("c1", "unit", this->units_frequency, this->f.at(i), 6, true);
            tix->element_attr("c2", "unit", this->units_amplitude, this->a.at(i), 6, true);
            if (this->units_phase == "degrees") tix->element_attr("c3", "unit", "deg", this->p.at(i), 6, true);
            else tix->element_attr("c3", "unit", this->units_phase, this->p.at(i), 6, true);
            tix->pop("caldata");

        }

    }


    void add_to_xml_3_of_3(std::shared_ptr<tinyxmlwriter> &tix) const {

        tix->pop("calibration");
    }



};  // end calibration

bool operator == (const std::shared_ptr<calibration>& lhs, const std::shared_ptr<calibration>& rhs) {

    //


    if (lhs->sensor != rhs->sensor) return false;
    if (lhs->chopper != rhs->chopper) return false;
    if (lhs->units_amplitude != rhs->units_amplitude) return false;
    if (lhs->units_frequency != rhs->units_frequency) return false;
    if (lhs->units_phase != rhs->units_phase) return false;
    if (lhs->date != rhs->date) return false;
    if (lhs->time != rhs->time) return false;
    if (lhs->Operator != rhs->Operator) return false;
    if (lhs->ct != rhs->ct) return false;

    if (lhs->f != rhs->f) return false;
    if (lhs->a != rhs->a) return false;
    if (lhs->p != rhs->p) return false;

    return true;
}

#endif
