#ifndef TINYXMLWRITER_H
#define TINYXMLWRITER_H

#include <complex>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


class tinyxmlwriter {
public:
    tinyxmlwriter(const bool write_header = false, const std::filesystem::path &filename = "") {
        if (write_header) {
            this->header();
        }

        if (!filename.empty()) {
            this->filename = filename;
        }
    }
    ~tinyxmlwriter() {
        if (this->file.is_open())
            this->file.close();
    }

    void header() {
        this->xml << "<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>" << std::endl;
    }
    void indent() {
        for (size_t i = 0; i < this->stack.size(); ++i)
            this->xml << this->indentation;
    }

    /*!
   * \brief push opens a section node
   * \param element
   */
    void push(const std::string &element) {
        this->indent();
        this->xml << "<" << element << ">" << std::endl;
        this->stack.push_back(element);
    }

    /*!
   * \brief push
   * \param element
   * \param attributes string of attributes
   */
    void push(const std::string &element, const std::string &str_attributes) {
        this->indent();
        this->xml << "<" << element << " " << str_attributes << ">" << std::endl;
        this->stack.push_back(element);
    }

    /*!
   * \brief push for typical: "channel id=3", opens a section node
   * \param element like channel
   * \param str_attribute like id
   * \param int_value like 3
   * \return
   */
    void push(const std::string &element, const std::string &str_attribute, const auto &attribute_value) {
        this->indent();
        //std::cout << "atr val " << attribute_value << std::endl;
        //std::cout << "<" << element << " " << str_attribute << "=\"" << attribute_value << "\">" << std::endl;
        this->xml << "<" << element << " " << str_attribute << "=\"" << attribute_value << "\">" << std::endl;
        this->stack.push_back(element);
    }

    /*!
   * \brief pop closes a push with </tag> - optional you can:
   * \param reminder use tag in case you auto comment your source code
   */
    void pop(const std::string &reminder = "") {
        if (stack.size()) {
            std::string element = this->stack.back();
            this->stack.pop_back();
            this->indent();
            if (reminder.size()) {
                if (reminder != element) {
                    std::ostringstream err_str( (std::string("tinyxmlwriter::") + __func__), std::ios_base::ate);
                    err_str << ":: possibly closing error of node (manually): </" << reminder << "> vs auto: </" << element << ">";
                    throw err_str.str();
                }
            }
            this->xml << "</" << element << ">" << std::endl;
            return;
        } else {
            std::ostringstream err_str( (std::string("tinyxmlwriter::") + __func__), std::ios_base::ate);
            err_str << ":: stack is empty, too many pop?: </" << reminder << ">";
            throw err_str.str();
        }
    }

    /*!
   * \brief element
   * \param element
   * \param val
   * \param precision assumes a double
   * \param scientific activate scientific in cas precision is set
   */
    void element(const std::string &element, const auto &val, const uint32_t precision = 0, const bool scientific = false) {
        if (precision) {
            std::stringstream sstr;
            sstr.precision(precision);
            if (scientific) {
                sstr << std::scientific;
                sstr << std::uppercase;
            }
            sstr << val;
            indent();
            xml << "<" << element << ">" << sstr.str() << "</" << element << ">" << std::endl;
        } else { // assuming nothing
            indent();
            xml << "<" << element << ">" << val << "</" << element << ">" << std::endl;
        }
    }

    /*!
   * \brief element with no content
   * \param element
   */
    void element_empty(const std::string &element) {
        indent();
        xml << "<" << element << "/>" << std::endl;
    }

    /*!
   * \brief element_attr element with a single attribute
   * \param element
   * \param str_attribute
   * \param attribute_value
   * \param val
   * \param precision
   * \param scientific
   */
    void element_attr(const std::string &element, const std::string &str_attribute, const auto attribute_value, const auto &val, const uint32_t precision = 0, const bool scientific = false) {
        if (precision) {
            std::stringstream sstr;
            sstr.precision(precision);
            if (scientific) {
                sstr << std::scientific;
                sstr << std::uppercase;
            }
            sstr << val;
            indent();
            xml << "<" << element << " " << str_attribute << "=\"" << attribute_value << "\">" << sstr.str() << "</" << element << ">" << std::endl;
        } else { // assuming nothing
            indent();
            xml << "<" << element << " " << str_attribute << "=\"" << attribute_value << "\">" << val << "</" << element << ">" << std::endl;
        }
    }

    void push_caldata(const auto chopper, const double gain_3, const double gain_2, const double gain_1, const std::complex<double> impedance) {
        this->indent();
        std::string chop = "on";
        if (!chopper)
            chop = "off";
        // <caldata chopper="on" gain_3="0" gain_2="0" gain_1="0" impedance="(0,0)">
        this->xml << "<caldata chopper=\"" << chop << "\" gain_3=\"" << gain_3 << "\" gain_2=\"" << gain_2 << "\" gain_1=\"" << gain_1 << "\" impedance=\"" << impedance << "\"> " << std::endl;
        this->stack.push_back("caldata");
    }

    bool snatch_cal_from_xml(const std::filesystem::path &parent, const std::filesystem::path &xml_filename) {
        if (xml_filename.empty())
            return false;

        if (parent.empty())
            return false;
        std::filesystem::path name(parent);
        name /= xml_filename;
        std::fstream xmlfile;
        xmlfile.open(name, std::ios::in);
        if (!xmlfile.is_open())
            return false;
        std::string line;
        bool in_section = false;
        std::string str_beg("<calibration_sensors>");
        std::string str_end("</calibration_sensors>");

        while (std::getline(xmlfile, line)) {
            if (line.find(str_beg) != std::string::npos) {
                in_section = true;
            }
            if (in_section) {
                this->xml << line << std::endl;
            }
            if (line.find(str_end) != std::string::npos) {
                in_section = false;
            }
        }

        return true;
    }

    bool write_file() {

        this->file.open(this->filename, std::ios::out | std::ios::trunc);
        if (!this->file.is_open()) {
            std::ostringstream err_str( (std::string("tinyxmlwriter::") + __func__), std::ios_base::ate);
            err_str << " can not open XML for writing " << this->filename;
            throw err_str.str();
        }
        this->file << xml.str();
        this->file.close();
        return true;
    }

    std::stringstream xml;          //!< contains the complete XML including indents
    std::string indentation = "  "; //!< contains two spaces as indentation
    std::vector<std::string> stack; //!< contains the stack of indententaion (levels)
private:
    std::fstream file;
    std::filesystem::path filename;
};

#endif // TINYXMLWRITER_H
