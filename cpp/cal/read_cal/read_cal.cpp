#include "read_cal.h"

read_cal::read_cal()
{

    auto dbfile = working_dir("data", "info.sql3");
    if (!dbfile.size()) {

        return;
    }
    else {
        this->dbloaded = true;
    }
    // that is a two column db
    this->sqldb.sqlite_select(dbfile, "SELECT * FROM sensor_aliases");
    for (auto &row : this->sqldb.table) {
        //        for (auto &col : row) {
        //            std::cout << col << " ";
        //        }
        //        std::cout << std::endl;

        if (row.size() == 2) this->sensor_aliases.emplace_back(std::make_pair(row.at(0), row.at(1)));
    }

    // std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
    if (!this->sensor_aliases.size()) this->dbloaded = false;

    this->f.reserve(200);
    this->a.reserve(200);
    this->p.reserve(200);
}

read_cal::~read_cal()
{

}


std::shared_ptr<calibration> read_cal::read_std_mtx_txt(const fs::path &filename, const ChopperStatus &chopper)
{

    auto cal = std::make_shared<calibration>();
    if (!this->dbloaded) {
        std::string err_str = __func__;
        err_str += ":: no database loaded, e.g info.sql3 missing";
        throw err_str;
        return cal;
    }
    if (!fs::exists(filename)) {
        std::string err_str = __func__;
        err_str += ":: file not found ->";
        err_str += std::filesystem::absolute(filename).string();
        throw err_str;
        return cal;
    }

    fs::path name(filename.filename());
    name.replace_extension("");
    std::string calfilename = name.string();
    std::transform(calfilename.begin(), calfilename.end(), calfilename.begin(), ::tolower);


    for (const auto &pairs : this->sensor_aliases) {
        if (mstr::begins_with(calfilename, pairs.first)) {
            this->fmagtype = pairs.second;
            // remove substring
            calfilename.erase(0, pairs.first.size());
            for (size_t i = 0; i < calfilename.size(); i++) {
                if (std::isdigit(calfilename[i])) this->fmagser += calfilename[i];
            }
            break;
        }
    }




    std::string chopper_tag = "empty      ";
    if (chopper == ChopperStatus::on) {
        chopper_tag = "chopper on";
    }
    else if(chopper == ChopperStatus::off) {
        chopper_tag = "chopper off";
    }


    cal->set_format(CalibrationType::mtx_old);
    std::size_t found = 0;
    bool failed = false;
    std::ifstream file(filename);
    std::string str;
    std::string line;
    bool header = true;
    while (std::getline(file, str)) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        line = mstr::simplify(str); // remove leading, trailing white spaces and double whites
        if (header) {
            found = line.find("break");
            if (found != std::string::npos) {
                std::cerr << "file contains break! corrupted " << filename.string() << '\n';
                failed = true;
                break;
            }

            found = line.find("solartron");
            if (found != std::string::npos) {
                this->date_hint= "DD/mm/YY";

            }
            found = line.find("numetriq");
            if (found != std::string::npos) {
                this->date_hint = "YY/mm/DD";

            }

            found = line.find("magnetometer");
            if (found != std::string::npos) {
                found = line.find("date");
                bool tryme = false;
                if (found != std::string::npos) tryme = true;
                auto items = mstr::split(line, ' ');
                if (tryme && (items.size() > 3) ) {
                    if (mstr::contains(items.at(0), "magnetometer", false) && !mstr::contains(items.at(1), "date", false) ) {
                        std::string res = items.at(1);
                        if (mstr::contains(res, "#")) {
                            auto items2 = mstr::split(res, '#');
                            if (items2.size() == 2) {
                                this->magtype = mstr::trim(items2.at(0));
                                this->magser = mstr::trim(items2.at(1));

                                for (const auto &pairs : this->sensor_aliases) {
                                    if (pairs.first == this->magtype) {
                                        this->magtype = pairs.second;
                                        break;
                                    }
                                }

                            }
                        }
                    }

                    if (mstr::contains(items.at(2), "date", false) && !mstr::contains(items.at(4), "/", false) ) {
                        this->magdate = mstr::trim(items.at(3));
                    }

                }
            }

            found = line.find("chopper");
            if (found != std::string::npos) {

                header = false;
                if (this->magser != this->fmagser) {
                    std::cerr << "serial from file differs from file content, takeing from file name!" << std::endl;
                }
                cal->serial = std::stoi(this->fmagser);
                if (this->magtype != this->fmagtype) {
                    std::cerr << "mag type from file differs from file content, takeing from file name!" << std::endl;
                }
                cal->sensor = this->fmagtype;
                cal->date = this->magdate;
                found = line.find("chopper on");
                if (found != std::string::npos) {
                    this->chopper = ChopperStatus::on;
                }
                found = line.find("chopper off");
                if (found != std::string::npos) {
                    this->chopper = ChopperStatus::off;
                }

            }


        }
        else {
            if (this->chopper == chopper) {
                auto values = mstr::split(line, ' ');


                if (values.size() == 3) {
                    this->f.emplace_back(std::stod(values[0]));
                    this->a.emplace_back(std::stod(values[1]));
                    this->p.emplace_back(std::stod(values[2]));
                }
            }

            else {
                found = line.find("chopper on");
                if (found != std::string::npos) {
                    this->chopper = ChopperStatus::on;
                }
                found = line.find("chopper off");
                if (found != std::string::npos) {
                    this->chopper = ChopperStatus::off;
                }
            }
        }

        std::cout << line << '\n';
    }
    file.close();

    if (!failed) {
        this->guess_date(this->magdate);
        cal->date = this->magdate;
        cal->serial = std::stoi(this->magser);
        cal->sensor = this->magtype;

        if ((f.size() == this->p.size()) && (f.size() == a.size())) {
            cal->f = this->f;
            cal->a = this->a;
            cal->p = this->p;
        }



    }

    if (failed) {
        cal->clear();
        std::string err_str = __func__;
        err_str += ":: FAILED to read inside file, check BREAK or numbers ->";
        err_str += std::filesystem::absolute(filename).string();
        throw err_str;
    }


    return cal;
}


