#ifndef SURVEY_H
#define SURVEY_H

#include "atss.h"

// ************************************************************************  R U N *******************************************************************


/*!
 * \brief The run_d class - the main survey class should be in a try block and keep the locks
 */
class run_d {

public:

    run_d(const std::filesystem::path &run_dir, const bool no_create = true) : run_dir(run_dir) {

        this->channels.reserve(10);
        if (!no_create) {
            std::filesystem::create_directory(this->run_dir);
        }
        else {
            this->scan();
        }
        this->run_dir = std::filesystem::canonical(this->run_dir);
    }

    ~run_d() {
        this->clear();
    }

    std::filesystem::path add_channel(std::shared_ptr<channel> &new_channel) {

        if (!this->channels.size()) {
            new_channel->set_dir(this->run_dir);
            this->channels.push_back(new_channel);
            return this->run_dir;
        }
        else {
            for (auto &chan : this->channels) {
                if (same_run(chan, new_channel)) {
                    new_channel->set_dir(this->run_dir);
                    this->channels.push_back(new_channel);
                    return this->run_dir;
                }
            }
        }
        return std::filesystem::path();
    }


    size_t get_run_no() const {
        return mstr::string2run(this->run_dir.filename().c_str());
    }

    void clear() {
        for (auto &chan : this->channels) {
            if (chan != nullptr) chan.reset();
        }
        this->channels.clear();
    }

    size_t scan() {

        this->clear();
        for (auto const& atssfile : fs::recursive_directory_iterator(this->run_dir)) {
            if (std::filesystem::is_regular_file(atssfile) && atssfile.path().extension() == ".json") {
                std::filesystem::path atss(atssfile);
                atss.replace_extension(".atss");
                if (std::filesystem::exists(atss)) {
                    // std::cout << atssfile << " " << atss.filename() << '\n';
                    this->channels.emplace_back(std::make_shared<channel>(atssfile));
                    // if test ..
                }
            }
        }

        return this->channels.size();
    }

    std::shared_ptr<channel> ch_first()  {
        if (!channels.size()) {
            std::string err_str = __func__;
            err_str += "Station " + this->run_dir.parent_path().filename().string() + " " + this->run_dir.filename().string() +" is empty";
            throw err_str;
        }
        return channels.at(0);
    }

    void ls() const {
        size_t i = 0;
        for (const auto &ch : channels) {
            ++i;
            if (i != channels.size()) {
                std::cout << "    " << "│   " << "│   " << "├" << "───" << ch->filename() << "   " << ch->brief()  <<  std::endl;
            }
            else {
                std::cout << "    " << "│   " << "│   " << "└" << "───" << ch->filename() << "   " << ch->brief()  << std::endl;
            }
        }
    }

    std::filesystem::path run_dir;
    std::vector<std::shared_ptr<channel>> channels;


};


// ************************************************************************ S T A T I O N *******************************************************************

/*!
 * \brief The station_d class
 */
class station_d {
public:

    /*!
     * \brief station_d
     * \param station_dir
     * \param no_create if true, we perfom a site scan
     */
    station_d(const std::filesystem::path &station_dir, const bool no_create = true) : station_dir(station_dir) {

        if (!no_create) {
            std::filesystem::create_directory(this->station_dir);
        }
        else {
            this->scan();
        }
        this->station_dir = std::filesystem::canonical(this->station_dir);
    }

    ~station_d() {
        this->clear();
    }

    void clear() {
        for (auto &run : this->runs) {
            if (run != nullptr) {
                run.reset();
            }
        }
        this->runs.clear();
    }

    size_t scan() {

        this->clear();
        for (const auto & entry : std::filesystem::directory_iterator(station_dir)) {
            std::cout << entry.path() << std::endl;
            if (std::filesystem::is_directory(entry)) {
                auto is_run_no = mstr::string2run(entry.path().filename().string());
                if (is_run_no != SIZE_MAX) runs.push_back(std::make_shared<run_d>(entry));
            }
        }

        return runs.size();
    }

    /*!
     * \brief scan_free_run_no
     * \return first free run number inside the station directory
     */
    size_t scan_free_run_no() const {
        return fdirs::scan_runs(this->station_dir);
    }


