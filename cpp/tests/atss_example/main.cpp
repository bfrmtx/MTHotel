#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <../../include/json.h>
#include <../../include/atss.h>
#include "strings_etc.h"

#include <chrono>

using namespace std;
using jsn = nlohmann::ordered_json;

int main()
{

    std::shared_ptr<channel> channel_hx = std::make_shared<channel>(" H_x "); // empty spaces and "_" will be removed

    std::vector<double> d(12);
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
            channel_hx->set_datetime(sdate, fracs);
            cout << "this line will be executed ONLY if the date is valid" << endl;
            force_date_valid = false;
        }

        catch (const std::string &error) {

            std::cerr << endl << error << std::endl;
            std::cerr << endl << "in this demo I correct the start time" << std::endl;
            sdate = "2022-01-18";
            stime = "17:30:00";
            channel_hx->set_datetime((sdate + "T" + stime), fracs);
            force_date_valid = true;
        }
    }
    std::cout << "start date time " << mstr::get_date_from_iso8601(channel_hx->get_datetime()) << " " << mstr::get_time_fracs_from_iso8601(channel_hx->get_datetime()) << std::endl;
    std::cout << "Full ISO string " << channel_hx->get_datetime() << std::endl;

    std::cout << "add 6 days" << std::endl;
    double tfracs = 0.0;
    std::time_t tm = mstr::time_t_iso_8601_str("2022-01-30T17:30:00");
    struct tm* ptm = std::gmtime(&tm);
    ptm->tm_mday += 6;
    tm = std::mktime(ptm) - timezone;
    std::string a, b;
    double f = 0.0;
    std::string result = mstr::iso8601_time_t(tm);
    cout << result << std::endl;

    std::cout << "atss time examples" << std::endl;

    f = 0.0;
    std::time_t utc = mstr::time_t_iso_8601_str("1970-01-01T00:00:00");
    std::string udate, utime;
    std::cout << "atss time starts at second " << utc << " for: " << mstr::iso8601_time_t(utc) << std::endl;
    std::cout << "atss time fractions are zero: "<< f << std::endl;
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