void read_cal::clear()
{
    if (this->cal_date != nullptr) this->cal_date.reset();
    this->f.clear();
    this->a.clear();
    this->p.clear();
    this->f.reserve(200);
    this->a.reserve(200);
    this->p.reserve(200);

    this->magser.clear();
    this->magtype.clear();
    this->magdate.clear();
    this->date_hint.clear();
}


std::string read_cal::get_units_mtx_old() const
{
    std::string lines;
    lines = "FREQUENCY    MAGNITUDE    PHASE\n";
    lines += "Hz           V/(nT*Hz)    deg\n";

    return lines;

}

void read_cal::guess_date(const std::string &datestr)
{

    auto ymd = mstr::split(this->magdate, '/');
    int y = 0, m = 0, d = 0;
    bool b_iso = false;
    if (ymd.size() == 3) {
        if (date_hint == "DD/mm/YY") {
            y = std::stoi(ymd.at(2)) + 2000;
            m = std::stoi(ymd.at(1));
            d = std::stoi(ymd.at(0));
        }
        if (date_hint == "YY/mm/DD") {
            y = std::stoi(ymd.at(0)) + 2000;
            m = std::stoi(ymd.at(1));
            d = std::stoi(ymd.at(2));
        }
    }
    // try iso date
    if (ymd.size() == 0) {
        ymd = mstr::split(this->magdate, '-');
        if (ymd.size() == 3) {
            y = std::stoi(ymd.at(0));
            m = std::stoi(ymd.at(1));
            d = std::stoi(ymd.at(2));
            b_iso = true;
        }
    }

    if (ymd.size() == 0) return;

    int min_year = 0, max_year = 0;
    if (this->magtype == "MFS-07") {
        min_year = 2003;
        max_year = 2012;
    }

    if (this->magtype == "MFS-07e") {
        min_year = 2009;
        max_year = 2030;
    }
    if (this->magtype == "MFS-06") {
        min_year = 2000;
        max_year = 2010;
    }

    if (this->magtype == "MFS-06e") {
        min_year = 2009;
        max_year = 2030;
    }

    if (this->magtype == "MFS-08") {
        min_year = 2002;
        max_year = 2010;
    }

    if (this->magtype == "MFS-10e") {
        min_year = 2014;
        max_year = 2030;
    }

    if (this->magtype == "SHFT-02") {
        min_year = 2000;
        max_year = 2010;
    }

    if (this->magtype == "SHFT-02e") {
        min_year = 2009;
        max_year = 2030;
    }

    if (!b_iso) {
        if ( (y < min_year) || (y > max_year) ) {
            std::swap(y, d);
            if ( (y < min_year) || (y > max_year) ) {
                return;
            }

        }
    }

    this->cal_date = time_from_ints(y, m, d);
    this->magdate = tm_to_str_date(this->cal_date);
}



 std::vector<std::shared_ptr<calibration>> read_cal::read_std_xml(const fs::path &filename)
{
    auto tir = std::make_shared<tinyxml2::XMLDocument>();
    std::vector<std::shared_ptr<calibration>> cal_entries;

    if (!fs::exists(filename)) {
        return cal_entries;
    }

    try {

        bool loaded = tir->LoadFile(filename.string().c_str());
        if (loaded) {
            std::string err_str = __func__;
            err_str += ":: error loading XML file ";
            err_str += filename.string();
            throw err_str;
            return cal_entries;
        }
        auto proot = tir->RootElement(); // that is the envelope, mostly "measurement"
        if (proot == nullptr) {
            std::string err_str = __func__;
            err_str += "::Root Element XML_ERROR_FILE_READ_ERROR";
            err_str += filename.string();
            throw err_str;
            return cal_entries;
        }

        auto pscal_sens = open_node(proot, "calibration_sensors");
        auto pchan = open_node(pscal_sens, "channel");

        while (pchan) {
            int id = -1;
            int old_id = id;
            pchan->QueryIntAttribute("id", &id);
            if (old_id != id) {
                std::cout << "sensor for channel: " << id << std::endl;
                old_id = id;
                auto pca = open_node(pchan, "calibration");
                auto pci = open_node(pca, "calibrated_item");
                std::string sensor(xml_svalue(pci, "ci"));
                std::cout << sensor << " detected" << std::endl;
                int64_t serial = xml_ivalue(pci, "ci_serial_number");
                std::string cal_date(xml_svalue(pci, "ci_date"));
                std::string cal_time(xml_svalue(pci, "ci_time"));
                std::vector<double> f_on, f_off, a_on, a_off, p_on, p_off;
                auto f_unit = std::make_unique<std::string>();
                auto a_unit = std::make_unique<std::string>();
                auto p_unit = std::make_unique<std::string>();

                // cd will be NULL when there is no cal data, like e.g. for E
                auto cd = open_node(pca, "caldata", true);
                while (cd) {
                    if (cd != nullptr) {
                        std::string strchp("ukn");
                        const char *cchopper = cd->Attribute("chopper");
                        if (cchopper != nullptr) {
                            strchp = std::string(cchopper);
                        }
                        double f = xml_dvalue(cd, "c1", f_unit.get(), "unit");
                        double a = xml_dvalue(cd, "c2", a_unit.get(), "unit");
                        double p = xml_dvalue(cd, "c3", p_unit.get(), "unit");
                        if ((f != DBL_MAX && (a != DBL_MAX) && (p != DBL_MAX))) {
                            if (strchp == "on") {
                                f_on.push_back(f);
                                a_on.push_back(a);
                                p_on.push_back(p);
                            } else {
                                f_off.push_back(f);
                                a_off.push_back(a);
                                p_off.push_back(p);
                            }
                        }
                    }
                    cd = cd->NextSiblingElement("caldata");
                }
                if (f_on.size()) {
                    std::cout << "on  size " << f_on.size() << std::endl;
                    cal_entries.emplace_back(std::make_shared<calibration>());
                    if ((*a_unit.get() == "V/(nT*Hz)") && (*f_unit.get() == "Hz") && (*p_unit.get() == "deg")) {
                        cal_entries.back()->set_format(CalibrationType::mtx_old);
                        cal_entries.back()->chopper = ChopperStatus::on;
                        cal_entries.back()->f = f_on;
                        cal_entries.back()->a = a_on;
                        cal_entries.back()->p = p_on;
                        cal_entries.back()->sensor = sensor;
                        if (serial != INT64_MAX)
                            cal_entries.back()->serial = serial;
                        if (cal_date.size())
                            cal_entries.back()->date = cal_date;
                        if (cal_time.size())
                            cal_entries.back()->time = cal_time;
                        // cal_entries.back()->write_file("/tmp");
                    }
                }
                if (f_off.size()) {
                    std::cout << "off size " << f_off.size() << std::endl;
                    if ((*a_unit.get() == "V/(nT*Hz)") && (*f_unit.get() == "Hz") && (*p_unit.get() == "deg")) {
                        cal_entries.emplace_back(std::make_shared<calibration>());
                        cal_entries.back()->set_format(CalibrationType::mtx_old);
                        cal_entries.back()->chopper = ChopperStatus::off;
                        cal_entries.back()->f = f_off;
                        cal_entries.back()->a = a_off;
                        cal_entries.back()->p = p_off;
                        cal_entries.back()->sensor = sensor;
                        if (serial != INT64_MAX)
                            cal_entries.back()->serial = serial;
                        if (cal_date.size())
                            cal_entries.back()->date = cal_date;
                        if (cal_time.size())
                            cal_entries.back()->time = cal_time;

                        // cal_entries.back()->write_file("/tmp");
                    }
                }
                // likely electrodes or old sensors
                if (!f_on.size() && !f_off.size() && (serial != INT64_MAX)) {
                    cal_entries.emplace_back(std::make_shared<calibration>());
                    cal_entries.back()->sensor = sensor;
                    if (serial != INT64_MAX)
                        cal_entries.back()->serial = serial;
                    if (cal_date.size())
                        cal_entries.back()->date = cal_date;
                    if (cal_time.size())
                        cal_entries.back()->time = cal_time;
                    cal_entries.back()->write_file("/tmp");
                }
            }
            pchan = pchan->NextSiblingElement("channel");
        }

    } catch (const std::string &error) {
        std::cerr << error << std::endl;
    }


    return cal_entries;

}

