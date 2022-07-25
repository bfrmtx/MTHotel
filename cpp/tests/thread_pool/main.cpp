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



int main() {

    size_t nthreads = 5;

    std::vector<size_t> nvals(14);

    size_t i = 0;
    for (auto &nv : nvals) nv = 70 + i++;


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

    std::cout << "run finished - w/o exception" << std::endl;
    return 0;
}
