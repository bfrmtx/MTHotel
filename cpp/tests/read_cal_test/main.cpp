#include "read_cal.h"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <memory>

int main(int argc, char **argv)
{
    using namespace std;
    namespace fs = std::filesystem;
    auto exec_path = std::filesystem::path(argv[0]);


    cout << "testing reading cal files, give file name as parameter" << endl;

    if (argc < 2) return 0;


    std::shared_ptr<read_cal> mtx_cal_file = std::make_shared<read_cal>();
//    if (!mtx_cal_file->is_init()) {
//        mtx_cal_file.reset();
//        // in this short test we could return -1 at this moment
//        // from now on we can check against nullprt
//        // or wait for throwing and exception
//    }
    std::shared_ptr<calibration> cal, cal_in;


    fs::path filename(argv[1]);

    // in a greater programm we can test
    if (mtx_cal_file != nullptr) {

        // try to read
        try {
            cal = mtx_cal_file->read_std_mtx_txt(filename, ChopperStatus::off);

        }
        catch (const std::string &error) {

            std::cerr << error << std::endl;
            cal.reset();

            // we could also reset mtx_cal_file.reset()
            // depending on what we want to do
            // in a loop of many files we keep mtx_cal_file alive
        }
    }

    if (cal != nullptr) {
        cal->write_file(fs::path(getenv("HOME")));
    }

    std::cout  << endl << "finish write " << endl;

    cal_in = std::make_unique<calibration>();

//    size_t ncals = 0;
//    try {
//        ncals = cal_in->read_file("/home/bfr/MFS-07e_502_chopper_off.json", false);
//    }
//    catch (const std::string &error) {
//        std::cerr << error << std::endl;
//        cal_in.reset();
//    }
//    std::cout  << endl << "finish read " << ncals << endl;

//    if (cal != cal_in) {
//        std::cout << "not equal" << std::endl;
//    }


    return 0;
}
