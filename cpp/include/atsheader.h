#ifndef ATSHEADER
#define ATSHEADER

#include <cstdint>
#include <cstdio>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <typeinfo>
#include <vector>

#include "atsheader_def.h"
#include "strings_etc.h"

namespace fs = std::filesystem;

/*
https://www.codingame.com/playgrounds/5659/c17-filesystem

cout << "exists() = " << fs::exists(pathToShow) << "\n"
     << "root_name() = " << pathToShow.root_name() << "\n"
     << "root_path() = " << pathToShow.root_path() << "\n"
     << "relative_path() = " << pathToShow.relative_path() << "\n"
     << "parent_path() = " << pathToShow.parent_path() << "\n"
     << "filename() = " << pathToShow.filename() << "\n"
     << "stem() = " << pathToShow.stem() << "\n"
     << "extension() = " << pathToShow.extension() << "\n";

Here's an output for a file path like "C:\Windows\system.ini":

    exists() = 1
    root_name() = C:
          root_path() = C:\
    relative_path() = Windows\system.ini
      parent_path() = C:\Windows
    filename() = system.ini
      stem() = system
    extension() = .ini

     int i = 0;
for (const auto& part : pathToShow)
    cout << "path part: " << i++ << " = " << part << "\n";


*/



    /*!
 * \brief The atsheader class is a class for reading & writing the BINARY struct ATSHeader_80 and ATSComments_80 <br>
 * It has bee seperated from the binary structs
 */
    class atsheader {

public:
    /*!
   * \brief atsheader constructor
   */
    atsheader() { ; }

    /*!
   * \brief atsheader
   * \param filename file to open for reading the header
   */
    atsheader(const fs::path &filename,  const bool close_after_read = true) {
        this->read(filename, close_after_read);
    }
    /*!
     * \brief atsheadercopy constructor
     * \param rhs
     */
    atsheader(const atsheader &rhs) {
        this->header = rhs.header;
        this->filename = rhs.filename;
    }

    /*!
     * \brief atsheader copy constructor
     * \param rhs
     */
    atsheader(const std::shared_ptr<atsheader> &rhs) {
        this->header = rhs->header;
        this->filename = rhs->filename;
    }

    void close() {
        if (this->file.is_open())
            this->file.close();
    }

    /*!
   * \brief ~atsheader closes file in case
   */
    ~atsheader() {
        if (this->file.is_open())
            this->file.close();
    }


    ATSHeader_80 header;    //!< the binary header of 1024 bytes
    size_t write_count = 0; //!< count samples written to file, needed when finally re-write the header

    bool read(const fs::path &filename = "", const bool close_after_read = false) {
        if (filename.empty() && this->filename.empty())
            return false;
        else if (!filename.empty())
            this->filename = filename;
        if (this->filename.empty())
            return false;
        this->file.open(this->filename, std::ios::in | std::ios::binary);
        if (this->file.is_open()) {
            this->file.read((char *)&this->header, sizeof(this->header));
        }
        if (close_after_read)
            this->file.close();
        return true;
    }

    bool write(const bool close_after_write = false) {
        if (!sizeof(this->filename.filename()))
            return false;
        if (this->file.is_open())
            this->file.close();

        this->file.open(filename, std::ios::out | std::ios::binary);
        if (this->file.is_open()) {
            this->file.write((char *)&this->header, sizeof(this->header));
        } else {
            this->file.close();
            std::string err_str = __func__;
            err_str += ":: can not WRITE HEADER!";
            throw err_str;
            return false;
        }
        if (close_after_write)
            this->file.close();
        return true;
    }

    std::string gen_xmlfilename() const {

        // 295_2019-11-20_06-52-49_2019-11-27_07-22-49_R000_4H.xml
        std::string xmlfile;

        if (this->filename.empty()) {
            std::string err_str = __func__;
            err_str += ":: no ats filename!";
            throw err_str;
            return xmlfile;
        }

        std::string sad = this->start_date();
        std::string sat = this->start_time();

        std::string sod = this->stop_date();
        std::string sot = this->stop_time();

        std::string samp = mstr::sample_rate_to_str_simple(this->header.sample_rate);

        int irun = 0;
        auto tokens = mstr::split(this->filename.filename().string(), '_');
        for (auto &token : tokens) {
            if (token.starts_with('R') || token.starts_with('r')) {
                try{
                    auto rstr = token.substr(1);
                    irun = std::stoi(rstr);
                }
                catch  (...) {
                    irun = 0;
                }
            }
        }
        std::string run = mstr::zero_fill_field(irun,3);
        std::string serial = mstr::zero_fill_field(this->header.serial_number,3);



        std::replace(sat.begin(), sat.end(), ':', '-'); // replace all ':' to '-'
        std::replace(sot.begin(), sot.end(), ':', '-'); // replace all ':' to '-'

        xmlfile += serial;
        xmlfile += "_";
        xmlfile += sad;
        xmlfile += "_";
        xmlfile += sat;
        xmlfile += "_";
        xmlfile += sod;
        xmlfile += "_";
        xmlfile += sot;
        xmlfile += "_R";
        xmlfile += run;
        xmlfile += "_";
        xmlfile += samp;
        xmlfile += ".xml";


        return xmlfile;


    }

    bool re_write() {
        if (!sizeof(this->filename.filename()))
            return false;
        if (this->file.is_open())
            this->file.close();

        this->gen_xmlfilename();

        this->file.open(filename, std::ios::out | std::ios::in | std::ios::binary);
        if (this->file.is_open()) {
            this->file.write((char *)&this->header, sizeof(this->header));
        } else {
            this->file.close();
            std::string err_str = __func__;
            err_str += ":: can not RE-WRITE HEADER!";
            throw err_str;
            return false;
        }
        this->file.close();
        return true;
    }

    /*!
   * \brief ats_read_ints read native data from file without conversion; these are the ADC values templates are std::int32_t or double!
   * \param ints
   * \param close_after_read
   * \return
   */
    template <typename T>
    size_t ats_read_ints_doubles(std::vector<T> &ints_doubles) {
        if (!this->file.is_open()) {
            std::string err_str = __func__;
            err_str += ":: file not open!";
            ints_doubles.resize(0);
            throw err_str;
            return ints_doubles.size();
        }
        // whatever is was, reset ints to a 0 state
        if (!ints_doubles.size()) {
            std::string err_str = __func__;
            err_str += ":: ints size can't be ZERO - no chunk read";
            ints_doubles.resize(0);
            this->file.close();
            throw err_str;
            return ints_doubles.size();
        }
        auto pos_start = file.tellg();
        // next op will fail
        // typically when previous read reached EOF
        if (file.peek() == EOF || (std::istream::traits_type::eof() == pos_start)) {
            ints_doubles.resize(0);
            return 0;
        }

        if (pos_start < 1024) {
            std::string err_str = __func__;
            err_str += ":: we seem to be still INSIDE HEADER";
            ints_doubles.resize(0);
            this->file.close();
            throw err_str;
            return ints_doubles.size();
        }

        size_t i = 0;
        if (typeid(ints_doubles.at(0)) == typeid(int32_t)) {
            while (!this->file.eof() && (i < ints_doubles.size())) {
                this->file.read(static_cast<char *>(static_cast<void *>(&ints_doubles[i++])), 4);
            }
        }
        else if (typeid(ints_doubles.at(0)) == typeid(double)) {
            std::int32_t idata32;
            while (!this->file.eof() && (i < ints_doubles.size())) {
                this->file.read(static_cast<char *>(static_cast<void *>(&idata32)), 4);
                ints_doubles[i++] = this->header.lsbval * idata32;
            }
        }
        // i was incremented BEFORE the while loop above terminated with eof; last element is dirt
        if ((this->file.eof() ) && (i > 1)) {
            --i;
            ints_doubles.resize(i);  // keeps the elements;
        }
        else if ((i == 1) || !i) {
            ints_doubles.resize(0);
        }
        // success

        return ints_doubles.size();
    }

    template <typename T>
    size_t ats_write_ints_doubles(const double &lsbval, const std::vector<T> &ints_doubles, const bool close_after_write = false) {
        if (!this->file.is_open())
            return 0;
        if (!ints_doubles.size())
            return 0;

        if (typeid(ints_doubles.at(0)) == typeid(int32_t)) {
            if (lsbval == this->header.lsbval) {
                int32_t idata32;
                for (const auto &idata : ints_doubles) {
                    idata32 = idata;
                    this->file.write(static_cast<char *>(static_cast<void *>(&idata32)), 4);
                }
            } else {
                int32_t idata32;
                for (const auto &idata : ints_doubles) {
                    double val = idata * lsbval;
                    idata32 = int32_t(val / this->header.lsbval);
                    this->file.write(static_cast<char *>(static_cast<void *>(&idata32)), 4);
                }
            }
        }

        else if (typeid(ints_doubles.at(0)) == typeid(double)) {
            int32_t idata32;
            for (const auto &data : ints_doubles) {
                idata32 = int32_t(data / this->header.lsbval);
                this->file.write(static_cast<char *>(static_cast<void *>(&idata32)), 4);
            }
        }

        if (close_after_write)
            this->file.close();
        this->write_count += ints_doubles.size();
        return ints_doubles.size();
    }

    size_t ats_zero_ints(const size_t &n, const bool close_after_write = false) {
        if (!this->file.is_open())
            return 0;
        int32_t tint = 0;
        for (size_t i = 0; i < n; ++i) {
            this->file.write(static_cast<char *>(static_cast<void *>(&tint)), 4);
        }
        if (close_after_write)
            this->file.close();
        this->write_count += n;
        return n;
    }

    size_t ats_write_doubles(const std::vector<double> &doubles, const bool close_after_write = false) {
        if (!this->file.is_open())
            return 0;
        if (!doubles.size())
            return 0;
        try {
            int32_t idata32;
            for (const auto &data : doubles) {
                idata32 = int32_t(data / this->header.lsbval);
                this->file.write(static_cast<char *>(static_cast<void *>(&idata32)), 4);
            }
        } catch (std::system_error &e) {
            std::cerr << e.code().message() << std::endl;
            std::cerr << "failed writing data atsheader::" << __func__ << std::endl;
            this->file.close();
            return 0;
        }
        if (close_after_write)
            this->file.close();
        return doubles.size();
    }

    size_t write_ascii() {
        this->file.close();
        std::fstream afile;
        fs::path afilename;
        size_t sz = 0;
        afilename = this->filename.parent_path();
        afilename /= this->filename.stem();
        afilename += ".tsdata";
        afile.open(afilename, std::ios::out | std::ios::trunc);
        if (!afile.is_open()) {
            std::string err_str = __func__;
            err_str += ":: can not open ascii for writing " + afilename.string();
            throw err_str;
            return 0;
        }
        this->read();
        std::vector<double> ints_doubles(8192 * 64);
        size_t i = 0;
        while (this->ats_read_ints_doubles(ints_doubles)) {
            std::cout << i++ << std::endl;
            for (auto const &d : ints_doubles)
                afile << d << std::endl;
            sz += ints_doubles.size();
        }
        afile.close();
        return sz;
    }

    /*!
   * \brief path
   * \return path (and therewith filename)
   */
    fs::path path() const {
        return this->filename;
    }

    std::string site_name() const {
        return this->filename.parent_path().parent_path().filename();
    }

    void change_dir(const fs::path &new_dir) {
        fs::path fname = this->filename.filename();
        this->filename.clear();
        this->filename = new_dir;
        this->filename /= fname;
    }

    std::string start_date() const {
        int64_t utc = static_cast<int64_t>(this->header.start);
        return mstr::iso8601_time_t(utc, 1);
    }

    std::string stop_date() const {
        long double sf_lhs = static_cast<long double>(this->header.samples) / static_cast<long double>(this->header.sample_rate);
        sf_lhs +=  static_cast<long double>(this->header.start);
        double f = 0.0, intpart;
        f = modf(sf_lhs, &intpart);
        int64_t utc = static_cast<int64_t>(intpart);
        return mstr::iso8601_time_t(utc, 1);

    }

    std::string start_time() const {
        int64_t utc = static_cast<int64_t>(this->header.start);
        return mstr::iso8601_time_t(utc, 2);
    }

    std::string stop_time() const {
        long double sf_lhs = static_cast<long double>(this->header.samples) / static_cast<long double>(this->header.sample_rate);
        sf_lhs +=  static_cast<long double>(this->header.start);
        double f = 0.0, intpart;
        f = modf(sf_lhs, &intpart);
        int64_t utc = static_cast<int64_t>(intpart);
        return mstr::iso8601_time_t(utc, 2, f);
    }

private:
    std::fstream file;
    fs::path filename;

};

