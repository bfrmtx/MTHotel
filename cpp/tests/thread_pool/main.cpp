#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <list>
#include <memory>
#include <vector>
#include <thread>

#include "../include/bthread.h"

void mytask(const size_t n) {
    std::cout << n << ": " << std::this_thread::get_id() << std::endl;
}

class int_thread
{
public:
    int_thread() {;}

    void show_thread_no(const size_t n) {
        std::cout << n << ": " << std::this_thread::get_id() << std::endl;
    }
};


int main() {

    size_t nthreads = 5;

    std::vector<size_t> nvals(14);

    size_t i = 0;
    for (auto &nv : nvals) nv = 70 + i++;

    std::vector<std::shared_ptr<int_thread>> ithrs;


    std::vector<size_t> execs = mk_mini_threads(0, nvals.size());

    std::cout << "possible threads: " << std::thread::hardware_concurrency() << std::endl;


   // std::cout << "executing main: " << execs[0] << "  trail:" << execs[1] << std::endl;

    i = 0;
    for (const auto &ex : execs) {
        try {
            std::vector<std::jthread> threads;
            std::cout << "starting: " << ex << std::endl;
            for (size_t j = 0; j < ex; ++j) {
                threads.emplace_back(std::jthread (mytask, std::ref(nvals.at(i++))));
            }

        }
        catch (...) {
            std::cerr << "could not execute all threads" << std::endl;
            return EXIT_FAILURE;
        }
    }


    for (i = 0; i < 12; ++i) {
        ithrs.emplace_back(std::make_shared<int_thread>());
    }

    i = 0;
    std::vector<std::jthread> threads;
    for (auto &ithr : ithrs) {
         std::cout << "starting class threads: " << i << std::endl;
        threads.emplace_back(std::jthread (&int_thread::show_thread_no, ithr, std::ref(i) ));
        ++i;
    }




    std::cout << "run finished - w/o exception" << std::endl;
    return 0;
}
