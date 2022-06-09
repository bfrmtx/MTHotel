#include "read_cal.h"

read_cal::read_cal()
{

    auto dbfile = working_dir("data", "info.sql3");

    try {
        if (!std::filesystem::exists(dbfile)) {
            dbfile = getenv("HOME");
            dbfile /= "devel/github_mthotel/MTHotel/cpp/data/info.sql3";

            if (!std::filesystem::exists(dbfile)) {
                std::string err_str = __func__;
                err_str += ":: read_cal() ";
                err_str += dbfile.string();
                throw err_str;
                return;
            }
        }
    }
    catch (const std::string &error) {
        std::cerr << error << std::endl;
    }
    this->dbloaded = true;
    // that is a two column db
    this->sqldb.sqlite_select(dbfile, "SELECT * FROM sensor_aliases");
    for (auto &row : this->sqldb.table) {
        //                for (auto &col : row) {
        //                    std::cout << col << " ";
        //                }
        //                std::cout << std::endl;

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
    this->clear();
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
        bool ok = false;
        //std::cout << pairs.first << "  " <<  pairs.second << std::endl;
        if (mstr::begins_with(calfilename, pairs.first)) {

            std::string xcal(calfilename);
            this->fmagtype = pairs.second;
            // remove substring
            xcal.erase(0, pairs.first.size());
            if (std::isdigit(xcal[0])) ok = true;  // we have a number and not an e
            if (std::isalpha(xcal[0])) ok = false;  // we have a number and not an e
            if (ok) {
                for (size_t i = 0; i < xcal.size(); i++) {
                    if (std::isdigit(calfilename[i])) this->fmagser += xcal[i];
                }
                break;
            }
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
    bool stop = false;
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

            found = line.find("date");
            if (found != std::string::npos) {
                auto tokens(mstr::split(line, "date"));
                if (tokens.size() == 2) {
                    auto tokens_dt(mstr::split(tokens.at(1), "time"));
                    if (tokens_dt.size() == 2) {
                        this->magtime = mstr::trim(tokens_dt.at(1));
                        if (this->magtime.at(0) == ':') this->magtime = this->magtime.substr(1);
                        this->magdate = mstr::trim(tokens_dt.at(0));
                        this->magdate.erase(remove(this->magdate.begin(), this->magdate.end(), ':'), this->magdate.end());
                    }
                }
            }


            found = line.find("chopper");
            if (found != std::string::npos) {

                found = line.find("chopper on");
                if (found != std::string::npos) {
                    this->chopper = ChopperStatus::on;
                    if (this->chopper == chopper) header = false;
                }
                found = line.find("chopper off");
                if (found != std::string::npos) {
                    this->chopper = ChopperStatus::off;
                    if (this->chopper == chopper) header = false;
                }

            }


        }
        else {
            if ((this->chopper == chopper) && !stop) {
                auto values = mstr::split(line, ' ');
                // we may have an empty line
                if (values.size() == 3) {
                    // data line never starts with ABC.. and also frequencies are not negative -
                    // if the number is +123 ... I don't know where it is coming from - change the files
                    if (std::isalpha(values[0].at(0)) ||  std::ispunct(values[0].at(0))) {
                        stop = true;
                    }
                    else {
                        this->f.emplace_back(std::stod(values[0]));
                        this->a.emplace_back(std::stod(values[1]));
                        this->p.emplace_back(std::stod(values[2]));
                    }
                }
            }
        }

        //     std::cout << line << '\n';
    }
    file.close();

    if (!failed) {
        this->guess_date(this->magdate);
        cal->date = this->magdate;
        cal->time = this->magtime;
        cal->serial = std::stoi(this->fmagser);
        cal->sensor = this->fmagtype;
        cal->chopper = this->chopper;

        if ((this->f.size() == this->p.size()) && (this->f.size() == this->a.size())) {
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

    this->fmagser.clear();
    this->fmagtype.clear();
    this->magdate.clear();
    this->date_hint.clear();
    this->chopper = ChopperStatus::off;
}


std::string read_cal::get_units_mtx_old() const
{
    std::string lines;
    lines = "FREQUENCY    MAGNITUDE    PHASE\n";
    lines += "Hz           V/(nT*Hz)    deg\n";

    return lines;

}

void read_cal::guess_date(std::string &datestr)
{

    auto ymd = mstr::split(this->magdate, '/');
    int y = 0, m = 0, d = 0;
    if (ymd.size() == 3) {
        if (date_hint == "DD/mm/YY") {  // old solartron
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
        }
    }

    if (ymd.size() == 0) return;


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
                    // cal_entries.back()->write_file("/tmp");
                }
            }
            pchan = pchan->NextSiblingElement("channel");
        }

    } catch (const std::string &error) {
        std::cerr << error << std::endl;
    }


    return cal_entries;

}

std::vector<std::shared_ptr<calibration> > read_cal::read_std_xml_single(const std::filesystem::__cxx11::path &filename)
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
        auto proot = tir->RootElement(); // that is the envelope, mostly "calibration"
        if (proot == nullptr) {
            std::string err_str = __func__;
            err_str += "::Root Element XML_ERROR_FILE_READ_ERROR";
            err_str += filename.string();
            throw err_str;
            return cal_entries;
        }




        auto pci = open_node(proot, "calibrated_item");
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
        auto cd = open_node(proot, "caldata", true);
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
            // cal_entries.back()->write_file("/tmp");
        }



    } catch (const std::string &error) {
        std::cerr << error << std::endl;
    }


    return cal_entries;
}

