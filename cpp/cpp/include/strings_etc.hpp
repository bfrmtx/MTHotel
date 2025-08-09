#ifndef STRINGS_ETC_HPP
#define STRINGS_ETC_HPP
/*!
 * @file strings_etc.hpp
 * @brief Utility functions for string manipulation and other common tasks.
 */
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <format>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

/*!
 * @brief
 */
namespace mstr {

/*!
 * @brief zero frac: round to zero; we assume that values smaller than this are zero if a TIME is given
 * @details this is used to round values to zero in the time series, e.g. if start time fraction is smaller than this value, we set the fraction part to zero.
 */
static const double zero_frac = 1.0 / 1048576.0; // 1048576 is 1 MHz sample frequency (power of two) "0.00000095367431640625"

// ************************   T R I M   F U N C T I O N S   ******************************
/*!
 * @brief Trim leading whitespace from a string.
 * @param s The input string.
 * @return A new string with leading whitespace removed.
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
 * @brief Trim trailing whitespace from a string.
 * @param s The input string.
 * @return A new string with trailing whitespace removed.
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
 * @brief Trim whitespace from both ends of a string.
 * @param s The input string.
 * @return A new string with leading and trailing whitespace removed.
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
inline std::string simplify(const std::string s_in, bool remove_underscores = false) {
  bool next = false;
  bool has_first_char = false;
  std::string s = mstr::trim(s_in); // start with trimmed string
  std::string str;
  str.reserve(s.size());
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (!isspace(s[i])) { // not a whitespace character
      if (remove_underscores) {
        if (s[i] != '_') // copy only if not an underscore
          str += s[i];
      } else
        str += s[i]; // copy the character
      next = false;
      has_first_char = true;
    } else { // is a whitespace character
      if (!next && has_first_char)
        str += ' '; // add a single space if not already added
      next = true;  // next is true now, we have a whitespace added. so next needs to false; this is done in the !isspace check
    }
  }
  // at the end we may have a trailing space, so we trim it
  return rtrim(str);
}

/*!
 * @brief removeTrailingCharacters removes all trailing characters from a string
 * @param str The input string.
 * @param charToRemove The character to remove from the end of the string.
 */

inline void removeTrailingCharacters(std::string &str, const char charToRemove) {
  str.erase(str.find_last_not_of(charToRemove) + 1, std::string::npos);
}

/*!
 * @brief removeLeadingCharacters removes all leading characters from a string
 * @param str The input string.
 * @param charToRemove The character to remove from the beginning of the string.
 */
inline void removeLeadingCharacters(std::string &str, const char charToRemove) {
  str.erase(0, std::min(str.find_first_not_of(charToRemove), str.size() - 1));
}
/*!
 * @brief Check if a filename is valid.
 * @param name The filename to check.
 * @return True if the filename is valid, false otherwise.
 */
inline bool is_valid_filename(const std::string &name) {
  static const std::string forbidden = "/\\:*?\"<>|\n\r\t";
  if (name.empty() || name == "." || name == "..")
    return false;
  for (char c : name) {
    if (c < 32 || forbidden.find(c) != std::string::npos)
      return false;
  }
  return true;
}
/*!
 * @brief Check if a path is a file or a string, which we can append to a path.
 * @details we have either /home/data/file.txt or file.txt
 * @details my_path would be /home/data in this case. if file not exists, we assume I can append "file.txt" to my_path
 * @details in that case we must check that parameter p does not contain a path separator (/ \ : .. etc.) in the string. otherwise we can not append it to my_path
 * @param my_path The filesystem path to check. must be either in path or a simple filename without path.
 * @param p The path to check.
 * @return The path if it is a file, an empty path otherwise.
 */
inline std::filesystem::path path_is_file(const std::filesystem::path &my_path, const std::filesystem::path &p) {
  if (p.empty()) {
    throw std::runtime_error("Path is empty in path_is_file() function.");
  }
  // my_path must be a directory, so we can append p to it
  if (!std::filesystem::is_directory(my_path)) {
    throw std::runtime_error("Path is not a directory: " + my_path.string());
  }
  // check if p is a file, if it is a file, we return it
  if (std::filesystem::exists(p) && std::filesystem::is_regular_file(p)) {
    auto canon = std::filesystem::canonical(p);
    // cannon must contain my_path as parent path
    if (canon.has_parent_path() && canon.parent_path() == my_path) {
      return canon; // p is a file, return its canonical path
    } else {
      throw std::runtime_error("Path is not a file in path_is_file() function: " + p.string() + " is not in " + my_path.string());
    }
  }
  // check if p is a string, which we can append to my_path
  if (p.has_filename() && !p.has_parent_path()) {
    if (!is_valid_filename(p.filename().string())) {
      throw std::runtime_error("Invalid path: " + p.string());
    }
  }
  return my_path / p; // append the filename to my_path
}

// a binary string, allocated as string(size), and getting data from a char* pointer or struct, may contain '\x0' characters
//
// ******* B I N A R Y   S T R I N G S *******
//

/*!
 * @brief Trim trailing null characters from a binary string.
 * @param s The input string.
 * @return A new string with trailing null characters removed.
 */
inline std::string brtrim(const std::string &s) {
  auto found = s.find_last_not_of('\x0');
  if (found != std::string::npos) {
    return s.substr(0, found + 1);
  }
  return std::string();
}

/*!
 * @brief Trim leading null characters from a binary string.
 * @param s The input string.
 * @return A new string with leading null characters removed.
 */
inline std::string bltrim(const std::string &s) {
  auto found = s.find_first_not_of('\x0');
  if (found != std::string::npos) {
    return s.substr(found);
  }
  return s;
}
/*!
 * @brief Trim leading and trailing null characters from a binary string.
 * @param s The input string.
 * @return A new string with leading and trailing null characters removed.
 */
inline std::string btrim(const std::string &s) {
  std::string str(bltrim(s));
  return brtrim(str);
}

/*!
 * @brief Clean a binary string by removing null characters and trimming whitespace.
 * @param str The input string.
 * @return A cleaned string with null characters removed and whitespace trimmed.
 */
inline std::string clean_b_str(std::string const &str) {

  std::string strbc;

  // nothing inside - chars starting with NULL terminator;
  // in binary files, this indicates that the string is empty or has NO VALID DATA, so rest can be garbage
  // this happens if the binary memory was allocated with a size, but not filled with NULL
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

inline std::string clean_bc_str(const char *c, size_t n) {
  std::string str(c, n);
  return clean_b_str(str);
}

// ********************* U T I L I T Y   F U N C T I O N S *********************

/*!
 * @brief Check if a string begins with a given prefix.
 * @param str The input string.
 * @param prefix The prefix to check for.
 * @return True if the string begins with the prefix, false otherwise.
 */
inline bool begins_with(std::string const &str, std::string const &prefix) {
  return (str.size() >= prefix.size()) && (0 == str.compare(0, prefix.size(), prefix));
}

/*!
 * @brief Check if a string ends with a given suffix.
 * @param str The input string.
 * @param suffix The suffix to check for.
 * @return True if the string ends with the suffix, false otherwise.
 */
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

/*!
 * @brief Replace all occurrences of a substring within a string.
 * @param in The input string.
 * @param search The substring to search for.
 * @param replace The substring to replace with.
 * @return A new string with all occurrences of the search substring replaced.
 */
inline std::string string_replace(const std::string &in, const std::string &search, const std::string &replace) {
  std::string s = in;
  size_t pos = 0;
  while ((pos = s.find(search, pos)) != std::string::npos) {
    s.replace(pos, search.length(), replace);
    pos += replace.length(); // Move past the replaced part
  }
  return s;
}

/*!
 * @brief Compare two strings for equality, with optional case sensitivity.
 * @param str The first string.
 * @param search The second string to compare against.
 * @param case_sensitive Whether the comparison should be case-sensitive.
 * @return True if the strings are equal, false otherwise.
 */
inline bool compare(const std::string &str, const std::string &search, bool case_sensitive = true) {
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
 * @brief Split a string into a vector of strings using a delimiter.
 * @param s The input string.
 * @param delim The delimiter to split by.
 * @return A vector of strings split by the delimiter.
 */
inline std::vector<std::string> split(const std::string &s, const std::string &delim) {
  std::vector<std::string> elems;
  size_t pos_start = 0, pos_end;
  std::string token;
  size_t delim_len = delim.size();
  while ((pos_end = s.find(delim, pos_start)) != std::string::npos) {
    token = s.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + delim_len;
    elems.push_back(token);
  }
  elems.push_back(s.substr(pos_start)); // Add the last token after the last delimiter
  return elems;
}
/*!
 * @brief Split a string into a vector of strings using a single character delimiter.
 * @param s The input string.
 * @param delim The character delimiter to split by.
 * @return A vector of strings split by the character delimiter.
 */
inline std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}
/*!
 * @brief Escape underscores in a string by replacing them with "\\_".
 * @param in The input string.
 * @return A new string with underscores escaped.
 */
inline std::string escape_underscore(const std::string &in) {
  std::string out;
  out.reserve(in.size() + 8); // in most cases we have 4 '_' to be replaced by '\\_'
  std::string replace = "\\_";
  char origin = '_';

  for (char c : in) {
    if (c == origin) {
      out += replace;
    } else {
      out += c;
    }
  }
  return out;
}

/*!
 * @brief Convert a bool or integer value to its string representation ("true"/"false" for bool, for int).
 * @param value The input value (bool or integer).
 * @return String representation.
 */
template <typename T>
inline std::string bool_to_string_bool(const T value) {
  if constexpr (std::is_same_v<T, bool>) {
    return value ? "true" : "false";
  } else if constexpr (std::is_integral_v<T>) {
    return value == 0 ? "false" : "true";
  } else {
    static_assert(std::is_same_v<T, bool> || std::is_integral_v<T>, "Type must be bool or integral");
    return {};
  }
}

template <typename T>
inline std::string copper_to_string_bool(const T value) {
  if constexpr (std::is_same_v<T, bool>) {
    return value ? "on" : "off";
  } else if constexpr (std::is_integral_v<T>) {
    return value == 0 ? "off" : "on";
  } else {
    static_assert(std::is_same_v<T, bool> || std::is_integral_v<T>, "Type must be bool or integral");
    return {};
  }
}

// *********************  N U M E R I C A L   F U N C T I O N S *********************

/*!
 * @brief mystod converts a string to a double; std::stod does not work in debug mode (March 2024)
 * @param s The input string.
 * @return The converted double value.
 */
inline double mystod(const std::string &s) {
  std::stringstream ss(s);
  double d;
  ss >> d;
  return d;
}

/*!
 * @brief isdigit_first_char checks if the first character of a string is a digit or a sign followed by a digit.
 * @details checks if the first character is a digit (0-9) or if it is a sign ('+' or '-') followed by a digit.
 * @param str The input string.
 * @return True if the first character is a digit or a sign followed by a digit, false otherwise.
 */
inline bool isdigit_first_char(const std::string &str) {
  if (str.empty())
    return false;
  if (str.at(0) == '+' || str.at(0) == '-') {
    return str.size() > 1 && std::isdigit(str.at(1));
  }
  return std::isdigit(str.at(0));
}

template <class T>
/*!
 * \brief zero_fill_field fills field padded with zeros: 4 -> 0004, -4 -> -004
 * \param num INTEGER type
 * \param width field width
 * \return string with at least width characters, starting with 0 or -0 in case
 */
inline std::string zero_fill_field(const T num, unsigned int width) {
  std::ostringstream oss;
  if (num < 0) {
    oss << '-';
    --width;
  }
  oss << std::setfill('0') << std::setw(width) << (num < 0 ? -num : num);
  return oss.str();
}

/*!
 * @brief Create a vector of stringstreams with right-adjusted integer values.
 * @tparam T Integral type (e.g., int, long)
 * @param vals Input vector of integral values.
 * @return Vector of stringstreams with right-adjusted values.
 */
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

/*!
 * @brief Create a vector of stringstreams with right-adjusted double values.
 * @tparam T Floating-point type (e.g., float, double)
 * @param vals Input vector of floating-point values.
 * @param low Lower threshold for scientific notation.
 * @param high Upper threshold for scientific notation.
 * @param sci_prec Precision for scientific notation.
 * @return Vector of stringstreams with right-adjusted values.
 */
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

/*!
 * @brief Extracts the channel number from a channel file path, like leading/084_ADU-07e_C000_TEx_128Hz.extension
 * @details The channel number is expected to be the first number after '_C' in the filename.
 * If the channel number cannot be found, SIZE_MAX is returned.
 * This function does not throw exceptions; it returns SIZE_MAX in case of errors.
 * @param p The file path.
 * @return The channel number or SIZE_MAX if not found.
 */
inline size_t channel_number_from_channel_file(const std::filesystem::path &p) {
  // leading/084_ADU-07e_C000_TEx_128Hz.extension
  // get the filename without extension
  std::string filename = p.filename().string();
  // channel number is the first number after '_C'
  auto pos_a = filename.find("_C");
  // find next '_'
  if (pos_a == std::string::npos)
    return SIZE_MAX; // no channel number found
  pos_a += 2;        // move past '_C'
  auto pos_b = filename.find('_', pos_a);
  if (pos_b == std::string::npos)
    return SIZE_MAX; // no channel number found
  // extract the channel number
  std::string channel_number_str = filename.substr(pos_a, pos_b - pos_a);
  // convert to size_t but; no exception handling here, SIZE_MAX is returned in case of error
  try {
    return std::stoul(channel_number_str);
  } catch (const std::invalid_argument &e) {
    std::cerr << "Invalid channel number in file: " << p << " - " << e.what() << std::endl;
    return SIZE_MAX; // return SIZE_MAX in case
  } catch (const std::out_of_range &e) {
    std::cerr << "Channel number out of range in file: " << p << " - " << e.what() << std::endl;
    return SIZE_MAX; // return SIZE_MAX
  }
}
// *********************** F R E Q U E N C Y   A N D   P E R I O D S ***********************/

/*!
 * @brief Creates a vector of stringstreams with right-adjusted frequency and period values. Input must be convertible to int.
 * @param fs Input vector of frequency values.
 * @return Vector of stringstreams with right-adjusted values.
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
/*!
 * \brief run2string
 * \param run like 1, 12, 123
 * \details This function converts a run number to a string formatted as "run_XXX",
 * where XXX is the run number zero-padded to 3 digits.
 * \return run_001 or run_012 and so on
 */
inline std::string run2string(const auto &run) {

  std::string srun("run_");
  return srun + mstr::zero_fill_field(run, 3);
}

/*!
 * \brief string2run
 * \param srun run_001 or run_012 and so on
 * \return 1 or 12 or SIZE_MAX if the string is not valid
 * @details This function extracts the run number from a string formatted as "run_XXX",
 */
inline size_t string2run(const std::string &srun) {
  // split the string at the last '_' and take the rest
  std::string ssrun = srun.substr(srun.find_last_of('_') + 1);
  if (ssrun.empty() || (ssrun.size() > 6))
    return SIZE_MAX;
  return size_t(std::stoul(ssrun));
}

/*!
 * \brief sample_rate_to_str converts 256 to 256 and Hz, 0.25 to 4 and s; if numbers results in to fractions and round_f_or_s is true return value is != 0; USE FOR beautiful output
 * \param sample_rate input
 * \param f_or_s sample rate either as Hz or s as ouput, ROUNDING is applied if round_f_or_s is true
 * \param unit either "Hz" or "s"
 * \param round_f_or_s round - 0 in case of flawless conversion; example: 4.00001Hz may be a numerical error, 4Hz wanted
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

/*!
 * @brief sample_rate_to_str_simple, similar to f_to_string, but number is rounded to the nearest integer and unit is either "Hz" or "s"
 * @details this is a simple converter making "4Hz", "4 Hz", "4 Hz ",  "4Hz ". Or if f is 0.25 it will return "4s", "4 s", "4 s ", "4s "
 * @param sample_rate frequency in Hz
 * @param add_space
 * @param append_space
 * @return formatted string like "4Hz", "4 Hz", "4 Hz ", "4Hz "
 */
inline std::string sample_rate_to_str_simple(const double &sample_rate, const bool add_space = false, const bool append_space = false) {
  double f_or_s = 0;
  std::string unit;
  bool round_f_or_s = true;
  double must_be_zero = mstr::sample_rate_to_str(sample_rate, f_or_s, unit, round_f_or_s);
  if (must_be_zero != 0.0)
    std::cerr << "sample_rate_to_str_simple: rounding error: " << must_be_zero << std::endl;

  std::string sval;
  sval = std::to_string(static_cast<uint32_t>(f_or_s));
  if (add_space)
    sval += " ";
  sval += unit;
  if (append_space)
    sval += " ";

  return sval;
}

/*!
 * @brief Converts a string representation of a sample rate to its numeric value.
 * @param srate The string representation of the sample rate (e.g., "256Hz", "0.25s").
 * @return The numeric value of the sample rate in Hz as double; ref to other functions
 */
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

/*!
 * @brief f_to_string a simple converter making "4Hz", "4 Hz", "4 Hz ",  "4Hz ". Or if f is 0.25 it will return "4s", "4 s", "4 s ", "4s "
 * @param f frequency in Hz
 * @param add_space
 * @param append_space
 * @return
 */
inline std::string f_to_string(const double &f, const bool add_space = false, const bool append_space = false) {
  std::stringstream ss;

  if (f < 1.0) {
    ss << (1. / f);
    if (add_space)
      ss << " ";
    ss << "s";
  } else {
    ss << f;
    if (add_space)
      ss << " ";
    ss << "Hz";
  }
  if (append_space)
    ss << " ";
  return ss.str();
}

// *********************  T I M E and D A T E  F U N C T I O N S *********************

// the std::chrono do not work with UTC reliably, so we use a custom utc_clock

/*!
 * @brief Converts a time_t aka int value to a date and time string in UTC; the most common case.
 * @param ti The time_t value. time_t has no fractions.
 * @param date The output date string in "YYYY-MM-DD" format.
 * @param time The output time string in "HH:MM:SS" format.
 */
inline void date_and_time(const time_t &ti, std::string &date, std::string &time) {
  struct tm tt = {0, 0, 0, 0, 0, 0, 0, 0};
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

/*!
 * @brief Creates a measurement directory name based on the given time_t value.
 * @param ti The time_t value representing the measurement time.
 * @return A string formatted as "meas_YYYY-MM-DD_HH-MM-SS".
 */
inline std::string measdir_time(const time_t &ti) {
  struct tm tt = {0, 0, 0, 0, 0, 0, 0, 0};
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
/*!
 * @brief Converts a date string in "YYYY-MM-DD" format to individual year, month, and day integers.
 * @param date_str The date string to convert.
 * @param year The output year integer.
 * @param month The output month integer.
 * @param day The output day integer.
 */
void date_to_numbers(const std::string &date_str, int &year, int &month, int &day) {
  // date_str is expected to be in "YYYY-MM-DD" format
  std::vector<std::string> parts = mstr::split(date_str, '-');
  if (parts.size() != 3) {
    year = month = day = 0; // invalid date
    return;
  }
  year = std::stoi(parts[0]);
  month = std::stoi(parts[1]);
  day = std::stoi(parts[2]);
}

/*!
 * @brief Converts a time string in "HH:MM:SS" format to individual hour, minute, and second integers.
 * @param time_str The time string to convert.
 * @param hour The output hour integer.
 * @param minute The output minute integer.
 * @param second The output second integer.
 */
void time_to_numbers(const std::string &time_str, int &hour, int &minute, int &second, double &fraction) {
  // time_str is expected to be in "HH:MM:SS" format
  // check for a trailing .123 if present
  std::vector<std::string> parts = mstr::split(time_str, ':');
  if (parts.size() != 3) {
    hour = minute = second = 0; // invalid time
    fraction = 0.0;
    return;
  }
  hour = std::stoi(parts[0]);
  minute = std::stoi(parts[1]);
  // check if seconds have a fraction part
  std::vector<std::string> s_parts = mstr::split(parts[2], '.');
  if (s_parts.size() == 2) {
    second = std::stoi(s_parts[0]);
    fraction = std::stod("0." + s_parts[1]);
  } else {
    second = std::stoi(parts[2]);
    fraction = 0.0;
  }
}

/*!
 * @brief Extracts the date from an ISO 8601 formatted string.
 * @param datetime The ISO 8601 formatted string.
 * @return The extracted date as a string.
 */
inline std::string get_date_from_iso8601(const std::string &datetime) {
  auto splits = mstr::split(datetime, 'T');
  if (splits.size() > 1)
    return splits.at(0);
  return std::string();
}
/*!
 * @brief Extracts the time from an ISO 8601 formatted string, excluding fractional seconds.
 * @details treat the fractional part later; this function intends to be used to make a time_t, which does not have fractions
 * @param datetime The ISO 8601 formatted string.
 * @return The extracted time as a string, without fractional seconds.
 */
inline std::string get_time_from_iso8601(const std::string &datetime) {
  auto splits = mstr::split(datetime, 'T');
  if (splits.size() > 1) {                         // vector has at least 2 parts
    auto splitss = mstr::split(splits.at(1), '.'); // split the time part at the dot to separate fractional seconds
    if (splitss.size())                            // we have at least one part
      return splitss.at(0);                        // and return the first part, regardless if there was a dot or not
  }
  return std::string();
}

inline time_t iso8601_to_time_t(const std::string &iso_datetime) {
  int year, month, day, hour, minute, second;
  double fraction = 0.0;
  // Extract date and time parts
  std::string date_part = mstr::get_date_from_iso8601(iso_datetime);
  std::string time_part = mstr::get_time_from_iso8601(iso_datetime);
  if (date_part.empty() || time_part.empty()) {
    std::cerr << "iso8601_to_time_t: Invalid ISO 8601 format: " << iso_datetime << std::endl;
    return -1; // return -1 in case of error
  }
  // Convert date part to numbers
  date_to_numbers(date_part, year, month, day);
  // Convert time part to numbers
  time_to_numbers(time_part, hour, minute, second, fraction);
  struct tm tt = {0, 0, 0, 0, 0, 0, 0, 0};
  tt.tm_year = year - 1900;
  tt.tm_mon = month - 1;
  tt.tm_mday = day;
  tt.tm_hour = hour;
  tt.tm_min = minute;
  tt.tm_sec = second;

  return timegm(&tt);
}
/*!
 * @brief take a time_t value and return a string in ISO 8601 format
 * @details The function can return just the date, just the time, or both date and time in ISO 8601 format.
 * @details If fractions are provided, they are added to the time string.
 * @details The function does not throw exceptions, but returns an empty string in case of errors, such as invalid fractions.
 * @param t The time_t value to convert.
 * @param iso_0_date_1_time_2 Specifies whether to return date (0), time (1), or both (2).
 * @param fracs The fractional seconds to include (if any).
 * @return The ISO 8601 formatted string.
 */
std::string iso8601_time_t(const time_t &t, int iso_0_date_1_time_2 = 0, const double fracs = 0.0) {
  std::string mydate, mytime;
  mstr::date_and_time(t, mydate, mytime); // have both date and time strings
  if (fracs >= zero_frac && fracs < 1.0) {
    // add fractions to time, convert to string which can hold this "0.00000095367431640625"
    std::ostringstream oss;
    oss << std::setprecision(20) << fracs;
    std::string sfrac = oss.str(); // preserves more digits
    // remove leading "0." if present
    if (sfrac.size() > 2 && sfrac.substr(0, 2) == "0.") {
      sfrac = sfrac.substr(2); // remove "0."
    } else {
      std::cerr << "iso8601_time_t: fractions must be in range [0, 1), got: " << fracs << std::endl;
      return {};
    }
    // remove trailing zeros
    while (!sfrac.empty() && sfrac.back() == '0') {
      sfrac.pop_back();
    }
    // if sfrac is empty, we have no fractions, so we do not add it
    if (sfrac.empty()) {
      return mydate + "T" + mytime; // no fractions, just date and time
    }
    // add fractions to time string
    mytime += "." + sfrac; // add fractions to time string, e.g. "12:34:56.789" or "12:34:56.789123456"
  } else if (fracs >= 1.0) {
    std::cerr << "iso8601_time_t: fractions must be in range [0, 1), got: " << fracs << std::endl;
    return {};
  }
  if (iso_0_date_1_time_2 == 0) {
    return mydate + "T" + mytime;
  } else if (iso_0_date_1_time_2 == 1) {
    return mydate;
  } else {
    return mytime;
  }
}

/*!
 * @brief Extracts the fractional seconds from an ISO 8601 formatted string.
 * @param datetime The ISO 8601 formatted string.
 * @return The extracted fractional seconds as a string.
 */
inline std::string get_fractional_seconds_from_iso8601(const std::string &datetime) {
  auto splits = mstr::split(datetime, '.');
  if (splits.size() > 1) { // vector has at least 2 parts, 2020-01-01T12:00:00 "." 123456
    std::string s("0.");
    s += splits.at(1);
    return s;
  }
  return std::string();
}

/*!
 * \brief concat ISO date and ISO time together with optional fraction of seconds, in case fraction is > 10E-12 secs
 * \param date like 2012-09-14
 * \param time like 14:32:45
 * @details if time is empty, date is interpreted as datetime like 2012-09-14T22:23:45
 * \param fracs positive numberlike 0.012 and greater as zero_frac
 * \return ISO 8601 formatted string like 2012-09-14T14:32:45.012345
 * @details if fracs is 0.0, no fraction is appended, otherwise it
 *          is appended as ".012345" to the time string, so the result is like
 *          2012-09-14T14:32:45.012345
 */
inline std::string iso8601_str_date_time(const std::string &date, const std::string &time, const double &fracs = 0.0) {
  if (date.empty())
    return std::string();

  if (time.empty()) {
    // date is interpreted as datetime like 2012-09-14T22:23:45
    if (fracs < mstr::zero_frac)
      return date; // no fraction, just return date
    else
      return date + "T" + time + std::format(".{:014}", static_cast<uint64_t>(fracs * 100000000000000));
  }

  if (fracs < mstr::zero_frac)
    return date + "T" + time; // no fraction, just return date and time

  // append fraction to the time string
  return date + "T" + time + std::format(".{:014}", static_cast<uint64_t>(fracs * 100000000000000));
}

//// some helper functions for the tm struct
/*!
 * \@brief tm_to_num_date return simply the date numbers from a std::tm struct
 * \@param date
 * \@param year
 * \@param month
 * \@param day
 */
void tm_to_num_date(const std::tm *date, int &year, int &month, int &day) {
  year = date->tm_year + 1900;
  month = date->tm_mon + 1;
  day = date->tm_mday;
}

/*!
 * \@brief tm_to_num_date return simply the time numbers fron a std::tm struct
 * \@param date
 * \@param year
 * \@param month
 * \@param day
 */
void tm_to_num_time(const std::tm *date, int &hour, int &min, int &sec) {
  hour = date->tm_hour;
  min = date->tm_min;
  sec = date->tm_sec;
}

/*!
 * \@brief tm_to_str_date simple conversion to something like 2021-05-19
 * \@param date
 * \@return
 */
std::string tm_to_str_date(const std::shared_ptr<tm> &date) {
  return std::to_string(date->tm_year + 1900) + "-" + mstr::zero_fill_field(date->tm_mon + 1, 2) + "-" + mstr::zero_fill_field(date->tm_mday, 2);
}

/*!
 * \@brief tm_to_str_time simple conversion to something like 14:22:50
 * \@param date
 * \@return
 */
std::string tm_to_str_time(const std::tm *date) {
  return mstr::zero_fill_field(date->tm_hour, 2) + ":" + mstr::zero_fill_field(date->tm_min + 1, 2) + ":" + mstr::zero_fill_field(date->tm_sec, 2);
}

/*!
 * \@brief time_from_ints create a std::tm from numbers
 * \@param YYYY
 * \@param MM
 * \@param DD
 * \@param hh
 * \@param mm
 * \@param ss
 * \@return
 */
std::shared_ptr<tm> time_from_ints(const int YYYY = 0, const int MM = 0, const int DD = 0, const int hh = 0, const int mm = 0, const int ss = 0) {
  std::shared_ptr<tm> dt = std::make_shared<tm>();
  dt->tm_year = YYYY - 1900; // Years from 1900
  dt->tm_mon = MM - 1;       // 0-based
  dt->tm_mday = DD;          // 1 based

  dt->tm_hour = hh;
  dt->tm_min = mm;
  dt->tm_sec = ss;
  dt->tm_isdst = 0;
  std::mktime(dt.get());

  return dt;
}

} // namespace mstr

#endif // STRINGS_ETC_HPP