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
    std::string datetime;
    std::string Operator;           //!< the one made the calibration; UPPERCASE because operator is keyword in C++

    std::vector<double> f;
    std::vector<double> a;
    std::vector<double> p;
    CalibrationType ct = CalibrationType::nn;

    calibration() {
        this->clear();
    }

    calibration(const std::string &sensor, const uint64_t &serial, const CalibrationType ct) : sensor(sensor), serial(serial) {

        this->set_format(ct, false);
    }

    std::string gen_json_filename_from_blank(const ChopperStatus &chopper) {
        std::string fname(this->sensor);
        fname += "_" + mstr::zero_fill_field(this->serial, 4);
        if (chopper == ChopperStatus::on) fname += "_chopper_on.json";
        else if (chopper == ChopperStatus::off) fname += "_chopper_off.json";
        else fname += ".json";

        return fname;
    }



    calibration(const std::shared_ptr<calibration> &rhs) {
        if (rhs != nullptr) {
            this->sensor = rhs->sensor;
            this->serial = rhs->serial;
            this->chopper = rhs->chopper;
            this->units_frequency = rhs->units_frequency;
            this->units_amplitude = rhs->units_amplitude;
            this->units_phase = rhs->units_phase;
            this->datetime = rhs->datetime;
            this->Operator = rhs->Operator;
            this->f = rhs->f;
            this->a = rhs->a;
            this->p = rhs->p;
            this->ct = rhs->ct;
        }
    }



    void clear() {
        this->sensor.clear();
        this->serial = 0;
        this->datetime.clear();
        this->Operator.clear();
        this->chopper = ChopperStatus::off;
        this->units_frequency = "unknown";
        this->units_amplitude = "unknown";
        this->units_phase = "unknown";
        this->ct = CalibrationType::nn;
    }

    void set_format(const CalibrationType ct, bool skip_date_time = true) {
        if (ct == CalibrationType::mtx_old) {
            this->units_frequency = "Hz";
            this->units_amplitude = "V/(nT*Hz)";
            this->units_phase = "degrees";
            if (!skip_date_time) {
                this->datetime = "1970-01-01T00:00:00";
            }
            this->Operator = "";
            this->ct = CalibrationType::mtx_old;
        }
        else if (ct == CalibrationType::mtx) {
            this->units_frequency = "Hz";
            this->units_amplitude = "mV/nT";
            this->units_phase = "degrees";
            if (!skip_date_time) {
                this->datetime = "1970-01-01T00:00:00";
            }
            this->Operator = "";
            this->ct = CalibrationType::mtx;

        }
        else if (ct == CalibrationType::nn) {
            this->units_frequency = "unknown";
            this->units_amplitude = "unknown";
            this->units_phase = "unknown";
            if (!skip_date_time) {
                this->datetime = "1970-01-01T00:00:00";
            }
            this->Operator = "";
            this->ct = CalibrationType::nn;

        }

    }

    size_t tasks_todo(const bool ampl_div_f = false, const bool ampl_mul_f = false, const bool ampl_mul_by_1000 = false,
                      const bool old_to_new = false, const bool new_to_old = false) {

        if (!this->f.size()) {
            std::string err_str = __func__;
            err_str += ":: no data! ->";
            throw err_str;
            return 0;
        }

        if ((this->f.size() != this->a.size()) || (this->f.size() != this->p.size())) {
            std::string err_str = __func__;
            err_str += ":: no data corrupted ->";
            throw err_str;
            return 0;
        }

        if (ampl_div_f) {
            for (size_t i = 0; i < f.size(); ++i) {
                this->a[i] /= this->f[i];
            }
        }

        if (ampl_mul_f) {
            for (size_t i = 0; i < f.size(); ++i) {
                this->a[i] *= this->f[i];
            }
        }
        if (ampl_mul_by_1000) {
            for (size_t i = 0; i < f.size(); ++i) {
                this->a[i] *= 1000.;
            }
        }

        if (old_to_new) {
            for (size_t i = 0; i < f.size(); ++i) {
                this->a[i] *= (1000. * this->f[i]);
            }
            this->set_format(CalibrationType::mtx);

        }

        if (new_to_old) {
            for (size_t i = 0; i < f.size(); ++i) {
                this->a[i] /= (1000. * this->f[i]);
            }
            this->set_format(CalibrationType::mtx_old);
        }

        return f.size();

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
        head["sensor_calibration"]["units_frequency"] = this->units_frequency;
        head["sensor_calibration"]["units_amplitude"] = this->units_amplitude;
        head["sensor_calibration"]["units_phase"] = this->units_phase;
        head["sensor_calibration"]["datetime"] = this->datetime;
        head["sensor_calibration"]["Operator"] = this->Operator;

        head["sensor_calibration"]["f"] = f;
        head["sensor_calibration"]["a"] = a;
        head["sensor_calibration"]["p"] = p;

        return head;
    }

    /*!
     * \brief write_file write a json file with sensor, serial and chopper are part of file name; CONTENT hast NOT! these parts
     * hence that if e.g. sensor would be included inside the file, filename AND content must be altered in case - that is stupid
     * \param directory_path_only
     * \return site of calibration frequencies
     */
    size_t write_file(const std::filesystem::path &directory_path_only) const {

        std::filesystem::path filepath(std::filesystem::canonical(directory_path_only));
        std::string fname(this->sensor);
        fname += "_" + mstr::zero_fill_field(this->serial, 4);
        if (this->chopper == ChopperStatus::on) fname += "_chopper_on.json";
        else if (this->chopper == ChopperStatus::off) fname += "_chopper_off.json";
        else fname += ".json";
        filepath /= fname;
        std::ofstream file;
        file.open (filepath, std::fstream::out | std::fstream::trunc);
        nlohmann::ordered_json head;                // use ordered because of readability (vectors last)
        // use other.update(head); to join

        head["sensor_calibration"]["sensor"] = this->sensor;
        head["sensor_calibration"]["serial"] = this->serial;
        head["sensor_calibration"]["chopper"] = int(this->chopper);
        head["sensor_calibration"]["units_frequency"] = this->units_frequency;
        head["sensor_calibration"]["units_amplitude"] = this->units_amplitude;
        head["sensor_calibration"]["units_phase"] = this->units_phase;
        head["sensor_calibration"]["datetime"] = this->datetime;
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

    size_t old_to_newformat() {
        bool go_mtx_f = false;
        bool go_mtx_p = false;
        if (this->units_frequency == "Hz" ) go_mtx_f = true;
        if (this->units_phase == "degrees" ) go_mtx_p = true;


        if ( (this->units_amplitude == "V/(nT*Hz)") && go_mtx_f && go_mtx_p ) {

            this->ct = CalibrationType::mtx_old;
        }

        if ( (this->units_amplitude == "mV/nT") && go_mtx_f && go_mtx_p ) {

            this->ct = CalibrationType::mtx;
            return this->f.size();
        }

        if (this->ct == CalibrationType::mtx_old) {
            auto fi = this->f.cbegin();
            for (auto &v : this->a) {
                v *= 1000.0 * *fi++;
            }
            this->set_format(CalibrationType::mtx, true);
            return this->f.size();
        }

        return 0;



    }

    /*!
     * \brief parse_head .. hence that for electrodes we may have data, serial, only EFP-06 or so
     * \return
     */
    size_t parse_head(const nlohmann::ordered_json &head, const std::filesystem::path &filepath = "") {

        this->sensor = std::string(head["sensor_calibration"]["sensor"]);
        this->serial = uint64_t(head["sensor_calibration"]["serial"]);
        int64_t ch = int64_t(head["sensor_calibration"]["chopper"]);
        if (ch == 1) this->chopper = ChopperStatus::on;
        else this->chopper = ChopperStatus::off;


        this->units_frequency = std::string(head["sensor_calibration"]["units_frequency"]);
        this->units_amplitude = std::string(head["sensor_calibration"]["units_amplitude"]);
        this->units_phase = std::string(head["sensor_calibration"]["units_phase"]);
        this->datetime = std::string(head["sensor_calibration"]["datetime"]);
        this->Operator = std::string(head["sensor_calibration"]["Operator"]);

        this->f = std::vector<double>(head["sensor_calibration"]["f"]);
        this->a = std::vector<double>(head["sensor_calibration"]["a"]);
        this->p = std::vector<double>(head["sensor_calibration"]["p"]);

        if ((this->f.size() != this->a.size()) || (this->f.size() != this->p.size())) {
            this->clear();
            std::string err_str = __func__;
            err_str += ":: calibration vecors f,a, are inconsistent ->";
            err_str += std::filesystem::absolute(filepath).string();
            throw err_str;
            return 0;
        }

        return this->f.size();
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

        this->parse_head(head, filepath);


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
                this->set_format(CalibrationType::mtx, true);
            }
        }

        return this->f.size();

    }

    void add_to_xml_1_of_3(std::shared_ptr<tinyxmlwriter> &tix, const std::string identifier = "") const {
        tix->push("calibration");

        tix->element("cal_version", "1.0");
        tix->element("creator", "cal_base");
        tix->element("user", "metronix");

        tix->push("calibrated_item");
        if (identifier.size()) {
            tix->element_attr("ci", "identifier", identifier, this->sensor);
        }
        else if (mstr::begins_with(this->sensor, "MFS")) {
            tix->element_attr("ci", "identifier", "coil", this->sensor);
        }
        else if (mstr::begins_with(this->sensor, "SHFT")) {
            tix->element_attr("ci", "identifier", "coil", this->sensor);
        }
        else if (mstr::begins_with(this->sensor, "FGS")) {
            tix->element_attr("ci", "identifier", "fluxgate", this->sensor);
        }
        else if (mstr::begins_with(this->sensor, "ADB")) {
            tix->element_attr("ci", "identifier", "board", this->sensor);
        }
        else tix->element("ci", this->sensor);
        tix->element("ci_serial_number", this->serial);
        tix->element_empty("ci_revision");
        tix->element("ci_date", this->datetime.substr(0,9));
        tix->element("ci_time", this->datetime.substr(10));
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

    std::filesystem::path mtx_cal_head(const std::filesystem::path &directory_path_only, bool create_filepath_only) const {

        if (this->ct == CalibrationType::nn) {
            std::string err_str = __func__;
            err_str += ":: calibration type is CalibrationType::nn  -> no idea what to do";
            throw err_str;
            return std::filesystem::path();
        }

        std::filesystem::path filepath(std::filesystem::canonical(directory_path_only));
        std::string fname(this->sensor);
        fname += "_" + (mstr::zero_fill_field(this->serial, 4) + ".txt");
        filepath /= fname;

        if (create_filepath_only) {
            return filepath;
        }

        std::ofstream file;

        file.open (filepath, std::fstream::out | std::fstream::trunc);

        if (!file.is_open()) {
            std::string err_str = __func__;
            err_str += ":: can not open ->";
            err_str += std::filesystem::absolute(filepath).string();
            throw err_str;
            return std::filesystem::path();
        }

        // ISO is a date hint for YY/mm/DD
        file << "Calibration measurement with ISO" << std::endl;


        file << "Magnetometer: " << this->sensor <<  " #" << std::to_string(this->serial);
        file << " DateTime: " << this->datetime << std::endl << std::endl;

        file.close();

        return filepath;



    }

    void mtx_cal_body(const std::filesystem::path &full_path_filename) const {

        std::ofstream file;
        file.open (full_path_filename, std::fstream::out | std::fstream::app);

        if (!file.is_open()) {
            std::string err_str = __func__;
            err_str += ":: can not open ->";
            err_str += std::filesystem::absolute(full_path_filename).string();
            throw err_str;
            return;
        }

        file << "FREQUENCY    MAGNITUDE    PHASE" << std::endl;
        file <<   "Hz           V/(nT*Hz)    deg" << std::endl;
        if (this->chopper == ChopperStatus::off) {
            file << "Chopper off" << std::endl;
        }
        if (this->chopper == ChopperStatus::on) {
            file << "Chopper on" << std::endl;
        }

        file.setf(std::ios::scientific, std::ios::floatfield);
        file.precision(8);

        for (size_t i = 0; i < this->f.size(); ++i) {
            file << this->f.at(i) << "  ";
            if (this->ct == CalibrationType::mtx_old) file << this->a.at(i) << "  ";
            if (this->ct == CalibrationType::mtx) file << (this->a.at(i) / (1000. * this->f[i])) << "  ";
            file << this->p.at(i) << std::endl;

        }
        file << std::endl;
        file.close();
    }

    std::string brief() const {
        std::stringstream ss;
        ss << "Sensor: " << this->sensor << " Serial: " << this->serial << " Entries: " << this->f.size();
        return ss.str();
    }


};  // end calibration

