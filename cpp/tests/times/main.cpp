#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <vector>
#include <unordered_map>
#include <thread>
#include "strings_etc.h"

#include "../include/bthread.h"





int main() {



    double out_fracs = 0.0;

    std::vector<std::string> ts;
    ts.emplace_back("2009-08-20T13:22:01.04"); // with frac
    ts.emplace_back("2009-08-20T13:22:01");    // w/o frac
    uint64_t n_2 = 1250774521;                 // that is the date above for [1]
    ts.emplace_back("1969-12-31T23:59:01");    // what time_t implementation we have?

    size_t i = 0;
    std::cout << std::endl;
    for (const auto &str: ts) {
        auto date = mstr::get_date_from_iso8601(str);
        auto time = mstr::get_time_from_iso8601(str);
        auto time_fracs = mstr::get_time_fracs_from_iso8601(str);
        double fracs = mstr::get_fracs_from_iso8601(str);
        auto r_1 = mstr::time_t_iso_8601_str(str);
        std::cout << "input: " << str << std::endl;
        std::cout << date << " " << time << " " << fracs << ", time with fracs " << time_fracs << std::endl;
        auto back = mstr::iso8601_time_t(r_1, 0, out_fracs);
        std::cout << "back conversion: " << back << std::endl;
        std::cout << "timestamp: " << r_1 << std::endl;
        if (i == 1) {
            std::cout << "original timestamp for example 1: " << n_2 << std::endl;
        }
        auto compare_date = std::gmtime(&r_1);
        std::string readable_time(asctime(compare_date));
        std::cout << "gmt time as string " <<  readable_time << std::endl;
        std::cout << std::endl;

        if (i == 1) {
            std::cout << "adding 120 s " << std::endl;
            r_1 += 120;
            std::cout << mstr::iso8601_time_t(r_1, 0, out_fracs) << std::endl;
        }

        ++i;
    }


    std::cout << std::endl;


    std::cout << "main finished " << std::endl;
    return 0;
}
