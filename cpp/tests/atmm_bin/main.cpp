#include <iostream>
#include <fstream>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <complex>
#include <chrono>
#include <filesystem>
#include <algorithm>
#include <bitset>
#include <cstddef>
#include <bitset>

#include "atss.h"






std::ostream& operator << (std::ostream& os, std::byte b) {
    return os << std::bitset<8>(std::to_integer<int>(b));
}

//std::istream& operator >> (std::istream& is, std::bitset<8> &b) {

//    char ch;
//    is >> ch;
//    b = std::bitset<8>(ch);
//    return is;
//}

void print(const std::byte &b) {
    std::cout << std::bitset<8>(std::to_integer<int>(b)) << std::endl;
}





int main()
{


    auto atmmfile = std::make_shared<atmm>();


    std::vector<bool> a;
    a.push_back(true);
    a.push_back(true);
    a.push_back(true);
    a.push_back(false);
    a.push_back(true);
    a.push_back(true);
    a.push_back(true);
    a.push_back(false);

    // include #9
    a.push_back(0);

    std::vector<bool> b;



    std::byte by{0};

    std::byte result{0};
    uint8_t one(1);
    char outp;

    for(uint8_t i = 0; i < 8; i++) {
        if(a[i]) {
            result = (result | std::byte{uint8_t(one << i)});
        }
    }
    outp = static_cast<char>(result);

    std::cout << result << "  " << outp << "  " << int(result) << std::endl;

    std::cout << "print: ";
    print(result);

    std::bitset<8> bsi(outp);

    std::cout << bsi << std::endl;

//    for(uint8_t i = 0; i < 8; i++) {
//        if(bsi[i]) {
//            b.push_back(true);
//        }
//        else {
//            b.push_back(false);
//        }

//    }


    std::ifstream infile;
    std::ofstream outfile;

    std::filesystem::path filepath(std::filesystem::temp_directory_path() / "test.atmm");

    outfile.open (filepath, std::ios::out | std::ios::trunc | std::ios::binary);

    if (outfile.is_open()) {
        outfile << a;
    }
    else {
        std::cout << "failed";
    }
    outfile.close();

    infile.open (filepath, std::ios::in | std::ios::binary);

    uintmax_t atmms =  0;

    try {
        atmms = std::filesystem::file_size(filepath);
        std::cout << "expecting max selections: " << atmms * 8 << std::endl;
        b.reserve(atmms * 8);
    }
    catch(std::filesystem::filesystem_error& e) {
        std::cerr << e.what() << std::endl;
        return 0;
    }


    if (infile.is_open()) {
        infile >> b;
    }
    else {
        std::cout << "failed";
    }
    infile.close();
    //b.resize(a.size());


    for (size_t i = 0; i < b.size(); ++i) {
        //std::cout << a.at(i) << " " << b.at(i) << std::endl;
        std::cout << i << " " << b.at(i) << std::endl;

    }


    return 0;
}