bool operator == (const std::shared_ptr<calibration>& lhs, const std::shared_ptr<calibration>& rhs) {

    // do I want to include the serial ... or check if just the data is the same?


    if (lhs->sensor != rhs->sensor) return false;
    if (lhs->serial != rhs->serial) return false;
    if (lhs->chopper != rhs->chopper) return false;
    if (lhs->units_amplitude != rhs->units_amplitude) return false;
    if (lhs->units_frequency != rhs->units_frequency) return false;
    if (lhs->units_phase != rhs->units_phase) return false;
    if (lhs->datetime != rhs->datetime) return false;
    if (lhs->Operator != rhs->Operator) return false;
    if (lhs->ct != rhs->ct) return false;

    if (lhs->f != rhs->f) return false;
    if (lhs->a != rhs->a) return false;
    if (lhs->p != rhs->p) return false;

    return true;
}

/*!
   compare a sensor - ignore the chopper
*/
auto compare_same_sensor = [](const std::shared_ptr<calibration> &lhs, const std::shared_ptr<calibration> &rhs) -> bool {
    if (lhs->sensor != rhs->sensor) return false;
    if (lhs->units_amplitude != rhs->units_amplitude) return false;
    if (lhs->units_frequency != rhs->units_frequency) return false;
    if (lhs->units_phase != rhs->units_phase) return false;
    if (lhs->serial != rhs->serial) return false;

    return true;
};

auto compare_sensor_and_chopper = [](const std::shared_ptr<calibration> &lhs, const std::shared_ptr<calibration> &rhs) -> bool {
    if (lhs->sensor != rhs->sensor) return false;
    if (lhs->serial != rhs->serial) return false;
    if (lhs->chopper != rhs->chopper) return false;
    return true;
};


#endif