bool compare_ats_pos(const std::shared_ptr<atsheader> &lhs, const std::shared_ptr<atsheader> &rhs) {
    float limit = 0.01;
    if (std::abs(lhs->header.x1 - rhs->header.x1) > limit)
        return false;
    if (std::abs(lhs->header.y1 - rhs->header.y1) > limit)
        return false;
    if (std::abs(lhs->header.z1 - rhs->header.z1) > limit)
        return false;
    if (std::abs(lhs->header.x2 - rhs->header.x2) > limit)
        return false;
    if (std::abs(lhs->header.y2 - rhs->header.y2) > limit)
        return false;
    if (std::abs(lhs->header.z2 - rhs->header.z2) > limit)
        return false;
    return true;
}

bool compare_ats_lsb(const std::shared_ptr<atsheader> &lhs, const std::shared_ptr<atsheader> &rhs) {
    bool res = true;
    if ((lhs->header.lsbval < 0.) || (rhs->header.lsbval) < 0.)
        return false;
    double d = lhs->header.lsbval / rhs->header.lsbval;
    if (d < 1.0)
        d = 1.0 / d;
    if (d > (1.0 + 10E-5))
        res = false;
    return res;
}

auto compare_ats_sample_rate = [](const std::shared_ptr<atsheader> &lhs, const std::shared_ptr<atsheader> &rhs) -> bool {
    return (lhs->header.sample_rate == rhs->header.sample_rate);
};

