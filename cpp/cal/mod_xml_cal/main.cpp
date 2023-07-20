#include "mt_base.h"
#include <iostream>
#include <iomanip>
#include <filesystem>
#include <memory>

#include "base_xml.h"
#include "../../xml/tinyxml2/tinyxml2.h"

void set_xml_dvalue(tinyxml2::XMLElement *top_node, const std::string node, const double &value, std::string *attr = nullptr, const std::string attr_name = "") {
    auto new_node = top_node->FirstChildElement(node.c_str());
    if (new_node != nullptr) {
        std::stringstream sstr;
        sstr.precision(8);

        sstr << std::scientific;
        sstr << std::uppercase;
        sstr << value;
        new_node->SetText( sstr.str().c_str());
    }

}
int main(int argc, char **argv)
{
    using namespace std;
    namespace fs = std::filesystem;

    auto exec_path = std::filesystem::path(argv[0]);


    double ampl_mul = 1.0;
    double phase_mul = 1.0;
    double ampl_add = 0;
    double phase_add = 0;
    int64_t serial = 0;
    std::string sensor = "";
    auto chopper = ChopperStatus::off;
    std::string messages;
    size_t count = 0;



    fs::path outdir;

    unsigned l = 1;
    while (argc > 1 && (l < unsigned(argc)) && *argv[l] == '-') {
        std::string marg(argv[l]);

        if (marg.compare("-sensor") == 0) {
            sensor = std::string(argv[++l]);
        }
        if (marg.compare("-serial") == 0) {
            serial = abs(atoi(argv[++l]));
        }
        if (marg.compare("-chopper") == 0) {
            std::string chop = std::string(argv[++l]);
            if (chop == "on") chopper = ChopperStatus::on;
        }
        if (marg.compare("-ampl_mul") == 0) {
            ampl_mul = std::stod(argv[++l]);
        }
        if (marg.compare("-phase_mul") == 0) {
            phase_mul = stod(argv[++l]);
        }

        if (marg.compare("-ampl_add") == 0) {
            ampl_add = std::stod(argv[++l]);
        }
        if (marg.compare("-phase_mul") == 0) {
            phase_add = stod(argv[++l]);
        }

        if ((marg.compare("-help") == 0) || (marg.compare("--help") == 0)) {
            std::cout << "modify caldat inside XML file" << std::endl;
            std::cout << "-ampl_add d" << std::endl;
            std::cout << "-ampl_mul d"<< std::endl;
            std::cout << "-phase_add d" << std::endl;
            std::cout << "-phase_mul d "<< std::endl;
            std::cout << "-sensor string" << std::endl;
            std::cout << "-serial n" << std::endl;
            std::cout << " xmlfile" << std::endl;
            return EXIT_FAILURE;
        }
        else if (marg.compare("-") == 0) {
            std::cerr << "\nunrecognized option " << argv[l] << std::endl;
            return EXIT_FAILURE;
        }
        ++l;
    }

    fs::path filename = argv[argc-1];


    auto tir = std::make_shared<tinyxml2::XMLDocument>();

    if (!fs::exists(filename)) {
        std::cerr << "no xml file loaded";
        return EXIT_FAILURE;
    }

    bool loaded = tir->LoadFile(filename.string().c_str());
    if (loaded) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << ":: error loading XML file " << filename;
        throw err_str.str();
    }
    auto proot = tir->RootElement(); // that is the envelope, mostly "measurement"
    if (proot == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << "::Root Element XML_ERROR_FILE_READ_ERROR" << filename;
        throw err_str.str();
    }

    auto pscal_sens = open_node(proot, "calibration_sensors", true);
    if (pscal_sens == nullptr) {
        std::ostringstream err_str(__func__, std::ios_base::ate);
        err_str << "::calibration_sensors entry not there, " << filename;
        throw err_str.str();
    }
    auto pchan = open_node(pscal_sens, "channel");

    while (pchan) {
        int id = -1;
        int old_id = id;
        pchan->QueryIntAttribute("id", &id);
        if (old_id != id) {
            std::ostringstream message(messages, std::ios_base::ate); // this inside a thread, try bundle output
            message << "sensor for channel: " << id << " -> ";
            auto pca = open_node(pchan, "calibration");
            try {
                auto pci = open_node(pca, "calibrated_item");
                std::string xsensor(xml_svalue(pci, "ci"));
                int64_t xserial = xml_ivalue(pci, "ci_serial_number");
                if (xserial != INT64_MAX) message << xsensor << " " << xserial << " detected";

                if ((xserial == serial) && (xsensor == sensor)) {

                    std::cout << "changing " << sensor << " serial: " << serial << std::endl;

                    auto f_unit = std::make_unique<std::string>();
                    auto a_unit = std::make_unique<std::string>();
                    auto p_unit = std::make_unique<std::string>();

                    auto cd = open_node(pca, "caldata", true);
                    while (cd) {
                        if (cd != nullptr) {
                            std::string strchp("ukn");
                            const char *cchopper = cd->Attribute("chopper");
                            if (cchopper != nullptr) {
                                strchp = std::string(cchopper);
                            }
                            double a = xml_dvalue(cd, "c2", a_unit.get(), "unit");
                            double p = xml_dvalue(cd, "c3", p_unit.get(), "unit");
                            auto aa = a;
                            auto pp = p;
                            if (ampl_mul != 1.0) a *= ampl_mul;
                            if (ampl_add != 0.0) a += ampl_add;

                            if (phase_mul != 1.0) p *= phase_mul;
                            if (phase_add != 0.0) p += phase_add;

                            if ( (a != DBL_MAX) && (p != DBL_MAX) ) {
                                if ( (strchp == "on") && (chopper == ChopperStatus::on) ) {
                                    if (aa != a) {
                                        set_xml_dvalue(cd, "c2",  a);
                                        count++;
                                    }
                                    if (pp != p) {
                                        set_xml_dvalue(cd, "c2",  a);
                                        count++;
                                    }

                                }
                                if ( (strchp == "off") && (chopper == ChopperStatus::off) ) {
                                    if (aa != a) {
                                        set_xml_dvalue(cd, "c2",  a);
                                        count++;
                                    }
                                    if (pp != p) {
                                        set_xml_dvalue(cd, "c2",  a);
                                        count++;
                                    }

                                }
                            }
                        }

                        cd = cd->NextSiblingElement("caldata");
                    }
                }

            }



            catch (const std::string &error) {
                std::cerr << message.str() << std::endl;
                std::cerr << error << std::endl;
                std::cerr << "ignore in case this is E" << std::endl;
            }
        }
        pchan = pchan->NextSiblingElement("channel");
    }

    if (count) tir->SaveFile(filename.string().c_str(), false);


    std::cout  << endl << "finish write " << endl;



    return EXIT_SUCCESS;
}
