#ifndef STRINGS_ETC_H
#define STRINGS_ETC_H

#include <string>
#include <string_view>
#include <iostream>
#include <iterator>
#include <iomanip>
#include <locale>
#include <algorithm>
#include <vector>
#include <cmath>


namespace mstr {

static void removeTrailingCharacters(std::string &str, const char charToRemove) {
    str.erase (str.find_last_not_of(charToRemove) + 1, std::string::npos );
}

static void removeLeadingCharacters(std::string &str, const char charToRemove) {
    str.erase(0, std::min(str.find_first_not_of(charToRemove), str.size() - 1));
}

/*!
 * \brief rtrim remove trailing empty spaces from a string
 * \param s
 * \return
 */
inline std::string rtrim(const std::string& s) {
    std::string ws(" \t\f\v\n\r");
    auto found = s.find_last_not_of(ws);
    if (found != std::string::npos) {
        return s.substr(0, found + 1);
    }
    return s;
}

/*!
 * \brief ltrim remove leading empty spaces from a string
 * \param s
 * \return
 */
inline std::string ltrim(const std::string& s) {
    std::string ws(" \t\f\v\n\r");
    auto found = s.find_first_not_of(ws);
    if (found != std::string::npos) {
        return s.substr(found);
    }
    return std::string();
}

/*!
 * \brief trim trims a string on both ends
 * \param s
 * \return
 */
inline std::string trim(const std::string& s) {
    std::string str(rtrim(s));
    return ltrim(str);
}

/*!
 * \brief simplify removes all leading and trailing whitespaces from a string; additionally all whitespaces INSIDE will be replaced with a single space
 * underscores can be removed: this is needed when you create atss file names where the underscore is a separator and
 * systemname= ADU_08e will break the file name tags "_"
 * \param s
 * \return
 */
inline std::string simplify(const std::string& s, const bool remove_underscores = false) {
    bool next = false;
    bool has_first_char = false;
    std::string str;
    str.reserve(s.size());
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (!isspace(s[i])) {
            if (remove_underscores) {
                if (s[i] != '_') str +=s[i] ;
            }
            else str +=s[i];
            next = false;
            has_first_char = true;
        }
        else {
            if (!next && has_first_char) str += ' ';
            next = true;
        }
    }
    return rtrim(str);
}

/*!
 * \brief brtrim remove following terminators from a string - may happen when the file was a BINARY
 * \param s
 * \return
 */
inline std::string brtrim(const std::string& s) {

    auto found = s.find_last_not_of('\x0');
    if (found != std::string::npos) {
        return s.substr(0, found + 1);
    }
    return s;
}

/*!
 * \brief bltrim remove leading terminators from a string - may happen when the file was a BINARY
 * \param s
 * \return
 */
inline std::string bltrim(const std::string& s) {
    auto found = s.find_first_not_of('\x0');
    if (found != std::string::npos) {
        return s.substr(found);
    }
    return std::string();
}

/*!
 * \brief btrim bltrim remove leading and trailing terminators from a string - may happen when the file was a BINARY
 this does NOT take care for \x0 inside a string - especially when made from char* binary
 * \param s
 * \return
 */
inline std::string btrim(const std::string& s) {
    std::string str(brtrim(s));
    return bltrim(str);
}

/*!
 * \brief clean_b_str
 * \param s string
 * \return a clean string from binary file, free of "\x0" and trimmed
 */
inline std::string  clean_b_str(std::string const &str) {

    std::string strbc;

    // nothing inside - chars starting with NULL terminator;
    if (str.at(0) == '\x0') return strbc;

    // copy until next NULL Terminator
    for (const auto &ch : str) {
        if (ch != '\x0') strbc.push_back(ch);
        else break;
    }
    // remove trailers and beginners white space 
    return trim(strbc);
}

/*!
 * \brief clean_bc_str cleans a cstring form all "\x0"
 * \param c char*
 * \param n size_t n characters to use for string
 * \return string free of "\x0" and trimmed
 */