auto compare_ats_sensor_type = [](const std::shared_ptr<atsheader> &lhs, const std::shared_ptr<atsheader> &rhs) -> bool {
    return (std::string(lhs->header.sensor_type) == std::string(rhs->header.sensor_type));
};

auto compare_ats_chopper = [](const std::shared_ptr<atsheader> &lhs, const std::shared_ptr<atsheader> &rhs) -> bool {
    return (lhs->header.chopper == rhs->header.chopper);
};

auto compare_ats_channel_type = [](const std::shared_ptr<atsheader> &lhs, const std::shared_ptr<atsheader> &rhs) -> bool {
    return (std::string(lhs->header.channel_type, sizeof(lhs->header.channel_type)) == std::string(rhs->header.channel_type, sizeof(rhs->header.channel_type)));
};

auto compare_ats_start = [](const std::shared_ptr<atsheader> &lhs, const std::shared_ptr<atsheader> &rhs) -> bool {
    return lhs->header.start < rhs->header.start;
};

/*!
 *  \brief compare_ats_stop compares stop times, which may NOT! be at a full second!
 */

auto compare_ats_stop = [](const std::shared_ptr<atsheader> &lhs, const std::shared_ptr<atsheader> &rhs) -> bool {
    long double sf_lhs = static_cast<long double>(lhs->header.samples) / static_cast<long double>(lhs->header.sample_rate);
    long double sf_rhs = static_cast<long double>(rhs->header.samples) / static_cast<long double>(rhs->header.sample_rate);

    return ((static_cast<long double>(lhs->header.start)) + sf_lhs) < (static_cast<long double>(rhs->header.start) + sf_lhs);
};

