#include <iostream>
#include <iomanip>
#include <filesystem>
#include <list>
#include <memory>
#include <mutex>

#include "mt_base.h"
#include "atss.h"

void add_channel_to_channels(std::vector<std::shared_ptr<channel>> &channels, std::mutex &add_me, const std::vector<std::shared_ptr<channel>> &found_channels) {
    try {
        std::lock_guard<std::mutex> lck (add_me);
    }
    catch (...) {
        std::cerr << "execption cought in add_channel_to_channels" << std::endl;
        return;

    }
}

void site_scan(const std::filesystem::path &site, std::mutex &add_me, std::vector<std::shared_ptr<channel>> &channels) {

    std::vector<std::shared_ptr<channel>> chans;
    for (auto const& atssfile : fs::recursive_directory_iterator(site)) {
        if (std::filesystem::is_regular_file(atssfile) && atssfile.path().extension() == ".json") {
            std::filesystem::path atss(atssfile);
            atss.replace_extension(".atss");
            if (std::filesystem::exists(atss)) {
                std::cout << atssfile << " " << atss.filename() << '\n';
                chans.emplace_back(std::make_shared<channel>(atssfile));
                // if test ..
            }

        }
    }
    if (chans.size()) {
        std::sort(chans.begin(), chans.end(), compare_channel_start_lt);
        try {
            std::lock_guard<std::mutex> lck (add_me);
            for (const auto &res : chans) {
                channels.emplace_back(std::make_shared<channel>(res));
            }

        }
        catch (...) {
            std::cerr << "execption cought in find::site_scan" << std::endl;
            return;
        }

    }
}

int main(int argc, char **argv)
{

    std::cout << "testing reading survey structure, give survey dir name as parameter" << std::endl;

    if (argc < 2) return 0;
    std::filesystem::path survey_dir(argv[1]);

    std::vector<std::filesystem::path> sites;
    std::vector<std::shared_ptr<channel>> channels;

    auto ts_dir = survey_dir / "ts";
    if (std::filesystem::exists(ts_dir)) {
        for (const auto & entry : std::filesystem::directory_iterator(ts_dir)) {
            std::cout << entry.path() << std::endl;
            if (std::filesystem::is_directory(entry))  sites.emplace_back(entry);
        }
    }

    std::mutex add_me;

    if (sites.size()) {
        for (const auto &site : sites) {
            site_scan(site, add_me, channels);

        }
    }

    std::list<std::shared_ptr<channel>> all_chans; // scan the survey
    // "/tmp/aa/Short_Mining/ts/Sarıçam/meas_2019-11-04_07-55-03/098_ADU-07e_C003_R000_THy_8Hz.json"
    // "/tmp/aa/Short_Mining/ts/Sarıçam/meas_2019-11-04_07-55-03/098_ADU-07e_C000_R000_TEx_8Hz.json"
    std::filesystem::path sf("/tmp/aa/Short_Mining/ts/Sarıçam/meas_2019-11-04_07-55-03/098_ADU-07e_C000_R000_TEx_8Hz.json");
                             channel test(sf);

                             std::cout << test.brief() << std::endl;

                             std::cout << "main finished" << std::endl;







        return 0;
}