inline std::string  clean_bc_str(const char* c, size_t n) {
    std::string str(c, n);
    std::string strbc;

    // nothing inside - chars starting with NULL terminator;
    if (str.at(0) == '\x0') return strbc;

    // copy until next NULL Terminator
    for (const auto &ch : str) {
        if (ch != '\x0') strbc.push_back(ch);
        else break;
    }
    // remove trailers and beginners white space 
    return trim(strbc);

}

/*!
 * \brief begins_with test if string begins with prefix - like MFS0612334.txt MFS06
 * \param str string to test
 * \param prefix what to find at the beginnig
 * \return
 */
inline bool begins_with(std::string const &str, std::string const &prefix) {

    return (str.size() >= prefix.size()) && (0 == str.compare(0, prefix.size(), prefix));

}

inline bool ends_with(std::string const &str, std::string const &suffix) {

    return (str.size() >= suffix.size()) && (0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix));

}

/*!
 * \brief contains search a needle in the haystack
 * \param str haystack
 * \param search needle
 * \param case_sensitive
 * \return true if contains otherwise false
 */
inline bool contains(std::string const &str, std::string const &search, bool case_sensitive = true) {

    if (!case_sensitive) {
        auto str1(str);
        auto search1(search);
        std::transform(str1.begin(), str1.end(), str1.begin(), ::tolower);
        std::transform(search1.begin(), search1.end(), search1.begin(), ::tolower);
        if (str1.find(search1) != std::string::npos) {
            return true;
        }
        else return false;
    }
    else if (str.find(search) != std::string::npos) {
        return true;
    }

    return false;
}

/*!
 * \brief compare full comparsion
 * \param str
 * \param search
 * \param case_sensitive
 * \return
 */
inline bool compare(std::string_view const &str, std::string_view const &search, bool case_sensitive = true) {

    if (case_sensitive) {
        std::string str1(str);
        std::string search1(search);
        std::transform(str.begin(), str.end(), str1.begin(), ::tolower);
        std::transform(search.begin(), search.end(), search1.begin(), ::tolower);
        if (str1.compare(search1) == 0) {
            return true;
        }
        return false;
    }
    else if (str.compare(search) == 0) {
        return true;
    }

    return false;
}

/*!
 * \brief BoolToString
 * \param b
 * \return returns true and false instead of "1" and "0"
 */
inline const char* BoolToString(const bool b)
{
    return b ? "true" : "false";
}

inline const char* IntToString(const int64_t b)
{
    return b ? "true" : "false";
}

inline const char* BoolToChopper(const bool b)
{
    return b ? "on" : "off";
}