auto diff_time = [](const std::shared_ptr<atsheader> &lhs, const std::shared_ptr<atsheader> &rhs) -> int64_t {
    long double sf_lhs = static_cast<long double>(lhs->header.samples) / static_cast<long double>(lhs->header.sample_rate);
    long double sf_rhs = static_cast<long double>(rhs->header.start);
    sf_lhs = (static_cast<long double>(lhs->header.start)) + sf_lhs;

    return int64_t((sf_rhs - sf_lhs) * static_cast<long double>(lhs->header.sample_rate));
};

/*!
 * \brief ats_can_simple_cat AFTER sorting by date, check if we can cat as integers
 * \param
 */
auto ats_can_simple_cat = [](const std::shared_ptr<atsheader> &lhs, const std::shared_ptr<atsheader> &rhs) -> bool {
    if (!compare_ats_start(lhs, rhs))
        return false;
    if (!compare_ats_channel_type(lhs, rhs))
        return false;
    if (!compare_ats_sample_rate(lhs, rhs))
        return false;

    return true;
};

template <class T>
class comp_if_equal {
public:
    T lhsval;
    T rhsval;
    std::shared_ptr<atsheader> lhs;
    std::string what;
    comp_if_equal(const std::shared_ptr<atsheader> &lhs, const std::string &what) : lhs(lhs), what(what) {
        if (this->what == "lsbval")
            this->lhsval = this->lhs->header.lsbval;
    }
    bool operator()(const std::shared_ptr<atsheader> &rhs) {
        if (this->what == "lsbval")
            this->rhsval = rhs->header.lsbval;
        return (this->lhsval == this->rhsval);
    }
};

