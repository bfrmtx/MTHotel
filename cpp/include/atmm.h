#ifndef ATMM_H
#define ATMM_H

#include <filesystem>
#include <vector>
#include <fstream>
#include <iostream>
#include <bitset>

/*!
 * \brief ostream operator << takes a vector of bool and writes one char for each bool
 * \param os
 * \param vb in case of 9 bool, 2 chars are writen, last 7 bools are true == excluded from processing!
 * \return
 */
std::ostream& operator << (std::ostream& os, const std::vector<bool> &vb) {
    std::byte result{0};
    uint8_t one(1);
    size_t i;
    uint8_t j;
    for (i = 0; i < vb.size() / 8; ++i) {
        for(j = 0; j < 8; ++j) {
            if(vb[i*8 + j]) {
                result = (result | std::byte{uint8_t(one << j)});
            }
        }
        os << static_cast<char>(result);
        result = std::byte{0};
    }
    if (vb.size() % 8) {
        result = std::byte{0};
        for(j = 0; j < vb.size() % 8; j++) {
            if(vb[i*8 + j]) {
                result = (result | std::byte{uint8_t(one << j)});
            }
        }
        for (; j < 8; ++j ) {
            result = (result | std::byte{uint8_t(one << j)});
        }
        os << static_cast<char>(result);
    }
    return os;
}

// try use reserve!, should resize as best option!
/*!
 * \brief istream operator >> reads a binary file as chars; converts to vector of bits
 * \param is
 * \param vb in case 9 bits have been written earlier, 2 chars are on the disk -> vector contains 16 bits
 * \return
 */
std::istream& operator >> (std::istream& is, std::vector<bool> &vb) {

    char ch;
    std::bitset<8> bsi;
    while (!is.eof()) {
        is >> ch;
        if (is.eof()) return is;
        bsi = std::bitset<8>(ch);
        for(uint8_t i = 0; i < 8; i++) {
            vb.emplace_back(bsi[i]);
        }
    }
    return is;
}



/*!
 * \brief The atmm class represents a selection; 0 included, 1 excluded
 */

class atmm
{
public:
    atmm() {
    }

    void read_data(std::vector<bool> &data, const std::filesystem::path &atmmfile, const size_t n) {
        // check json
        // div 8 , at least plus one
        // read char
        // make 8 bits

        std::ifstream file;
        std::filesystem::path atmm_file(atmmfile);
        atmm_file.replace_extension(".atmm");
        try {
            if (std::filesystem::is_regular_file(atmm_file)) {
                file.open (atmm_file, std::ios::in | std::ios::binary);
            }
        }
        catch (std::filesystem::filesystem_error& e) {
            std::string err_str = __func__;
            std::cerr << err_str << " " << e.what() << std::endl;
            return;
        }

        data.reserve(n + 8);

        if (file.is_open()) {
            file >> data;
        }
        else {
            std::cout << "failed";
        }
        file.close();

        data.resize(n);

    }


    void write_data(const std::vector<bool> &data, const std::filesystem::path &atmmfile) {

        std::ofstream file;
        std::filesystem::path atmm_file(atmmfile);
        atmm_file.replace_extension(".atmm");

        try {
            if (std::filesystem::is_regular_file(atmm_file)) {
                file.open (atmm_file, std::ios::out | std::ios::trunc | std::ios::binary);
            }
        }
        catch (std::filesystem::filesystem_error& e) {
            std::string err_str = __func__;
            std::cerr << err_str << " " << e.what() << std::endl;
            return;
        }


        if (file.is_open()) {
            file << data;
        }
        else {
            std::cout << "failed";
        }
        file.close();

    }

//    size_t vchar_size(const size_t n) const {

//    }

    ~atmm() {}

    //    std::ostream& operator<< (std::ostream& os, std::byte b) {
    //        return os << std::bitset<8>(std::to_integer<int>(b));
    //    }

};

/*
 *
 *
std::vector<bool> a;
a.push_back(true);
a.push_back(false);
//...
for (auto it = a.begin(); it != a.end();) // see 0x for meaning of auto
{
    unsigned b = 0;
    for (int i = 0; i < 8*sizeof(b); ++i)
    {
        b |= (*it & 1) << (8*sizeof(b) - 1 - i);
        ++it;
    }
    // flush 'b'
}
*/


#endif // ATMM_H
