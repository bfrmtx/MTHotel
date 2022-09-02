#include <iostream>
#include <iomanip>
#include <filesystem>
#include <list>
#include <memory>
#include <mutex>

#include "mt_base.h"
#include "atss.h"
#include "survey.h"


using jsn = nlohmann::ordered_json;

int main(int argc, char **argv)
{

    std::cout << "testing reading survey structure, give survey dir name as parameter" << std::endl;

    if (argc < 2) return 0;
    std::filesystem::path survey_dir(argv[1]);

    std::vector<std::filesystem::path> sites;
    std::vector<std::shared_ptr<channel>> channels;

    auto cal = std::make_shared<calibration>();
    cal->set_format(CalibrationType::mtx, false);

    std::unique_ptr<survey_d> survey;

    try {
        // create a survey with existin data
        survey  = std::make_unique<survey_d>(argv[1]);

    }
    catch (std::filesystem::filesystem_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }


    survey->ls();




    return 0;
}