class Greater {
    double _than;
    std::string what;

public:
    Greater(double th, std::string wh) : _than(th), what(wh) {
    }
    bool operator()(std::shared_ptr<atsheader> &rhs) const {
        return rhs->header.lsbval > _than;
    }
};

/*!
 * \brief ats_channel_sort AFTER sorting by date you my want to sort by channel type
 * \param atsheaders
 */
void ats_channel_sort(std::vector<std::shared_ptr<atsheader>> &atsheaders) {
    std::vector<std::string> chtp = {"Ex", "Ey", "Hx", "Hy", "Hz", "Ez", "Jx", "Jy", "Jz", "x", "y", "z", "T", "t"};
    std::vector<std::shared_ptr<atsheader>> new_atsheaders;
    std::vector<size_t> idxs;
    for (const auto &chtpye : chtp) {
        size_t i = 0;
        for (auto &atsh : atsheaders) {
            std::string typ(atsh->header.channel_type, sizeof(atsh->header.channel_type));
            if (chtpye == typ) {
                new_atsheaders.push_back(atsh);
                idxs.emplace_back(i);
            }
            ++i;
        }
    }

    // remove erase items from vector
    std::sort(idxs.begin(), idxs.end());  // Make sure the container is sorted
    for (auto ix = idxs.rbegin(); ix != idxs.rend(); ++ix) {
        atsheaders.erase(atsheaders.begin() + *ix);
    }


    new_atsheaders.swap(atsheaders);
    // do we have unkown channels ?
    if (new_atsheaders.size()) {
         for (auto &atsh : atsheaders) {
            new_atsheaders.push_back(atsh);
        }
    }

}

#endif // ATSHEADER
