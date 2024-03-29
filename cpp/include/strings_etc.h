#ifndef STRINGS_ETC_H
#define STRINGS_ETC_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <locale>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace mstr {

/*!
 * @brief mystod converts a string to a double; std::stod does not wok in debug mode (March 2024)
 * @param s
 * @return
 */
inline double mystod(const std::string &s) {
  std::stringstream ss(s);
  double d;
  ss >> d;
  return d;
}

static const double zero_frac = 1.0 / 4194304.0; // 4194304 is 4 MHz sample frequency;

inline void removeTrailingCharacters(std::string &str, const char charToRemove) {
  str.erase(str.find_last_not_of(charToRemove) + 1, std::string::npos);
}

inline void removeLeadingCharacters(std::string &str, const char charToRemove) {
  str.erase(0, std::min(str.find_first_not_of(charToRemove), str.size() - 1));
}

/*!
 * \brief rtrim remove trailing empty spaces from a string
 * \param s
 * \return
 */
inline std::string rtrim(const std::string &s) {
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
inline std::string ltrim(const std::string &s) {
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
inline std::string trim(const std::string &s) {
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
inline std::string simplify(const std::string &s, const bool remove_underscores = false) {
  bool next = false;
  bool has_first_char = false;
  std::string str;
  str.reserve(s.size());
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (!isspace(s[i])) {
      if (remove_underscores) {
        if (s[i] != '_')
          str += s[i];
      } else
        str += s[i];
      next = false;
      has_first_char = true;
    } else {
      if (!next && has_first_char)
        str += ' ';
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
inline std::string brtrim(const std::string &s) {

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
inline std::string bltrim(const std::string &s) {
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
inline std::string btrim(const std::string &s) {
  std::string str(brtrim(s));
  return bltrim(str);
}

/*!
 * \brief clean_b_str
 * \param s string
 * \return a clean string from binary file, free of "\x0" and trimmed
 */
inline std::string clean_b_str(std::string const &str) {

  std::string strbc;

  // nothing inside - chars starting with NULL terminator;
  if (str.at(0) == '\x0')
    return strbc;

  // copy until next NULL Terminator
  for (const auto &ch : str) {
    if (ch != '\x0')
      strbc.push_back(ch);
    else
      break;
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
inline std::string clean_bc_str(const char *c, size_t n) {
  std::string str(c, n);
  std::string strbc;

  // nothing inside - chars starting with NULL terminator;
  if (str.at(0) == '\x0')
    return strbc;

  // copy until next NULL Terminator
  for (const auto &ch : str) {
    if (ch != '\x0')
      strbc.push_back(ch);
    else
      break;
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
    } else
      return false;
  } else if (str.find(search) != std::string::npos) {
    return true;
  }

  return false;
}

inline std::string string_replace(const std::string &in, const std::string &search, const std::string &replace) {
  std::string s = in;
  size_t pos = 0;
  while (pos += replace.length()) {
    pos = s.find(search, pos);
    if (pos == std::string::npos) {
      break;
    }
    s.replace(pos, search.length(), replace);
  }

  return s;
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
  } else if (str.compare(search) == 0) {
    return true;
  }

  return false;
}

/*!
 * \brief BoolToString
 * \param b
 * \return returns true and false instead of "1" and "0"
 */
inline const char *BoolToString(const bool b) {
  return b ? "true" : "false";
}

inline const char *IntToString(const int64_t b) {
  return b ? "true" : "false";
}

inline const char *BoolToChopper(const bool b) {
  return b ? "on" : "off";
}

inline const char *IntToChopper(const int64_t b) {
  return b ? "on" : "off";
}

inline std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

inline std::vector<std::string> split(const std::string &s, const std::string delim) {

  std::vector<std::string> elems;

  size_t pos_start = 0, pos_end, delim_len = delim.size();
  std::string token;

  while ((pos_end = s.find(delim, pos_start)) != std::string::npos) {
    token = s.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + delim_len;
    elems.push_back(token);
  }

  elems.push_back(s.substr(pos_start));

  return elems;
}

/*!
 * \brief sample_rate_to_str converts 256 to 256 and Hz, 0.25 to 4 and s; if numbers results in to fractions and round_f_or_s is true return value is != 0
 * \param sample_rate input
 * \param f_or_s sampe rate either as Hz or s as ouput
 * \param unit either "Hz" or "s"
 * \param round_f_or_s round - 0 in case of flawless conversion; example: 4.00001Hz may be a numrical error, 4Hz wanted
 * \return difference between rounded and not rounded sample rate; should be zero for most cases; if not you must take a decision
 */
inline double sample_rate_to_str(const double &sample_rate, double &f_or_s, std::string &unit, const bool round_f_or_s = false) {

  double diff_s;
  if (sample_rate > 0.999999) {
    f_or_s = sample_rate;
    diff_s = sample_rate;
    unit = "Hz";
  } else {
    f_or_s = 1.0 / sample_rate;
    diff_s = 1.0 / sample_rate;
    unit = "s";
  }

  if (round_f_or_s) {
    f_or_s = round(f_or_s); // round for the nearest integral type; 5.5 -> 6, 5.8 -> 6, 5.001 -> 5
  }
  if (!round_f_or_s)
    return 0.0;

  return f_or_s - diff_s; // would be 0.001 for 5.001 input
}

inline double str_to_sample_rate(const std::string &srate) {
  std::string snum;
  std::string sunit;
  double rate = 0.0;
  int isok = 0;

  for (const auto &c : srate) {
    if (std::isdigit(c))
      snum.push_back(c);
    if (std::isalpha(c))
      sunit.push_back(c);
  }

  try {
    rate = mystod(snum);
    if (sunit == "Hz")
      isok = 1;
    if (sunit == "s")
      isok = 2;
  } catch (...) {
    rate = 0.0;
    return rate;
  }
  if (isok == 1)
    return rate;
  else if (isok == 2)
    return 1.0 / rate;
  return rate;
}

inline std::string sample_rate_to_str_simple(const double &sample_rate, bool add_space_for_s = false) {
  double f_or_s = 0;
  std::string unit;
  bool round_f_or_s = true;
  double must_be_zero = mstr::sample_rate_to_str(sample_rate, f_or_s, unit, round_f_or_s);
  if (must_be_zero != 0.0)
    std::cerr << "sample_rate_to_str_simple: rounding error: " << must_be_zero << std::endl;

  std::string sval;
  sval = std::to_string(static_cast<uint32_t>(f_or_s));
  sval += unit;
  if (add_space_for_s) {
    if (unit == "s")
      sval += " ";
  }
  return sval;
}

inline std::string f_to_string(const double &f, bool add_space_for_s = false) {
  std::stringstream ss;
  if ((f < 1) && add_space_for_s)
    ss << (1. / f) << " "
       << "s ";
  else if (f < 1)
    ss << (1. / f) << " "
       << "s";
  else
    ss << f << "Hz";
  return ss.str();
}

template <class T>
/*!
 * \brief zero_fill_field fills field padded with zeros: 4 -> 0004, -4 -> -004
 * \param num INTEGER type
 * \param width field width
 * \return string with at least width characters, starting with 0 or -0 in case
 */
inline std::string zero_fill_field(const T num, unsigned int width) {
  std::ostringstream ostr;

  if (num < 0) {
    ostr << '-';
    --width;
  }
  ostr << std::setfill('0') << std::setw(width) << (num < 0 ? -num : num);

  return ostr.str();
}

/*!
 * \brief iso_8601_str_d_str_d cats ISO date and ISO time together with optional fraction of seconds, in case fraction is > 10E-12 secs
 * \param date like 2012-09-14
 * \param time like 14:32:45
 * \param fracs positive numberlike 0.012 and greater as zero_frac
 * \return
 */
inline std::string iso_8601_str_d_str_d(const std::string &date, const std::string &time, const double &fracs = 0.0) {
  std::string dt = date + "T" + time;
  std::string sfracs;
  if (fracs > zero_frac) {
    sfracs = std::to_string(fracs);
    sfracs.erase(0, 1);
    removeTrailingCharacters(sfracs, '0');
    if ((sfracs.find('.') != sfracs.npos) && sfracs.size() == 1)
      sfracs.clear();
    dt += sfracs;
  }
  return dt;
}

/*!
 * \brief iso_8601_str_d append fractions of seconds
 * \param datetime ISO 8601 str
 * \param fracs accepted if > zero_frac - otherwise a roundind error is assumed
 * \return
 */
inline std::string iso_8601_str_d(const std::string &datetime, const double &fracs = 0.0) {
  std::string sfracs;
  if (fracs > zero_frac) {
    sfracs = std::to_string(fracs);
    sfracs.erase(0, 1);
    removeTrailingCharacters(sfracs, '0');
    if ((sfracs.find('.') != sfracs.npos) && sfracs.size() == 1)
      sfracs.clear();
    return (datetime + sfracs);
  }
  return datetime;
}

/*!
 * \brief time_t_iso_8601_str parses an ISO dat string like 2022-01-18T17:30:00.4 or 2022-01-18T17:30:00
 * \param datetime ISO date string w or /wo ending Z os seconds like 00 or 00.4
 * \return time_t struct; time_t can NOT hold fractions of seconds
 */
inline time_t time_t_iso_8601_str(const std::string &datetime) {
  // maybe not thread safe?
  struct tm tt = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  double dseconds = 0.0;
  if (sscanf(datetime.c_str(), "%04d-%02d-%02dT%02d:%02d:%lf",
             &tt.tm_year, &tt.tm_mon, &tt.tm_mday,
             &tt.tm_hour, &tt.tm_min, &dseconds) != 6)
    return -1;
  double fs = 0.0;
  // double fracpart = std::modf (dseconds , &fs);
  tt.tm_sec = int(fs);
  tt.tm_mon -= 1;
  tt.tm_year -= 1900;
  tt.tm_isdst = 0;
  return mktime(&tt) - timezone;
}

inline time_t time_t_iso_8601_str_fracs(const std::string &datetime, double &remaining_fracs) {
  // maybe not thread safe?
  struct tm tt = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  double dseconds;
  if (sscanf(datetime.c_str(), "%04d-%02d-%02dT%02d:%02d:%lf",
             &tt.tm_year, &tt.tm_mon, &tt.tm_mday,
             &tt.tm_hour, &tt.tm_min, &dseconds) != 6)
    return -1;
  double fs;
  remaining_fracs = std::modf(dseconds, &fs);
  tt.tm_sec = int(fs);
  tt.tm_mon -= 1;
  tt.tm_year -= 1900;
  tt.tm_isdst = 0;
  return mktime(&tt) - timezone;
}

inline std::string iso8601_time_t(const time_t &ti, const int iso_0_date_1_time_2 = 0, const double &fracs = 0.0) {
  // maby not thread safe?
  struct tm tt = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  std::string sfracs;
  std::string date;
  std::string time;
  tt = *std::gmtime(&ti);
  //    date.clear();
  //    time.clear();
  date += std::to_string(tt.tm_year + 1900) + "-";
  date += zero_fill_field(tt.tm_mon + 1, 2) + "-";
  date += zero_fill_field(tt.tm_mday, 2);
  time += zero_fill_field(tt.tm_hour, 2) + ":";
  time += zero_fill_field(tt.tm_min, 2) + ":";

  time += zero_fill_field(tt.tm_sec, 2);
  if (std::abs(fracs) > zero_frac) {
    sfracs = std::to_string(fracs);
    sfracs.erase(0, 1);
    removeTrailingCharacters(sfracs, '0');
    time += sfracs;
  }
  if (iso_0_date_1_time_2 == 1)
    return date;
  if (iso_0_date_1_time_2 == 2)
    return time;
  ;
  return date + "T" + time;
}

inline std::string get_date_from_iso8601(const std::string &datetime) {
  auto splits = mstr::split(datetime, 'T');
  if (splits.size() > 1)
    return splits.at(0);
  return std::string();
}
inline std::string get_time_from_iso8601(const std::string &datetime) {
  auto splits = mstr::split(datetime, 'T');
  if (splits.size() > 1) {
    auto splitss = mstr::split(splits.at(1), '.');
    if (splitss.size())
      return splitss.at(0);
  }
  return std::string();
}

inline std::string get_time_fracs_from_iso8601(const std::string &datetime) {
  auto splits = mstr::split(datetime, 'T');
  if (splits.size() > 1)
    return splits.at(1);
  return std::string();
}

inline double get_fracs_from_iso8601(const std::string &datetime) {
  auto splits = mstr::split(datetime, '.');
  if (splits.size() > 1) {
    std::string s("0.");
    s += splits.at(1);
    return std::stold(s);
  }
  return 0.0;
}

inline std::string measdir_time(const time_t &ti) {
  struct tm tt = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  tt = *std::gmtime(&ti);
  std::string date("meas_");
  std::string time("_");
  date += std::to_string(tt.tm_year + 1900) + "-";
  date += mstr::zero_fill_field(tt.tm_mon + 1, 2) + "-";
  date += mstr::zero_fill_field(tt.tm_mday, 2);
  time += mstr::zero_fill_field(tt.tm_hour, 2) + "-";
  time += mstr::zero_fill_field(tt.tm_min, 2) + "-";
  time += mstr::zero_fill_field(tt.tm_sec, 2);

  return date + time;
}

inline void date_and_time(const time_t &ti, std::string &date, std::string &time) {
  struct tm tt = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  tt = *std::gmtime(&ti);
  date.clear();
  time.clear();
  date += std::to_string(tt.tm_year + 1900) + "-";
  date += mstr::zero_fill_field(tt.tm_mon + 1, 2) + "-";
  date += mstr::zero_fill_field(tt.tm_mday, 2);
  time += mstr::zero_fill_field(tt.tm_hour, 2) + ":";
  time += mstr::zero_fill_field(tt.tm_min, 2) + ":";
  time += mstr::zero_fill_field(tt.tm_sec, 2);
}

inline bool isdigit_first_char(const std::string &str) {
  return (str.at(0) == '-' || std::isdigit(str.at(0)) || str.at(0) == '+');
}

/*!
 * \brief run2string
 * \param run
 * \return run_001 or run_012 and so on
 */
inline std::string run2string(const auto &run) {
  std::string srun("run_");
  return srun + mstr::zero_fill_field(run, 3);
}

/*!
 * \brief string2run
 * \param srun run_001 or run_012 and so on
 * \return
 */
inline size_t string2run(const std::string &srun) {
  // split the string at the last '_' and take the rest
  std::string ssrun = srun.substr(srun.find_last_of('_') + 1);
  if (ssrun.empty() || (ssrun.size() > 6))
    return SIZE_MAX;
  return size_t(std::stoul(ssrun));
}

/*!
 * \brief field_width_right_adjusted_freqs_periods - this is for frequencies wich can be converted to INT!!!!
 * \param fs
 * \return field width
 */
inline std::vector<std::stringstream> field_width_right_adjusted_freqs_periods(const std::vector<double> &fs) {
  std::vector<std::stringstream> sss(fs.size());
  size_t i = 0;
  size_t max_l = 0;
  std::string unit;
  for (const auto &f : fs) {
    int add;
    if (f > 0.99999) {
      add = int(round(f));
    } else {
      add = int(round(1.0 / f));
    }

    sss[i++] << add << unit;
  }

  for (const auto &s : sss) {
    if (s.str().size() > max_l)
      max_l = s.str().size();
  }
  sss.clear();
  sss.resize(fs.size());

  i = 0;
  for (const auto &f : fs) {
    int add;
    if (f > 0.99999) {
      add = int(round(f));
      unit = "Hz";
    } else {
      add = int(round(1.0 / f));
      unit = "s ";
    }

    sss[i++] << std::setw(max_l) << add << unit;
  }

  return sss;
}
template <std::integral T>
std::vector<std::stringstream> field_width_right_adjusted_ints(const std::vector<T> &vals) {
  std::vector<std::stringstream> sss(vals.size());
  size_t i = 0;
  size_t max_l = 0;
  for (const auto &v : vals) {
    sss[i++] << v;
  }

  for (const auto &s : sss) {
    if (s.str().size() > max_l)
      max_l = s.str().size();
  }
  sss.clear();
  sss.resize(vals.size());

  i = 0;
  for (const auto &v : vals) {
    sss[i++] << std::setw(max_l) << v;
  }

  return sss;
}

template <std::floating_point T>
std::vector<std::stringstream> field_width_right_adjusted_doubles(const std::vector<T> &vals, const double low = 0.01, const double high = 10000, size_t sci_prec = 4) {
  std::vector<std::stringstream> sss(vals.size());

  bool is_sci = false;
  auto mima = std::minmax_element(vals.cbegin(), vals.cend());

  if (*mima.first < low)
    is_sci = true;
  if (*mima.second < high)
    is_sci = true;

  size_t i = 0;
  size_t max_l = 0;
  for (const auto &v : vals) {
    if (is_sci)
      sss[i++] << std::scientific << std::setprecision(sci_prec) << v;
    else
      sss[i++] << v;
  }

  for (const auto &s : sss) {
    if (s.str().size() > max_l)
      max_l = s.str().size();
  }
  sss.clear();
  sss.resize(vals.size());

  i = 0;
  for (const auto &v : vals) {
    if (is_sci)
      sss[i++] << std::setw(max_l) << std::scientific << std::setprecision(sci_prec) << v;
    else
      sss[i++] << std::setw(max_l) << v;
  }

  return sss;
}

inline std::string escape_undersorce(const std::string &in) {
  std::string out;
  out.reserve(in.size() + 8); // in most cases we have 4 '_' to be replaced by '\\_'
  std::string replace = "\\_";
  char origin = '_';

  for (const auto &c : in) {
    if (c != origin)
      out += c;
    else
      out += replace;
  }
  return out;
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
