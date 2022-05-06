#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <../../include/json.h>
#include <../../include/atss.h>

#include "../../include/atss_time.h"

#include <chrono>

using namespace std;
using jsn = nlohmann::ordered_json;

int main()
{

    std::shared_ptr<channel<double>> channel_hx = std::make_shared<channel<double>>(" H_x "); // empty spaces and "_" will be removed

    channel_hx->d.resize(12);
    channel_hx->set_serial(124);
    channel_hx->set_system("ADU-08e");
    channel_hx->set_run(99);
    channel_hx->set_channel_no(2);

    bool force_sample_rate_valid(true);

    while(force_sample_rate_valid){
        try {
            cout << channel_hx->filename() << endl;
            cout << "this line will be executed ONLY if the sample frequency is valid" << endl<< endl<< endl<< endl;
            force_sample_rate_valid = false;
        }

        catch (const std::string &error) {

            std::cerr << endl << error << std::endl;
            std::cerr << endl << "in this demo I correct to 0.25 Hz sample rate" << std::endl;
            channel_hx->set_sample_rate(0.25);
            force_sample_rate_valid = true;
        }
    }

    bool force_date_valid(true);

    std::string sdate("1234");
    std::string stime("5678");
    double fracs = 0.4;

    while(force_date_valid){
        try {
            channel_hx->set_date_time(sdate, stime, fracs);
            cout << "this line will be executed ONLY if the date is valid" << endl;
            force_date_valid = false;
        }

        catch (const std::string &error) {

            std::cerr << endl << error << std::endl;
            std::cerr << endl << "in this demo I correct the start time" << std::endl;
            sdate = "2022-01-18";
            stime = "17:30:00";
            channel_hx->set_date_time(sdate, stime, fracs);
            force_date_valid = true;
        }
    }
    std::cout << "start date time " << channel_hx->get_date_time_str() << std::endl;
    std::cout << "Full ISO string " << channel_hx->get_date_time_str_iso() << std::endl;

    std::cout << "add 6 days" << std::endl;

    std::time_t tm = mstr::string_iso8601_time_t("2022-01-30T17:30:00", 0.0);
    struct tm* ptm = std::gmtime(&tm);
    ptm->tm_mday += 6;
    tm = std::mktime(ptm) - timezone;
    std::string a, b;
    double f;
    std::string result = mstr::time_t_iso8601_utc(tm,a,b,f);
    cout << result << std::endl;

    std::cout << "atss time examples" << std::endl;

    f = 0.0;
    std::time_t utc = mtime::string_iso8601_time_t("1970-01-01T00:00:00", f);
    std::string udate, utime;
    std::cout << "atss time starts at second " << utc << " for: " << mtime::time_t_iso8601_utc(utc, udate, utime, f) << std::endl;
    std::cout << "atss time fractions are zero: "<< f << std::endl;

    long tst = 1250774545; // 2009-08-20T13:22:25
    utc = (long)tst;       // use long
    std::cout << "atss time from utc time stamp: " << tst << " should be 2009-08-20T13:22:25" << std::endl;
    cout << "ats time: " << mtime::time_t_iso8601_utc(utc, udate, utime, f) << endl;
    std::cout << "atss time add a second " << endl;
    ++utc;
    cout << "ats time added second: " << mtime::time_t_iso8601_utc(utc, udate, utime, f) << endl;
    utc += 86400;
    cout << "ats time added day:    " << mtime::time_t_iso8601_utc(utc, udate, utime, f) << endl;
    utc = mtime::add_d_m_y(utc, 0, 1, 0);
    cout << "ats time added month:  " << mtime::time_t_iso8601_utc(utc, udate, utime, f) << endl;
    cout << "ats test again" << endl;
    utc = mtime::string_iso8601_time_t("2021-12-31T14:30:00");
    cout << "ats time: " << mtime::time_t_iso8601_utc(utc, udate, utime, f) << endl;
    utc = mtime::add_d_m_y(utc, 1, 0, 0);
    cout << "ats time added day:    " << mtime::time_t_iso8601_utc(utc, udate, utime, f) << endl;
    cout << "end..." << endl;
    return 0;
}


/*
using namespace std;
using jsn = nlohmann::ordered_json;


// STL style algo
template <class Iterator, class T>
void mult(Iterator first, Iterator last, const T &factor) {
    while (first != last) {
        *first++ *= factor;
    }
}

// vector sytle algo for doubles as an example
void adds(std::vector<double> &v, const double &addme) {
    for (auto &e : v) e += addme;
}


template <class T>
class atss {

public:

    std::vector<T> d;   // my data

    jsn head;
    atss(const string &name = "") {
        this->head = name;
    }
    ~atss() {
        cout << "atss deleted" << endl;
    }

    void clear() {
        d.clear();
        d.shrink_to_fit();
    }

    void link_data(std::shared_ptr<vector<T>> &data) {
        this->d = data;
    }


    void info(const size_t &first_elements = 0) const {
        if (d == nullptr) {
            cout << this->name <<  " atss data not initialized" << endl;
            return;
        }
        else {
            cout  << this->name << " atss data size " << d->size() << endl;
        }

        if ((first_elements > 0) && (first_elements < d->size())) {
            size_t i = 1;
            for (const auto &e : *this->d) {
                cout << e << " ";
                if (++i > first_elements) break;
            }
            cout << endl;
        }
    }


};



int main()
{
    std::shared_ptr<vector<double>> hx;

    atss<double> channel_hx("Hx");

    if (hx == nullptr) {
        cout << "data not initialized" << endl;
    }
    channel_hx.info();

    // init the external data vector
    hx = make_shared<vector<double>>();
    channel_hx.link_data(hx);

    hx->resize(5);
    for (auto &d : *hx) {
        d = 4.;
    }

    for (const auto &d : *hx) {
        cout << d << " ";
    }
    cout << endl;

    // use the channel
    mult(channel_hx.d->begin(), channel_hx.d->end(), 4.5);

    channel_hx.info(4);

    // use underlying data vector
    adds(*hx, 23.0);

    channel_hx.info(4);

    // status
    cout << "Hx raw status " << hx.use_count() << endl;
    channel_hx.clear();
    channel_hx.info(4);
    cout << "Hx raw status after clearing class " << hx.use_count() << endl;

    cout << "dive into a sub section" << endl;

    if (hx.use_count()) {

    }

    hx.reset();
    cout << "Hx raw status after myself " << hx.use_count() << endl;

    cout << "end..." << endl;
    return 0;
}

*/