    std::filesystem::path add_create_run(std::shared_ptr<channel> &new_channel) {

        std::filesystem::path runpath;

        for (auto &run : this->runs) {
            runpath = run->add_channel(new_channel);
        }

        if (runpath.empty()) {
            auto irun = this->scan_free_run_no();
            if (irun == SIZE_MAX) return std::filesystem::path();
            runpath = this->station_dir / mstr::run2string(irun);

            runs.push_back(std::make_shared<run_d>(runpath, false));
            runpath = runs.back()->add_channel(new_channel);

            std::filesystem::path mrunpath = fdirs::meta_dir(this->station_dir) / mstr::run2string(irun);
            if (!std::filesystem::exists(mrunpath)) std::filesystem::create_directory(mrunpath);
        }

        return runpath;
    }

    std::shared_ptr<run_d> get_run(const size_t &run_no) const{
        std::filesystem::path spath;
        auto srun = mstr::run2string(run_no);
        spath = this->station_dir / srun;
        std::shared_ptr<run_d> run;

        if (!std::filesystem::exists(spath)) {
            std::string err_str = __func__;
            err_str += ":: run does not exists! " + this->station_dir.filename().string();
            throw err_str;
        }
        else {
            for (const auto &r : this->runs ) {
                if (r->get_run_no() == run_no) return r;
            }
        }
        return run;
    }

    void ls() const {
        size_t i = 0;
        for (const auto &run : runs) {
            ++i;
            if (i != runs.size()) {
                std::cout  << "    " << "│   " << "├" << "───" << run->run_dir.filename().string();
                if (run->channels.size())  std::cout  << std::endl;
                else std::cout <<  " (empty) "<< std::endl;
            }
            else {
                std::cout << "    " << "│   " << "└" << "───" << run->run_dir.filename().string();
                if (run->channels.size())  std::cout  << std::endl;
                else std::cout <<  " (empty) "<< std::endl;
            }
            run->ls();
        }
    }

    std::vector<std::shared_ptr<run_d>> runs;
    std::filesystem::path station_dir;


};

// ************************************************************************ S U R V E Y *******************************************************************


class survey_d  {

public:

    /*!
     * \brief survey_d, put in a try block and catch(catch (std::filesystem::filesystem_error& e) { e.what() } )
     * \param survey_dir
     * \param no_create if true we perfom a suvey scan
     * \param no_create == false we add tree_size files to the survey
     */

    survey_d(const std::filesystem::path &survey_dir, const bool no_create = true, size_t tree_size = 0) : survey_dir(survey_dir) {

        std::unique_lock lock(this->station_lock);
        if (!no_create) {
            if (!std::filesystem::exists(this->survey_dir)) {
                std::filesystem::create_directory(this->survey_dir);
                this->survey_dir = std::filesystem::canonical(this->survey_dir);
                create_survey_dirs(this->survey_dir, survey_dirs());
            }


        }
        else {
            this->survey_dir = std::filesystem::canonical(this->survey_dir);
            this->scan();

        }

        if (tree_size) this->all_channels.reserve(tree_size);

    }

    void collect(const std::shared_ptr<channel> &chan) {

        std::unique_lock lock(this->station_lock);
        this->all_channels.push_back(chan);
    }

    void mk_tree() {

        std::sort(this->all_channels.begin(), this->all_channels.end(), compare_channel_start_lt);
        if (!std::filesystem::exists(this->survey_dir)) {

        }
        std::map<std::string, int> all_stations;
        int i = 0;
        for (const auto &ch : this->all_channels ) {
            all_stations.emplace(ch->tmp_station, i++);
        }
        i = 0;
        for (const auto &s : all_stations) {
            auto station = this->create_station(s.first);
            for (auto &ch : this->all_channels) {
                if (station.filename() == ch->tmp_station) {
                    this->add_create_run(station, ch);
                    if (ch->filepath_wo_ext.empty()) {
                        std::string err_str = __func__;
                        err_str += ":: NO PATH for channel CREATED ";
                        throw err_str;
                    }
                }
            }
        }
    }

    std::shared_ptr<channel> get_channel_from_all(const size_t &index) const {
        std::shared_lock lock(this->station_lock);
        return this->all_channels.at(index);
    }

    ~survey_d() {

        this->clear();
    }

    void clear() {
        for (auto &station : this->stations) {
            if (station != nullptr) {
                station.reset();
            }
        }
        this->stations.clear();
    }