inline const char* IntToChopper(const int64_t b)
{
    return b ? "on" : "off";
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, const std::string delim) {

    std::vector<std::string> elems;

    size_t pos_start = 0, pos_end, delim_len = delim.size();
    std::string token;

    while ((pos_end = s.find (delim, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        elems.push_back (token);
    }

    elems.push_back (s.substr (pos_start));

    return elems;
}


/*!
 * \brief sample_rate_to_str converts 256 to 256 and Hz, 0.25 to 4 and s; if numbers results in to fractions and round_f_or_s is true return value is != 0
 * \param sample_rate input
 * \param f_or_s sampe rate either as Hz or s as ouput
 * \param unit either "Hz" or "s"
 * \param round_f_or_s round - 0 in case of flawless conversion
 * \return difference between rounded and not rounded sample rate; should be zero for most cases; if not you must take a decision
 */
double sample_rate_to_str(const double& sample_rate, double& f_or_s, std::string &unit, const bool round_f_or_s = false) {

    double diff_s;
    if (sample_rate > 0.999999 ) {
        f_or_s = sample_rate;
        diff_s = sample_rate;
        unit = "Hz";
    }
    else {
        f_or_s = 1.0 / sample_rate;
        diff_s = 1.0 / sample_rate;
        unit = "s";
    }

    if (round_f_or_s) {
        f_or_s = round(f_or_s);

    }
    if (!round_f_or_s) return 0.0;

    return f_or_s - diff_s;
}

std::string sample_rate_to_str_simple(const double& sample_rate) {
    double f_or_s;
    std::string unit;
    bool round_f_or_s = true;
    double must_be_zero = mstr::sample_rate_to_str(sample_rate, f_or_s, unit, round_f_or_s);
    std::string sval;
    sval = std::to_string(static_cast<std::uint32_t>(f_or_s));
    sval += unit;
    return sval;
}

template <class T>
/*!
 * \brief zero_fill_field fills field padded with zeros: 4 -> 0004, -4 -> -004
 * \param num INTEGER type
 * \param width field width
 * \return string with at least width characters, starting with 0 or -0 in case
 */
std::string zero_fill_field(const T num, unsigned int width)
{
    std::ostringstream ostr;

    if (num < 0) {
        ostr << '-';
        --width;
    }
    ostr << std::setfill('0') << std::setw(width) << (num < 0 ? -num : num);

    return ostr.str();
}

std::string iso_8601_str_str(const std::string& date, const std::string& time, const double& fracs = 0.0, const std::string separator = "T", const std::string TZ = "Z"  ) {
        std::string dt = date + separator + time;
        std::string sfracs;
        if (fabs(fracs) > 1E-12) {
            sfracs = std::to_string(fracs);
            sfracs.erase(0,1);
            removeTrailingCharacters(sfracs, '0');
            dt += sfracs;
        }
        return dt + TZ;
}
/*!
 * \brief string_iso8601_time_t parses an ISO dat string like 2022-01-18T17:30:00.4Z or 2022-01-18T17:30:00Z or 2022-01-18T17:30:00.4
 * \param date ISO date string w or /wo ending Z os seconds like 00 or 00.4
 * \param fracs in case it has fraction of secnod, will be set here
 * \return time_t struct; time_t can NOT hold fractions of seconds
 */
time_t string_iso8601_time_t(const std::string date, double fracs) {
    struct tm tt = {0};
    double dseconds;
    if (sscanf(date.c_str(), "%04d-%02d-%02dT%02d:%02d:%lfZ",
               &tt.tm_year, &tt.tm_mon, &tt.tm_mday,
               &tt.tm_hour, &tt.tm_min, &dseconds) != 6)
        return -1;
    double seconds;
    fracs = fabs(std::modf(dseconds, &seconds));
    if (fracs < 1E-12) fracs = 0.0;
    tt.tm_sec   = (int)seconds;
    tt.tm_mon  -= 1;
    tt.tm_year -= 1900;
    tt.tm_isdst = 0;
    return mktime(&tt) - timezone;
}

std::string time_t_iso8601_utc(const time_t &ti, std::string &date, std::string &time, const double& fracs = 0.0, const std::string separator = "T", const std::string TZ = "Z" ) {
    struct tm tt = {0};
    std::string sfracs;
    tt = *std::gmtime(&ti);
    date += std::to_string(tt.tm_year + 1900) + "-";
    date += zero_fill_field(tt.tm_mon + 1, 2) + "-";
    date += zero_fill_field(tt.tm_mday, 2);
    time += zero_fill_field(tt.tm_hour, 2) + ":";
    time += zero_fill_field(tt.tm_min, 2) + ":";

    time += zero_fill_field(tt.tm_sec, 2);
    if (fabs(fracs) > 1E-12) {
        sfracs = std::to_string(fracs);
        sfracs.erase(0,1);
        removeTrailingCharacters(sfracs, '0');
    }
    return date + separator + time + sfracs + TZ;

}

bool isdigit_first_char(const std::string &str)
{
    return (str.at(0) == '-' || std::isdigit(str.at(0)) || str.at(0) == '+');
}


/*
time_t parseiso8601utc(const char *date) {
  struct tm tt = {0};
    double seconds;
    if (sscanf(date, "%04d-%02d-%02dT%02d:%02d:%lfZ",
               &tt.tm_year, &tt.tm_mon, &tt.tm_mday,
               &tt.tm_hour, &tt.tm_min, &seconds) != 6)
        return -1;
    tt.tm_sec   = (int) seconds;
    tt.tm_mon  -= 1;
    tt.tm_year -= 1900;
    tt.tm_isdst =-1;
    return mktime(&tt) - timezone;
}
*/

} // end namespace mstr
#endif // STRINGS_ETC_H
