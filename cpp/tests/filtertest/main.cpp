#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "sqlite_handler.h"
#include "fir_filter.h"
#include <survey.h>
using namespace std;


int main()
{

    std::filesystem::path sqlfile("/usr/local/mthotel/data/filter.sql3");
    auto sql_info = std::make_unique<sqlite_handler>();
    std::vector<double> coeff;
    std::filesystem::path fileoutpath(std::filesystem::temp_directory_path() / "aa/filter");

    std::cout << "filter calc:" << std::endl;

    std::vector<size_t> coeffs {471, 71};
    std::vector<double> samples {1024, 64, 1.5, 1, 0.5, 0.55, 0.0625};

    for (const auto &cf : coeffs) {

        for (const auto &samp : samples) {
            size_t hlen = (cf -1) / 2;
            double delay_time = double(hlen) / samp; // time needed for 1/2 filter length
            // if it is 1.1 -> we need 2 seconds
            double shift_time =  (ceil(delay_time));

            double fill_in_time = shift_time - delay_time;

     ;

            std::cout  << mstr::sample_rate_to_str_simple(samp) << " fill: " << fill_in_time << "  delay: " << delay_time << "  (test: " << fill_in_time + delay_time << ")" << "  shift time: " << shift_time << std::endl;

            size_t samples_shift = size_t(fill_in_time * samp);
            size_t samples_delay = size_t( delay_time * samp);
            size_t samples_control = samples_shift + samples_delay;
            size_t samples_at_new_start_time = size_t(shift_time * samp);


            std::cout << mstr::sample_rate_to_str_simple(samp) << " fill: " << samples_shift  << "  delay: " << samples_delay << "  (test: " << samples_control << ")" << "  shift time: " << samples_at_new_start_time << std::endl;

            // test if new start time is a full second




            auto rem = fmod((double(samples_at_new_start_time)/samp), 1.0);

            std::cout << "-->f: " << samp << " len/2: " << cf/2 << ", time:"<< delay_time << " shift:" << shift_time << "s, rem: " << rem << std::endl;
            if (rem != 0.0 ) std::cout << "    ERROR ********************************************" << std::endl;
            std::cout << std::endl << std::endl;

        }

        std::cout << std::endl;        std::cout << "--------------------------------------------------------------------------" << std::endl;
        std::cout << std::endl;

    }


    string sql_query("SELECT * FROM mtx4;");
    try {
        coeff = sql_info->sqlite_vector_double(sqlfile, sql_query);
    }
    catch (const std::string &error) {
        std::cerr << error << std::endl;
        sql_info.reset();
    }

    size_t i = 1;
//    for (const auto v: coeff) {
//        std::cout << i++ << ": " << v << std::endl;
//    }
    std::filesystem::path home_dir(getenv("HOME"));
    auto survey = std::make_shared<survey_d>(home_dir.string() + "/devel/ats_data/Northern_Mining");
    auto station = survey->get_station("Sarıçam"); // that is a shared pointer from survey
    auto run = station->get_run(5);
    auto channels = run->get_channels();


    std::vector<std::shared_ptr<fir_filter>> filters;
    std::vector<std::shared_ptr<channel>> channels_filtered;

    for (auto &chan : channels) {
        std::cout << chan->get_datetime() << std::endl;
        chan->open_atss_read();
    }

    for (auto chan: channels) {
        filters.emplace_back(std::make_shared<fir_filter>());
        channels_filtered.emplace_back( filters.back()->set_filter(chan, "mtx4") );
        //std::cout << chan->skip_samples(300) << std::endl;
        // watch out that the filterd pt has already the new sample freq
        std::cout << chan->shift_to_read_time(channels_filtered.back()->pt) << std::endl;
    }

    // create directories recursive
    std::filesystem::create_directories(fileoutpath);
    for (auto &chan : channels_filtered) {
        std::cout << chan->get_datetime() << std::endl;
        chan->set_dir(fileoutpath);
        chan->write_header();
    }



    try {

    }
    catch (...) {

    }


    return (0);
}