    std::shared_ptr<station_d> get_station(const std::string& station_name) const {
        std::shared_lock lock(this->station_lock);
        std::filesystem::path spath;
        spath = this->survey_dir / "stations" / station_name;
        // lamda
        auto stat = std::find_if(this->stations.begin(), this->stations.end(), [spath] (const std::shared_ptr<station_d> s) { return s->station_dir == spath ; });
        if (stat == stations.end()) {
            std::string err_str = __func__;
            err_str += "Station " + station_name + " does not exists";
            throw err_str;
        }

        return *stat;
    }


    std::shared_ptr<channel> get_first_ch(const std::string &station_name, const size_t &run_no) const {

        auto station = this->get_station(station_name);
        // check nullptr ... ? use catch!
        auto run = station->get_run(run_no);
        return run->ch_first();


    }

    void hide_station(const std::string &station_name) {
        try {

            std::unique_lock lock(this->station_lock);
            auto iter = this->stations.begin();
            while (iter != this->stations.end()) {
                // remove strings having length 5
                if ((*iter)->station_dir.filename() == station_name) {
                    iter = this->stations.erase(iter);
                }
                else {
                    ++iter;
                }
            }
        }
        catch(...) {
            std::cerr <<  "can not hide" << std::endl;
        }
    }

    std::filesystem::path add_create_run(const std::filesystem::path &station_path, std::shared_ptr<channel> &new_channel) {

        std::unique_lock lock(this->station_lock);

        for (auto &station : this->stations) {
            if (station->station_dir == station_path) return station->add_create_run(new_channel);
        }
        return std::filesystem::path();
    }


    std::filesystem::path create_station(const std::string &station_name) {
        std::unique_lock lock(this->station_lock);
        std::filesystem::path spath;


        spath = this->survey_dir / "stations" / station_name;
        // lamda
        if (std::find_if(this->stations.begin(), this->stations.end(), [spath] (const std::shared_ptr<station_d> s) { return s->station_dir == spath ; }) != stations.end()) {

            std::string err_str = __func__;
            err_str += "Station " + station_name + " already exists";
            throw err_str;
        }


        std::filesystem::path mpath = this->survey_dir / "meta" / station_name;

        if (!std::filesystem::exists(spath)) this->stations.emplace_back(std::make_shared<station_d>(spath, false));
        else {
            std::string err_str = __func__;
            err_str += "Station " + station_name + " already exists in filesystem - you can't create it";
            throw err_str;
        }
        if (!std::filesystem::exists(mpath)) std::filesystem::create_directory(mpath);

        return spath;
    }

    std::filesystem::path survey_path() const {
        std::shared_lock lock(this->station_lock);
        return this->survey_dir;
    }

    void ls() const {
        std::shared_lock lock(this->station_lock);
        std::cout  << std::endl <<  survey_dir.parent_path().string() << " contains:" << std::endl;
        std::cout <<  survey_dir.filename().string() << std::endl;
        size_t i = 0;
        for (const auto &station : stations) {
            ++i;
            if (i != stations.size()) {
                std::cout   << "    " << "├" << "───" << station->station_dir.filename().string();
                if (station->runs.size())  std::cout  << std::endl;
                else std::cout <<  " (empty) "<< std::endl;
            }
            else {
                std::cout << "    " << "└" << "───" << station->station_dir.filename().string();
                if (station->runs.size()) std::cout  << std::endl;
                else std::cout <<  " (empty) "<< std::endl;
            }
            station->ls();
        }
    }


private:
    size_t scan() {
        try {
            this->clear();
            std::filesystem::path spath = this->survey_dir / "stations";
            for (const auto & entry : std::filesystem::directory_iterator(spath)) {
                //std::cout << entry.path() << std::endl;
                if (std::filesystem::is_directory(entry)) {
                    // no create == true -> we scan the station
                    this->stations.emplace_back(std::make_shared<station_d>(entry, true));
                }
            }
        }

        catch(std::filesystem::filesystem_error& e) {
            std::cerr <<  e.what() << std::endl;
        }
        return this->stations.size();
    }

    std::vector<std::shared_ptr<station_d>> stations;
    std::vector<std::shared_ptr<channel>> all_channels;

    std::filesystem::path survey_dir;
    mutable std::shared_mutex station_lock;
};


#endif // SURVEY_H