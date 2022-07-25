#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <vector>
#include <unordered_map>
#include <thread>

#include "../include/bthread.h"


std::unordered_map<std::string, size_t> seat_reservation(const std::vector<std::string> &names, size_t seats) {

    //seats we use it to force an error ans see how the function and main behaves
    std::unordered_map<std::string, size_t> reservation;

    std::cout << "reservation" << std::endl;


    try {

        if (names.size() > seats) {
            std::string err_str = __func__;
            err_str += "::can not make reservation";
            throw err_str;
        }
        // so seat ok, continue
        size_t i = 0;
        for (const auto &name : names) {
            reservation.emplace(name, i++);
        }

    }
    catch (const std::string &error) {
        std::cerr << error << std::endl;
        std::cerr << "making a fake reservation" << std::endl;

        // so this code will be executed after throw
        // reserve what I have - but I have to ask for names later
        for (size_t i = 0; i < seats; ++i) {
            reservation.emplace("reserved_seat_" + std::to_string(i), i);
        }
    }
    // that would catch others
    catch (...) {

    }

    // that is finally returned
    return reservation;
}


int main() {

    std::vector<std::string> persons;
    std::vector<size_t> seats {3,2};

    persons.emplace_back("Billy");
    persons.emplace_back("Ted");
    persons.emplace_back("Scooter");


    std::unordered_map<std::string, size_t> result;

    for (const auto &seat : seats) {

        try {
            std::cout << "starting reservation MAIN: " << std::endl;
            result = seat_reservation(persons, seat);
            if (!result.size()) {
                std::string err_str = __func__;
                err_str += ":: can not make reservation - msg from MAIN";
                throw err_str;
            }
            else {
                if (result.size() == persons.size()) std::cout << "MAIN reservation successful" << std::endl;
                else  std::cout << "MAIN reservation PROBLEM" << std::endl;
                for (const auto &p : result) {
                    std::cout << p.first << " " << p.second << std::endl;
                }
            }


        }
        catch( const std::string &error ) {
            std::cerr << error <<std::endl;
            std::cerr << "MAIN could not execute reservation in MAIN" << std::endl;

        }
        std::cout << std::endl;

        result.clear();
    }

    std::cout << std::endl;


    std::cout << "main finished " << std::endl;
    return 0;
}
