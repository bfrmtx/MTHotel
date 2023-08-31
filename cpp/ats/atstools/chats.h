#ifndef CHATS_H
#define CHATS_H

#include <iostream>
#include <vector>

#include "atsheader.h"

void chats_files(std::shared_ptr<atsheader> &atsh, std::shared_ptr<atsheader> &adu08, const std::shared_ptr<ats_header_json> &atsj,
                 const size_t &adu06_shift_samples_hf, const size_t &adu06_shift_samples_lf) {

    std::cout << adu08->path() << std::endl;

    adu08->write(false);
    // now read the data, do no conversion
    size_t chunk_size = 524288;
    std::vector<int32_t> ints;
    std::vector<int32_t> skip_utc;




    //get the data and change
    if (adu06_shift_samples_hf && atsj->header["ADB_board_type"].get<std::string>() == "HF") {

        // LF board 31 samples cut, no date shift - old source code 30
        // HF board 32 - old source 32

        skip_utc.resize(adu06_shift_samples_hf);

    }
    if (adu06_shift_samples_lf && atsj->header["ADB_board_type"].get<std::string>() == "LF") {

        // LF board 31 samples cut, no date shift - old source code 30
        // HF board 32 - old source 32

        skip_utc.resize(adu06_shift_samples_lf);

    }
    size_t samples_read = 0;

    try {
        double lsbval = atsj->header["lsbval"];
        atsh->read();           // read header

        if (skip_utc.size()) atsh->ats_read_ints_doubles(skip_utc);
        if (atsj->header["samples"] < (chunk_size - skip_utc.size())) {
            ints.resize(size_t(atsj->header["samples"]) - skip_utc.size());
        }
        else {
            ints.resize(chunk_size);
        }

        do {
            atsh->ats_read_ints_doubles(ints);
            samples_read += ints.size();
            adu08->ats_write_ints_doubles(lsbval, ints);
        } while (ints.size());

        //update samples!
        adu08->header.samples = int32_t(samples_read);
        adu08->re_write();
    }

    catch (const std::string &error) {
        std::cerr << error << std::endl;
        std::cerr << "could not chats / create ats files" << std::endl;
        EXIT_FAILURE;
    }
    catch(...) {
        std::cerr << "could not chats / create ats files" << std::endl;
        EXIT_FAILURE;

    }



}


#endif // CHATS_H
